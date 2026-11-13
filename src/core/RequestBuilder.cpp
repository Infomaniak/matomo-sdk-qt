/*
 * Infomaniak - matomo-sdk-qt
 * Copyright (C) 2026 Infomaniak Network SA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <MatomoQt/RequestBuilder.h>
#include <MatomoQt/RequestStatus.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QUrlQuery>

#include <concepts>
#include <utility>

namespace MatomoQt {

namespace {

RequestBuildResult failure(RequestStatus::Value status, QString message) {
    return RequestBuildResult{RequestResult{status, std::move(message)}, {}};
}

RequestBuildResult invalidConfig(QString message) {
    return failure(RequestStatus::Value::RequestInvalidConfig, std::move(message));
}

RequestBuildResult invalidPayload(QString message) {
    return failure(RequestStatus::Value::RequestInvalidPayload, std::move(message));
}

bool isJsonObject(const QString &value) {
    QJsonParseError error{};
    const auto document = QJsonDocument::fromJson(value.toUtf8(), &error);
    return error.error == QJsonParseError::NoError && document.isObject();
}

bool hasValidActionUrlBase(const QUrl &url) {
    return url.isValid() && !url.isEmpty() && !url.isRelative();
}

bool isRelativeApplicationPath(const QUrl &reference) {
    return reference.scheme().isEmpty() && reference.authority().isEmpty();
}

/**
 * Returns true for a base made of a scheme and an empty authority, such as `app://`.
 *
 * QUrl::resolved() returns an empty URL for such a base, and promoting the path to the host
 * through QUrl would lowercase it, so these bases are used as a plain prefix instead.
 */
bool isSchemeOnlyBase(const QUrl &base) {
    // Compared on the serialized form: whether QUrl reports an empty or a null authority for
    // `app://` differs between Qt versions.
    return !base.scheme().isEmpty() && base.toString() == base.scheme() + QStringLiteral("://");
}

QString actionUrlFor(const QUrl &base, const QUrl &reference) {
    if (isSchemeOnlyBase(base)) {
        // A leading slash would produce an empty authority (`app:///home`), not `app://home`.
        QString path = reference.toString();
        while (path.startsWith(QLatin1Char('/'))) {
            path.remove(0, 1);
        }
        return base.toString() + path;
    }

    return base.resolved(reference).toString();
}

bool hasValidClientId(const QString &clientId) {
    static const QRegularExpression clientIdPattern(QStringLiteral("^[0-9A-Fa-f]{16}$"));
    return clientId.isEmpty() || clientIdPattern.match(clientId).hasMatch();
}

QString formattedEventValue(double value) {
    return QLocale::c().toString(value, 'g', 15);
}

bool addCustomDimensions(QUrlQuery &query, const QList<CustomDimension> &dimensions, QString *errorMessage) {
    QSet<int> usedDimensions;

    for (const auto &dimension: dimensions) {
        if (!dimension.isValid()) {
            *errorMessage = QStringLiteral("Custom dimension ID must be between %1 and %2.")
                                    .arg(CustomDimension::MinId)
                                    .arg(CustomDimension::MaxId);
            return false;
        }

        if (dimension.value.isEmpty()) {
            continue;
        }

        if (usedDimensions.contains(dimension.id)) {
            *errorMessage = QStringLiteral("Custom dimensions must not contain duplicate IDs.");
            return false;
        }

        usedDimensions.insert(dimension.id);
        query.addQueryItem(QStringLiteral("dimension%1").arg(dimension.id), dimension.value);
    }

    return true;
}

template<typename AddPayloadParameters>
concept PayloadParameterAppender = requires(AddPayloadParameters addPayloadParameters, QUrlQuery *query) {
    { addPayloadParameters(query) } -> std::same_as<RequestBuildResult>;
};

template<PayloadParameterAppender AddPayloadParameters>
RequestBuildResult buildRequest(const TrackerConfig &config, const QString &actionUrl, const RequestBuildOptions &options,
                                AddPayloadParameters addPayloadParameters) {
    if (!config.isValid()) {
        return invalidConfig(QStringLiteral("Tracker endpoint, action URL base and site ID are required."));
    }

    if (!hasValidActionUrlBase(config.actionUrlBase)) {
        return invalidConfig(QStringLiteral("Action URL base must be an absolute URL."));
    }

    if (!hasValidClientId(options.clientId)) {
        return invalidPayload(QStringLiteral("Client ID must be 16 hexadecimal characters."));
    }

    const QString clientHints = options.clientHints.trimmed();
    if (!clientHints.isEmpty() && !isJsonObject(clientHints)) {
        return invalidPayload(QStringLiteral("Client hints must be a JSON object."));
    }

    QUrl url = config.endpoint;
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("idsite"), QString::number(config.siteId));
    query.addQueryItem(QStringLiteral("rec"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("apiv"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("url"), actionUrl);

    if (!options.clientId.isEmpty()) {
        query.addQueryItem(QStringLiteral("_id"), options.clientId);
    }

    if (!options.userAgent.trimmed().isEmpty()) {
        query.addQueryItem(QStringLiteral("ua"), options.userAgent.trimmed());
    }

    if (!options.language.trimmed().isEmpty()) {
        query.addQueryItem(QStringLiteral("lang"), options.language.trimmed());
    }

    if (!options.screenResolution.trimmed().isEmpty()) {
        query.addQueryItem(QStringLiteral("res"), options.screenResolution.trimmed());
    }

    if (!clientHints.isEmpty()) {
        query.addQueryItem(QStringLiteral("uadata"), clientHints);
    }

    if (auto payloadResult = addPayloadParameters(&query); !payloadResult.accepted()) {
        return payloadResult;
    }

    // QUrlQuery leaves literal '+' unencoded, but Matomo (PHP) decodes '+' as a space in query
    // values, corrupting values like "C++" or exponent-formatted event values (e.g. 1e+20).
    url.setQuery(query.toString(QUrl::FullyEncoded).replace(QLatin1Char('+'), QLatin1String("%2B")), QUrl::StrictMode);
    return RequestBuildResult{RequestResult{RequestStatus::Value::RequestAccepted, {}}, TrackingRequest{url}};
}

} // namespace

RequestBuilder::RequestBuilder(TrackerConfig config) :
    m_config(std::move(config)) {}

TrackerConfig RequestBuilder::config() const {
    return m_config;
}

void RequestBuilder::setConfig(const TrackerConfig &config) {
    m_config = config;
}

RequestBuildResult RequestBuilder::buildPageView(const PageView &pageView, const RequestBuildOptions &options) const {
    if (!pageView.isValid()) {
        return invalidPayload(QStringLiteral("Page view path and custom dimensions must be valid."));
    }

    const QUrl actionReference(pageView.path.trimmed());
    if (!isRelativeApplicationPath(actionReference)) {
        return invalidPayload(QStringLiteral("Page view path must be a relative application path."));
    }

    return buildRequest(m_config, actionUrlFor(m_config.actionUrlBase, actionReference), options, [&pageView](QUrlQuery *query) {
        if (!pageView.actionName.trimmed().isEmpty()) {
            query->addQueryItem(QStringLiteral("action_name"), pageView.actionName.trimmed());
        }

        if (QString errorMessage; !addCustomDimensions(*query, pageView.customDimensions, &errorMessage)) {
            return invalidPayload(std::move(errorMessage));
        }

        return RequestBuildResult{RequestResult{RequestStatus::Value::RequestAccepted, {}}, {}};
    });
}

RequestBuildResult RequestBuilder::buildEvent(const Event &event, const RequestBuildOptions &options) const {
    if (!event.isValid()) {
        return invalidPayload(QStringLiteral("Event category, action, value and custom dimensions must be valid."));
    }

    QString actionUrl = m_config.actionUrlBase.toString();
    if (const QUrl actionReference(event.path.trimmed()); !actionReference.isEmpty()) {
        if (!isRelativeApplicationPath(actionReference)) {
            return invalidPayload(QStringLiteral("Event path must be a relative application path."));
        }
        actionUrl = actionUrlFor(m_config.actionUrlBase, actionReference);
    }

    return buildRequest(m_config, actionUrl, options, [&event](QUrlQuery *query) {
        query->addQueryItem(QStringLiteral("ca"), QStringLiteral("1"));
        query->addQueryItem(QStringLiteral("e_c"), event.category.trimmed());
        query->addQueryItem(QStringLiteral("e_a"), event.action.trimmed());

        if (!event.name.trimmed().isEmpty()) {
            query->addQueryItem(QStringLiteral("e_n"), event.name.trimmed());
        }

        if (event.value.has_value()) {
            query->addQueryItem(QStringLiteral("e_v"), formattedEventValue(*event.value));
        }

        if (QString errorMessage; !addCustomDimensions(*query, event.customDimensions, &errorMessage)) {
            return invalidPayload(std::move(errorMessage));
        }

        return RequestBuildResult{RequestResult{RequestStatus::Value::RequestAccepted, {}}, {}};
    });
}

RequestBuildResult RequestBuilder::buildPing(const QString &path, const RequestBuildOptions &options) const {
    if (path.trimmed().isEmpty()) {
        return invalidPayload(QStringLiteral("Tracking request path is required."));
    }

    const QUrl actionReference(path.trimmed());
    if (!isRelativeApplicationPath(actionReference)) {
        return invalidPayload(QStringLiteral("Tracking request path must be a relative application path."));
    }

    return buildRequest(m_config, actionUrlFor(m_config.actionUrlBase, actionReference), options, [](QUrlQuery *query) {
        query->addQueryItem(QStringLiteral("ping"), QStringLiteral("1"));
        return RequestBuildResult{RequestResult{RequestStatus::Value::RequestAccepted, {}}, {}};
    });
}

} // namespace MatomoQt

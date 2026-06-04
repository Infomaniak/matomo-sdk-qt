#include <MatomoQt/RequestBuilder.h>

#include <QtCore/QUrlQuery>
#include <QtTest/QtTest>

#include <algorithm>
#include <limits>

using namespace MatomoQt;

namespace {

class RequestBuilderTest : public QObject {
        Q_OBJECT

    private slots:
        static void buildsPageViewQuery();
        static void buildsEventQuery();
        static void buildsPingQuery();
        static void includesValidClientId();
        static void rejectsInvalidClientId();
        static void includesConfiguredClientContext();
        static void includesCustomDimensions();
        static void omitsEmptyOptionalFields();
        static void escapesReservedCharacters();
        static void encodesSerializedUrlQuery();
        static void acceptsLegacyPiwikEndpoint();
        static void dropsEndpointQueryParameters();
        static void rejectsInvalidConfig();
        static void rejectsInvalidPayload();
};

TrackerConfig validConfig() {
    return TrackerConfig{
            .endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php")),
            .actionUrlBase = QUrl(QStringLiteral("kdrive://app/")),
            .siteId = 42,
    };
}

QUrlQuery queryFor(const RequestBuildResult &result) {
    return QUrlQuery(result.request.url);
}

QString queryItemValue(const QUrlQuery &query, const QString &key) {
    const auto items = query.queryItems(QUrl::FullyDecoded);
    for (const auto &[itemKey, itemValue]: items) {
        if (itemKey == key) {
            return itemValue;
        }
    }

    return {};
}

bool hasQueryItem(const QUrlQuery &query, const QString &key) {
    const auto items = query.queryItems(QUrl::FullyDecoded);
    return std::ranges::any_of(items, [&key](const auto &item) { return item.first == key; });
}

} // namespace

void RequestBuilderTest::buildsPageViewQuery() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildPageView({
            .path = QStringLiteral("settings/account"),
            .actionName = QStringLiteral("Account settings"),
    });

    QVERIFY(result.accepted());
    QCOMPARE(result.request.url.adjusted(QUrl::RemoveQuery), QUrl(QStringLiteral("https://matomo.example.com/matomo.php")));

    const auto query = queryFor(result);
    QCOMPARE(queryItemValue(query, QStringLiteral("idsite")), QStringLiteral("42"));
    QCOMPARE(queryItemValue(query, QStringLiteral("rec")), QStringLiteral("1"));
    QCOMPARE(queryItemValue(query, QStringLiteral("apiv")), QStringLiteral("1"));
    QCOMPARE(queryItemValue(query, QStringLiteral("url")), QStringLiteral("kdrive://app/settings/account"));
    QCOMPARE(queryItemValue(query, QStringLiteral("action_name")), QStringLiteral("Account settings"));
}

void RequestBuilderTest::buildsEventQuery() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildEvent({
            .category = QStringLiteral("preferences"),
            .action = QStringLiteral("click"),
            .name = QStringLiteral("sync button"),
            .value = 3.5,
    });

    QVERIFY(result.accepted());

    const auto query = queryFor(result);
    QCOMPARE(queryItemValue(query, QStringLiteral("idsite")), QStringLiteral("42"));
    QCOMPARE(queryItemValue(query, QStringLiteral("rec")), QStringLiteral("1"));
    QCOMPARE(queryItemValue(query, QStringLiteral("apiv")), QStringLiteral("1"));
    QCOMPARE(queryItemValue(query, QStringLiteral("url")), QStringLiteral("kdrive://app/"));
    QCOMPARE(queryItemValue(query, QStringLiteral("ca")), QStringLiteral("1"));
    QCOMPARE(queryItemValue(query, QStringLiteral("e_c")), QStringLiteral("preferences"));
    QCOMPARE(queryItemValue(query, QStringLiteral("e_a")), QStringLiteral("click"));
    QCOMPARE(queryItemValue(query, QStringLiteral("e_n")), QStringLiteral("sync button"));
    QCOMPARE(queryItemValue(query, QStringLiteral("e_v")), QStringLiteral("3.5"));
    QVERIFY(!hasQueryItem(query, QStringLiteral("action_name")));
}

void RequestBuilderTest::buildsPingQuery() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildPing(QStringLiteral("/settings/account"));

    QVERIFY(result.accepted());

    const auto query = queryFor(result);
    QCOMPARE(queryItemValue(query, QStringLiteral("idsite")), QStringLiteral("42"));
    QCOMPARE(queryItemValue(query, QStringLiteral("rec")), QStringLiteral("1"));
    QCOMPARE(queryItemValue(query, QStringLiteral("apiv")), QStringLiteral("1"));
    QCOMPARE(queryItemValue(query, QStringLiteral("url")), QStringLiteral("kdrive://app/settings/account"));
    QCOMPARE(queryItemValue(query, QStringLiteral("ping")), QStringLiteral("1"));
    QVERIFY(!hasQueryItem(query, QStringLiteral("ca")));
}

void RequestBuilderTest::includesValidClientId() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildPing(QStringLiteral("settings"), {.clientId = QStringLiteral("0123456789abcdef")});

    QVERIFY(result.accepted());
    QCOMPARE(queryItemValue(queryFor(result), QStringLiteral("_id")), QStringLiteral("0123456789abcdef"));
}

void RequestBuilderTest::rejectsInvalidClientId() {
    const RequestBuilder builder(validConfig());

    const auto tooShort = builder.buildPing(QStringLiteral("settings"), {.clientId = QStringLiteral("0123456789abcde")});
    QCOMPARE(tooShort.result.status, RequestResult::Status::InvalidPayload);
    QVERIFY(tooShort.request.url.isEmpty());

    const auto notHex = builder.buildPing(QStringLiteral("settings"), {.clientId = QStringLiteral("0123456789abcdeg")});
    QCOMPARE(notHex.result.status, RequestResult::Status::InvalidPayload);
    QVERIFY(notHex.request.url.isEmpty());
}

void RequestBuilderTest::includesConfiguredClientContext() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildPageView({.path = QStringLiteral("settings")},
                                              {
                                                      .userAgent = QStringLiteral(" kDrive/4.0 Qt/6.8 Linux "),
                                                      .language = QStringLiteral(" fr-CH,fr;q=0.9 "),
                                                      .screenResolution = QStringLiteral(" 1920x1080 "),
                                              });

    QVERIFY(result.accepted());

    const auto query = queryFor(result);
    QCOMPARE(queryItemValue(query, QStringLiteral("ua")), QStringLiteral("kDrive/4.0 Qt/6.8 Linux"));
    QCOMPARE(queryItemValue(query, QStringLiteral("lang")), QStringLiteral("fr-CH,fr;q=0.9"));
    QCOMPARE(queryItemValue(query, QStringLiteral("res")), QStringLiteral("1920x1080"));
}

void RequestBuilderTest::includesCustomDimensions() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildPageView({
            .path = QStringLiteral("settings"),
            .customDimensions =
                    {
                            {.id = 1, .value = QStringLiteral("enabled")},
                            {.id = 7, .value = QStringLiteral("beta user")},
                    },
    });

    QVERIFY(result.accepted());

    const auto query = queryFor(result);
    QCOMPARE(queryItemValue(query, QStringLiteral("dimension1")), QStringLiteral("enabled"));
    QCOMPARE(queryItemValue(query, QStringLiteral("dimension7")), QStringLiteral("beta user"));
}

void RequestBuilderTest::omitsEmptyOptionalFields() {
    const RequestBuilder builder(validConfig());

    const auto pageView = builder.buildPageView({
            .path = QStringLiteral("settings"),
            .customDimensions = {{.id = 1}},
    });
    QVERIFY(pageView.accepted());
    QVERIFY(!hasQueryItem(queryFor(pageView), QStringLiteral("action_name")));
    QVERIFY(!hasQueryItem(queryFor(pageView), QStringLiteral("dimension1")));

    const auto event = builder.buildEvent({
            .category = QStringLiteral("preferences"),
            .action = QStringLiteral("click"),
    });
    QVERIFY(event.accepted());
    QVERIFY(!hasQueryItem(queryFor(event), QStringLiteral("e_n")));
    QVERIFY(!hasQueryItem(queryFor(event), QStringLiteral("e_v")));
    QVERIFY(!hasQueryItem(queryFor(event), QStringLiteral("ua")));
    QVERIFY(!hasQueryItem(queryFor(event), QStringLiteral("lang")));
    QVERIFY(!hasQueryItem(queryFor(event), QStringLiteral("res")));
}

void RequestBuilderTest::escapesReservedCharacters() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildPageView({
            .path = QStringLiteral("settings/a value + x"),
            .actionName = QStringLiteral("A&B=C% /? é"),
            .customDimensions = {{.id = 3, .value = QStringLiteral("x&y=z+é")}},
    });

    QVERIFY(result.accepted());

    const auto query = queryFor(result);
    QCOMPARE(queryItemValue(query, QStringLiteral("url")), QStringLiteral("kdrive://app/settings/a value + x"));
    QCOMPARE(queryItemValue(query, QStringLiteral("action_name")), QStringLiteral("A&B=C% /? é"));
    QCOMPARE(queryItemValue(query, QStringLiteral("dimension3")), QStringLiteral("x&y=z+é"));
    QCOMPARE(query.queryItems(QUrl::FullyDecoded).size(), 6);
}

void RequestBuilderTest::encodesSerializedUrlQuery() {
    const RequestBuilder builder(validConfig());
    const auto result = builder.buildPageView({
            .path = QStringLiteral("settings/a value + x"),
            .actionName = QStringLiteral("A&B=C% /?"),
            .customDimensions = {{.id = 3, .value = QStringLiteral("x&y=z+")}},
    });

    QVERIFY(result.accepted());

    const auto encodedUrl = result.request.url.toEncoded();
    QVERIFY2(encodedUrl.contains("url=kdrive://app/settings/a%20value%20+%20x"), encodedUrl.constData());
    QVERIFY2(encodedUrl.contains("action_name=A%26B%3DC%25%20/?"), encodedUrl.constData());
    QVERIFY2(encodedUrl.contains("dimension3=x%26y%3Dz+"), encodedUrl.constData());
}

void RequestBuilderTest::acceptsLegacyPiwikEndpoint() {
    auto config = validConfig();
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/piwik.php"));

    const RequestBuilder builder(config);
    const auto result = builder.buildPing(QStringLiteral("settings"));

    QVERIFY(result.accepted());
    QCOMPARE(result.request.url.adjusted(QUrl::RemoveQuery), QUrl(QStringLiteral("https://matomo.example.com/piwik.php")));
}

void RequestBuilderTest::dropsEndpointQueryParameters() {
    auto config = validConfig();
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php?idsite=999&token_auth=secret"));

    const RequestBuilder builder(config);
    const auto result = builder.buildPing(QStringLiteral("settings"));

    QVERIFY(result.accepted());

    const auto query = queryFor(result);
    QCOMPARE(queryItemValue(query, QStringLiteral("idsite")), QStringLiteral("42"));
    QVERIFY(!hasQueryItem(query, QStringLiteral("token_auth")));
}

void RequestBuilderTest::rejectsInvalidConfig() {
    auto config = validConfig();
    config.endpoint = QUrl();
    RequestBuilder builder(config);

    auto result = builder.buildPing(QStringLiteral("settings"));
    QCOMPARE(result.result.status, RequestResult::Status::InvalidConfig);
    QVERIFY(result.request.url.isEmpty());

    config = validConfig();
    config.siteId = 0;
    builder.setConfig(config);
    result = builder.buildPing(QStringLiteral("settings"));
    QCOMPARE(result.result.status, RequestResult::Status::InvalidConfig);
    QVERIFY(result.request.url.isEmpty());

    config = validConfig();
    config.actionUrlBase = QUrl(QStringLiteral("settings"));
    builder.setConfig(config);
    result = builder.buildPing(QStringLiteral("settings"));
    QCOMPARE(result.result.status, RequestResult::Status::InvalidConfig);
    QVERIFY(result.request.url.isEmpty());
}

void RequestBuilderTest::rejectsInvalidPayload() {
    const RequestBuilder builder(validConfig());

    QCOMPARE(builder.buildPageView({}).result.status, RequestResult::Status::InvalidPayload);
    QCOMPARE(builder.buildPing({}).result.status, RequestResult::Status::InvalidPayload);
    QCOMPARE(builder.buildEvent({.category = QStringLiteral("preferences")}).result.status,
             RequestResult::Status::InvalidPayload);
    QCOMPARE(builder.buildEvent({.category = QStringLiteral("preferences"),
                                 .action = QStringLiteral("click"),
                                 .value = std::numeric_limits<double>::quiet_NaN()})
                     .result.status,
             RequestResult::Status::InvalidPayload);
    QCOMPARE(builder.buildPageView({.path = QStringLiteral("settings"), .customDimensions = {{.id = 1000}}}).result.status,
             RequestResult::Status::InvalidPayload);
    QCOMPARE(builder.buildPageView({.path = QStringLiteral("settings"),
                                    .customDimensions = {{.id = 1, .value = QStringLiteral("a")},
                                                         {.id = 1, .value = QStringLiteral("b")}}})
                     .result.status,
             RequestResult::Status::InvalidPayload);
}

QTEST_MAIN(RequestBuilderTest)
#include "tst_requestbuilder.moc"

#include <MatomoQt/UserAgentBuilder.h>

#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QStringList>
#include <QtCore/QSysInfo>

#include <utility>

namespace MatomoQt {
namespace {

QString tokenPart(QString value) {
    value = value.trimmed();

    QString token;
    token.reserve(value.size());
    bool lastWasSeparator = false;

    for (const auto character: value) {
        const ushort code = character.unicode();
        const bool asciiAlphaNumeric =
                (code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z') || (code >= '0' && code <= '9');
        const bool tokenPunctuation =
                character == QLatin1Char('.') || character == QLatin1Char('_') || character == QLatin1Char('+');
        const bool separator = character.isSpace() || character == QLatin1Char('-');

        if (asciiAlphaNumeric || tokenPunctuation) {
            token.append(character);
            lastWasSeparator = false;
        } else if (separator && !token.isEmpty() && !lastWasSeparator) {
            token.append(QLatin1Char('-'));
            lastWasSeparator = true;
        }
    }

    while (token.endsWith(QLatin1Char('-'))) {
        token.chop(1);
    }

    return token;
}

QString productToken(const UserAgentInfo &info) {
    const QString name = tokenPart(info.productName);
    if (name.isEmpty()) {
        return {};
    }

    const QString version = tokenPart(info.productVersion);
    if (version.isEmpty()) {
        return name;
    }

    return name + QLatin1Char('/') + version;
}

QString qtToken(QString version) {
    version = tokenPart(std::move(version));
    if (version.isEmpty()) {
        return {};
    }

    return QStringLiteral("Qt/") + version;
}

QString joinTokens(const QStringList &tokens) {
    QStringList nonEmptyTokens;
    for (const auto &token: tokens) {
        if (!token.isEmpty()) {
            nonEmptyTokens.append(token);
        }
    }

    return nonEmptyTokens.join(QLatin1Char(' '));
}

QString osVersionPart(QString version, const QChar separator) {
    version = version.trimmed();
    if (version.isEmpty()) {
        return {};
    }

    QString normalized;
    normalized.reserve(version.size());
    bool lastWasSeparator = false;

    for (const auto character: version) {
        if (character.isDigit()) {
            normalized.append(character);
            lastWasSeparator = false;
        } else if ((character == QLatin1Char('.') || character == QLatin1Char('_')) && !normalized.isEmpty() &&
                   !lastWasSeparator) {
            normalized.append(separator);
            lastWasSeparator = true;
        }
    }

    while (normalized.endsWith(separator)) {
        normalized.chop(1);
    }

    return normalized;
}

QString desktopPlatformComment(const UserAgentInfo &info) {
    switch (info.operatingSystem) {
        case UserAgentOperatingSystem::Windows: {
            const QString version = osVersionPart(info.operatingSystemVersion, QLatin1Char('.'));
            if (info.architecture == UserAgentArchitecture::X64 || info.architecture == UserAgentArchitecture::Arm64) {
                return QStringLiteral("Windows NT %1; Win64; x64").arg(version.isEmpty() ? QStringLiteral("10.0") : version);
            }
            if (info.architecture == UserAgentArchitecture::Arm) {
                return QStringLiteral("Windows NT %1; ARM").arg(version.isEmpty() ? QStringLiteral("10.0") : version);
            }
            return QStringLiteral("Windows NT %1").arg(version.isEmpty() ? QStringLiteral("10.0") : version);
        }
        case UserAgentOperatingSystem::MacOS: {
            const QString version = osVersionPart(info.operatingSystemVersion, QLatin1Char('_'));
            const QString cpu =
                    info.architecture == UserAgentArchitecture::Arm64 || info.architecture == UserAgentArchitecture::Arm
                            ? QStringLiteral("ARM")
                            : QStringLiteral("Intel");
            if (version.isEmpty()) {
                return QStringLiteral("Macintosh; %1 Mac OS X").arg(cpu);
            }
            return QStringLiteral("Macintosh; %1 Mac OS X %2").arg(cpu, version);
        }
        case UserAgentOperatingSystem::Linux:
            switch (info.architecture) {
                case UserAgentArchitecture::X86:
                    return QStringLiteral("X11; Linux i686");
                case UserAgentArchitecture::Arm:
                    return QStringLiteral("X11; Linux armv7l");
                case UserAgentArchitecture::Arm64:
                    return QStringLiteral("X11; Linux aarch64");
                case UserAgentArchitecture::X64:
                case UserAgentArchitecture::Unknown:
                    return QStringLiteral("X11; Linux x86_64");
            }
            break;
        case UserAgentOperatingSystem::Unknown:
            return {};
    }

    return {};
}

UserAgentArchitecture architectureFromQt(QString architecture) {
    architecture = architecture.toLower();

    if (architecture == QStringLiteral("x86_64") || architecture == QStringLiteral("amd64")) {
        return UserAgentArchitecture::X64;
    }
    if (architecture == QStringLiteral("i386") || architecture == QStringLiteral("i486") ||
        architecture == QStringLiteral("i586") || architecture == QStringLiteral("i686") ||
        architecture == QStringLiteral("x86")) {
        return UserAgentArchitecture::X86;
    }
    if (architecture == QStringLiteral("arm64") || architecture == QStringLiteral("aarch64")) {
        return UserAgentArchitecture::Arm64;
    }
    if (architecture.startsWith(QStringLiteral("arm"))) {
        return UserAgentArchitecture::Arm;
    }

    return UserAgentArchitecture::Unknown;
}

QString currentOsVersion() {
    const auto version = QOperatingSystemVersion::current();
    if (version.majorVersion() < 0) {
        return {};
    }

    QStringList parts;
    parts.append(QString::number(version.majorVersion()));
    if (version.minorVersion() >= 0) {
        parts.append(QString::number(version.minorVersion()));
    }
    if (version.microVersion() >= 0) {
        parts.append(QString::number(version.microVersion()));
    }

    return parts.join(QLatin1Char('.'));
}

} // namespace

UserAgentInfo UserAgentBuilder::currentDesktopInfo(QString productName, QString productVersion) {
    UserAgentOperatingSystem operatingSystem = UserAgentOperatingSystem::Unknown;

#if defined(Q_OS_WIN)
    operatingSystem = UserAgentOperatingSystem::Windows;
#elif defined(Q_OS_MACOS)
    operatingSystem = UserAgentOperatingSystem::MacOS;
#elif defined(Q_OS_LINUX)
    operatingSystem = UserAgentOperatingSystem::Linux;
#endif

    return UserAgentInfo{
            .productName = std::move(productName),
            .productVersion = std::move(productVersion),
            .operatingSystem = operatingSystem,
            .operatingSystemVersion = currentOsVersion(),
            .architecture = architectureFromQt(QSysInfo::currentCpuArchitecture()),
            .qtVersion = QStringLiteral(QT_VERSION_STR),
    };
}

QString UserAgentBuilder::buildDesktopUserAgent(const UserAgentInfo &info) {
    const QString product = productToken(info);
    if (product.isEmpty()) {
        return {};
    }

    const QString qt = qtToken(info.qtVersion);
    const QString comment = desktopPlatformComment(info);
    if (comment.isEmpty()) {
        return joinTokens({product, qt});
    }

    return joinTokens({QStringLiteral("Mozilla/5.0 (") + comment + QLatin1Char(')'), product, qt});
}

} // namespace MatomoQt

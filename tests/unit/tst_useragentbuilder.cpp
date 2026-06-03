#include <MatomoQt/RequestBuilder.h>
#include <MatomoQt/UserAgentBuilder.h>

#include <QtCore/QUrlQuery>
#include <QtTest/QtTest>

using namespace MatomoQt;

namespace {

class UserAgentBuilderTest : public QObject {
        Q_OBJECT

    private slots:
        static void buildsWindowsX64UserAgent();
        static void buildsMacOSIntelUserAgent();
        static void buildsMacOSArmUserAgent();
        static void buildsLinuxX64UserAgent();
        static void buildsLinuxArm64UserAgent();
        static void buildsMinimalUnknownOsUserAgent();
        static void normalizesProductToken();
        static void rejectsEmptyProductName();
        static void currentDesktopInfoIncludesApplicationAndQtVersion();
        static void requestBuilderSendsBuiltUserAgent();
};

QString queryItemValue(const QUrlQuery &query, const QString &key) {
    const auto items = query.queryItems(QUrl::FullyDecoded);
    for (const auto &[itemKey, itemValue]: items) {
        if (itemKey == key) {
            return itemValue;
        }
    }

    return {};
}

TrackerConfig validConfig() {
    return TrackerConfig{
            .endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php")),
            .actionUrlBase = QUrl(QStringLiteral("kdrive://app/")),
            .siteId = 42,
    };
}

} // namespace

void UserAgentBuilderTest::buildsWindowsX64UserAgent() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Windows,
            .operatingSystemVersion = QStringLiteral("10.0"),
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsMacOSIntelUserAgent() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::MacOS,
            .operatingSystemVersion = QStringLiteral("14.4.1"),
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.2.3"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 14_4_1) kDrive/4.0.1 Qt/6.2.3"));
}

void UserAgentBuilderTest::buildsMacOSArmUserAgent() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::MacOS,
            .operatingSystemVersion = QStringLiteral("15.0"),
            .architecture = UserAgentArchitecture::Arm64,
            .qtVersion = QStringLiteral("6.2.3"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (Macintosh; ARM Mac OS X 15_0) kDrive/4.0.1 Qt/6.2.3"));
}

void UserAgentBuilderTest::buildsLinuxX64UserAgent() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsLinuxArm64UserAgent() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::Arm64,
            .qtVersion = QStringLiteral("6.11.1"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Linux aarch64) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsMinimalUnknownOsUserAgent() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .qtVersion = QStringLiteral("6.11.1"),
    });

    QCOMPARE(userAgent, QStringLiteral("kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::normalizesProductToken() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral(" kDrive Desktop! "),
            .productVersion = QStringLiteral(" 4.0 beta "),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral(" 6.11.1 "),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) kDrive-Desktop/4.0-beta Qt/6.11.1"));
}

void UserAgentBuilderTest::rejectsEmptyProductName() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral(" ! "),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
    });

    QVERIFY(userAgent.isEmpty());
}

void UserAgentBuilderTest::currentDesktopInfoIncludesApplicationAndQtVersion() {
    const auto info = UserAgentBuilder::currentDesktopInfo(QStringLiteral("kDrive"), QStringLiteral("4.0.1"));

    QCOMPARE(info.productName, QStringLiteral("kDrive"));
    QCOMPARE(info.productVersion, QStringLiteral("4.0.1"));
    QCOMPARE(info.qtVersion, QStringLiteral(QT_VERSION_STR));
}

void UserAgentBuilderTest::requestBuilderSendsBuiltUserAgent() {
    const RequestBuilder builder(validConfig());
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
    });

    const auto result = builder.buildPing(QStringLiteral("settings"), {.userAgent = userAgent});

    QVERIFY(result.accepted());
    QCOMPARE(queryItemValue(QUrlQuery(result.request.url), QStringLiteral("ua")), userAgent);
}

QTEST_MAIN(UserAgentBuilderTest)

#include "tst_useragentbuilder.moc"

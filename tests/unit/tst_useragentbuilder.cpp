#include <MatomoQt/RequestBuilder.h>
#include <MatomoQt/UserAgentBuilder.h>

#include <QtCore/QUrlQuery>
#include <QtTest/QtTest>

using namespace MatomoQt;

Q_DECLARE_METATYPE(MatomoQt::UserAgentArchitecture)

namespace {

class UserAgentBuilderTest : public QObject {
        Q_OBJECT

    private slots:
        static void buildsWindowsX64UserAgent();
        static void buildsMacOSIntelUserAgent();
        static void buildsMacOSArmUserAgent();
        static void buildsLinuxX64UserAgent();
        static void buildsLinuxArm64UserAgent();
        static void buildsLinuxUnknownArchUserAgent();
        static void buildsLinuxX64WithDistro();
        static void buildsLinuxArm64WithDistro();
        static void buildsLinuxUnknownArchWithDistro();
        static void buildsLinuxX64WithMultiWordDistro();
        static void buildsLinuxX64WithEmptyDistroFallsBack();
        static void trimsWindowsNtVersionToMajorMinor();
        static void buildsMinimalUnknownOsUserAgent();
        static void normalizesProductToken();
        static void rejectsEmptyProductName();
        static void mapsQtCpuArchitecture_data();
        static void mapsQtCpuArchitecture();
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

void UserAgentBuilderTest::buildsLinuxUnknownArchUserAgent() {
    // An unsupported CPU (e.g. riscv64 maps to Unknown) must not be mislabelled as x86_64;
    // emit a generic Linux comment so DeviceDetector still attributes the platform correctly.
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::Unknown,
            .qtVersion = QStringLiteral("6.11.1"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Linux) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsLinuxX64WithDistro() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
            .linuxDistro = QStringLiteral("Ubuntu"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Ubuntu; Linux x86_64) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsLinuxArm64WithDistro() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::Arm64,
            .qtVersion = QStringLiteral("6.11.1"),
            .linuxDistro = QStringLiteral("Ubuntu"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Ubuntu; Linux aarch64) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsLinuxUnknownArchWithDistro() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::Unknown,
            .qtVersion = QStringLiteral("6.11.1"),
            .linuxDistro = QStringLiteral("Ubuntu"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Ubuntu; Linux) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsLinuxX64WithMultiWordDistro() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
            .linuxDistro = QStringLiteral("Linux Mint"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Linux Mint; Linux x86_64) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::buildsLinuxX64WithEmptyDistroFallsBack() {
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Linux,
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
            .linuxDistro = QStringLiteral("  "),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) kDrive/4.0.1 Qt/6.11.1"));
}

void UserAgentBuilderTest::trimsWindowsNtVersionToMajorMinor() {
    // QOperatingSystemVersion exposes the Windows build number as the micro component, but real
    // UAs (and DeviceDetector's NT-version map) only expect "Windows NT <major>.<minor>".
    const auto userAgent = UserAgentBuilder::buildDesktopUserAgent({
            .productName = QStringLiteral("kDrive"),
            .productVersion = QStringLiteral("4.0.1"),
            .operatingSystem = UserAgentOperatingSystem::Windows,
            .operatingSystemVersion = QStringLiteral("10.0.19045"),
            .architecture = UserAgentArchitecture::X64,
            .qtVersion = QStringLiteral("6.11.1"),
    });

    QCOMPARE(userAgent, QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) kDrive/4.0.1 Qt/6.11.1"));
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

void UserAgentBuilderTest::mapsQtCpuArchitecture_data() {
    QTest::addColumn<QString>("qtArchitecture");
    QTest::addColumn<UserAgentArchitecture>("expected");

    QTest::newRow("x86_64") << QStringLiteral("x86_64") << UserAgentArchitecture::X64;
    QTest::newRow("amd64") << QStringLiteral("amd64") << UserAgentArchitecture::X64;
    QTest::newRow("i386") << QStringLiteral("i386") << UserAgentArchitecture::X86;
    QTest::newRow("i686") << QStringLiteral("i686") << UserAgentArchitecture::X86;
    QTest::newRow("x86") << QStringLiteral("x86") << UserAgentArchitecture::X86;
    QTest::newRow("arm64") << QStringLiteral("arm64") << UserAgentArchitecture::Arm64;
    QTest::newRow("aarch64") << QStringLiteral("aarch64") << UserAgentArchitecture::Arm64;
    QTest::newRow("armv7l") << QStringLiteral("armv7l") << UserAgentArchitecture::Arm;
    QTest::newRow("arm") << QStringLiteral("arm") << UserAgentArchitecture::Arm;
    QTest::newRow("normalizes case") << QStringLiteral("AArch64") << UserAgentArchitecture::Arm64;
    QTest::newRow("unrecognized") << QStringLiteral("riscv64") << UserAgentArchitecture::Unknown;
    QTest::newRow("empty") << QString() << UserAgentArchitecture::Unknown;
}

void UserAgentBuilderTest::mapsQtCpuArchitecture() {
    QFETCH(QString, qtArchitecture);
    QFETCH(UserAgentArchitecture, expected);

    QCOMPARE(UserAgentBuilder::architectureFromQt(qtArchitecture), expected);
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

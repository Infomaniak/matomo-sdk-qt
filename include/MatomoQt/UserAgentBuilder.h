#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

enum class UserAgentOperatingSystem {
    Unknown,
    Windows,
    MacOS,
    Linux,
};

enum class UserAgentArchitecture {
    Unknown,
    X86,
    X64,
    Arm,
    Arm64,
};

struct MATOMOQT_CORE_EXPORT UserAgentInfo {
        QString productName;
        QString productVersion;
        UserAgentOperatingSystem operatingSystem = UserAgentOperatingSystem::Unknown;
        QString operatingSystemVersion;
        UserAgentArchitecture architecture = UserAgentArchitecture::Unknown;
        QString qtVersion;
};

/** Builds desktop User-Agent strings for Matomo request context.
 *
 * The generated OS tokens intentionally follow forms recognized by Matomo's
 * DeviceDetector regexes and fixtures, for example `Windows NT ...`,
 * `Macintosh; ... Mac OS X ...` and `X11; Linux ...`.
 *
 * The Qt-based detection approach is inspired by pbek/qt-piwik-tracker, but
 * this SDK keeps product names injected by the host application and does not
 * hardcode any downstream app or vendor.
 *
 * References:
 * - Matomo Tracking HTTP API `ua` parameter:
 *   https://developer.matomo.org/api-reference/tracking-api
 * - Matomo DeviceDetector regexes and fixtures:
 *   https://github.com/matomo-org/device-detector
 */
class MATOMOQT_CORE_EXPORT UserAgentBuilder {
    public:
        [[nodiscard]] static UserAgentInfo currentDesktopInfo(QString productName, QString productVersion);
        [[nodiscard]] static QString buildDesktopUserAgent(const UserAgentInfo &info);
        [[nodiscard]] static UserAgentArchitecture architectureFromQt(QString architecture);
};

} // namespace MatomoQt

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
        /** OS version, e.g. `10.0.22631` on Windows, `14.7.0` on macOS, and the
         *  distribution's `VERSION_ID` from /etc/os-release on Linux (`26.04`). */
        QString operatingSystemVersion;
        UserAgentArchitecture architecture = UserAgentArchitecture::Unknown;
        QString qtVersion;
        QString linuxDistro;
};

/** Builds desktop User-Agent strings for Matomo request context.
 *
 * The generated OS tokens intentionally follow forms recognized by Matomo's
 * DeviceDetector regexes and fixtures, for example `Windows NT ...`,
 * `Macintosh; ... Mac OS X ...` and `X11; Linux ...`.
 *
 * On Linux, the distribution name (from /etc/os-release) is inserted into the
 * comment so DeviceDetector can classify the specific distro, e.g.
 * `X11; Ubuntu; Linux x86_64` instead of `X11; Linux x86_64`.
 *
 * The Linux distribution version is not part of the User-Agent: DeviceDetector
 * reads it from Client Hints only, see buildClientHints().
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

        /** Builds User-Agent Client Hints for Matomo's `uadata` parameter.
         *
         * Returns a compact JSON object such as
         * `{"platform":"Linux","platformVersion":"26.04","mobile":false,"model":""}`,
         * with the `Sec-CH-UA-Platform` platform names `Linux` and `macOS`. Only the
         * platform and its version are reported: `model` is always empty.
         *
         * Returns an empty string when the platform or its version is unknown, and on
         * Windows: there `platformVersion` must follow the Windows.Foundation
         * UniversalApiContract version (13.0.0 and above for Windows 11), which Qt Core
         * does not expose, and sending the NT version `10.0.22631` would make Matomo
         * report Windows 11 as Windows 10.
         *
         * References:
         * - https://wicg.github.io/ua-client-hints/
         * - https://developer.matomo.org/api-reference/tracking-api
         */
        [[nodiscard]] static QString buildClientHints(const UserAgentInfo &info);
        [[nodiscard]] static UserAgentArchitecture architectureFromQt(QString architecture);
        [[nodiscard]] static QString currentLinuxDistro();
};

} // namespace MatomoQt

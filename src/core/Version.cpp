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

#include <MatomoQt/Version.h>

#ifndef MATOMOQT_VERSION_STRING
#define MATOMOQT_VERSION_STRING "0.0.0"
#endif

namespace MatomoQt {

QString versionString() {
    return QStringLiteral(MATOMOQT_VERSION_STRING);
}

} // namespace MatomoQt

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

#include <MatomoQt/DispatchStatus.h>
#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

/** Result of dispatching a tracking request over the network. */
struct MATOMOQT_CORE_EXPORT DispatchResult {
        DispatchStatus::Value status = DispatchStatus::Value::NetworkError;
        int httpStatus = 0;
        QString message;

        /** Returns true when the request was sent and the server returned a 2xx response. */
        [[nodiscard]] bool success() const;
};
} // namespace MatomoQt

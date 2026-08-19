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

#include <MatomoQt/ConsentState.h>
#include <MatomoQt/Export.h>

namespace MatomoQt {

/**
 * Interface used by applications that want SDK-managed consent persistence.
 */
class MATOMOQT_CORE_EXPORT ConsentStore {
    public:
        ConsentStore() = default;
        ConsentStore(const ConsentStore &) = delete;
        ConsentStore &operator=(const ConsentStore &) = delete;
        ConsentStore(ConsentStore &&) = delete;
        ConsentStore &operator=(ConsentStore &&) = delete;
        virtual ~ConsentStore();

        /** Returns the stored consent state, or Unknown when none exists. */
        [[nodiscard]] virtual ConsentState::Value consentState() const = 0;

        /** Persists the consent state chosen by the application. */
        virtual void setConsentState(ConsentState::Value state) = 0;
};

} // namespace MatomoQt

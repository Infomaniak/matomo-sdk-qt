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

#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/Export.h>

class QSettings;

namespace MatomoQt {

/**
 * QSettings-based persistent consent store.
 *
 * The provided QSettings pointer is not owned by this instance.
 * The caller must ensure the QSettings outlives the store.
 */
class MATOMOQT_CORE_EXPORT QSettingsConsentStore : public ConsentStore {
    public:
        explicit QSettingsConsentStore(QSettings *settings);
        ~QSettingsConsentStore() override = default;

        [[nodiscard]] ConsentState::Value consentState() const override;
        void setConsentState(ConsentState::Value state) override;

    private:
        QSettings *m_settings = nullptr;
};

} // namespace MatomoQt

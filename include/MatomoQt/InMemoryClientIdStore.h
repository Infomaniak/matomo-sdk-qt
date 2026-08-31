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

#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

/**
 * Volatile in-memory client ID store.
 *
 * The ID is lost when the instance is destroyed.
 * Suitable for unit tests and for applications that manage their own persistence.
 */
class MATOMOQT_CORE_EXPORT InMemoryClientIdStore : public ClientIdStore {
    public:
        InMemoryClientIdStore() = default;

        [[nodiscard]] QString clientId() const override;
        void setClientId(const QString &clientId) override;
        void clearClientId() override;

    private:
        QString m_clientId;
};

} // namespace MatomoQt

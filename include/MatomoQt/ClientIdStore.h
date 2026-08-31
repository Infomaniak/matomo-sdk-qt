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

/**
 * Interface used by applications that want SDK-managed client ID persistence.
 */
class MATOMOQT_CORE_EXPORT ClientIdStore {
    public:
        ClientIdStore() = default;
        ClientIdStore(const ClientIdStore &) = delete;
        ClientIdStore &operator=(const ClientIdStore &) = delete;
        ClientIdStore(ClientIdStore &&) = delete;
        ClientIdStore &operator=(ClientIdStore &&) = delete;
        virtual ~ClientIdStore();

        /** Returns the stored client ID, or an empty string when none exists. */
        [[nodiscard]] virtual QString clientId() const = 0;

        /** Persists the client ID chosen by the application or SDK. */
        virtual void setClientId(const QString &clientId) = 0;

        /** Removes the stored client ID. */
        virtual void clearClientId() = 0;
};

} // namespace MatomoQt

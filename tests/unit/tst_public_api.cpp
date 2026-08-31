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

#include <MatomoQt/ClientIdStore.h>
#include <MatomoQt/ConsentState.h>
#include <MatomoQt/ConsentStore.h>
#include <MatomoQt/DispatchStatus.h>
#include <MatomoQt/PrivacyMode.h>
#include <MatomoQt/RequestStatus.h>

#include <QtCore/qmetaobject.h>

#include <array>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

namespace ConsentState = MatomoQt::ConsentState;
namespace DispatchStatus = MatomoQt::DispatchStatus;
namespace PrivacyMode = MatomoQt::PrivacyMode;
namespace RequestStatus = MatomoQt::RequestStatus;

template<typename Enum, std::size_t Size>
bool hasExpectedMetadata(const std::array<std::pair<Enum, std::string_view>, Size> &expectedValues) {
    const QMetaEnum metaEnum = QMetaEnum::fromType<Enum>();
    if (!metaEnum.isValid()) {
        return false;
    }

    for (const auto &[value, expectedKey]: expectedValues) {
        const char *key = metaEnum.valueToKey(static_cast<int>(value));
        if (key == nullptr || std::string_view(key) != expectedKey) {
            return false;
        }
    }
    return true;
}

} // namespace

static_assert(!std::is_copy_constructible_v<MatomoQt::ClientIdStore>);
static_assert(!std::is_copy_assignable_v<MatomoQt::ClientIdStore>);
static_assert(!std::is_move_constructible_v<MatomoQt::ClientIdStore>);
static_assert(!std::is_move_assignable_v<MatomoQt::ClientIdStore>);
static_assert(std::has_virtual_destructor_v<MatomoQt::ClientIdStore>);

static_assert(!std::is_copy_constructible_v<MatomoQt::ConsentStore>);
static_assert(!std::is_copy_assignable_v<MatomoQt::ConsentStore>);
static_assert(!std::is_move_constructible_v<MatomoQt::ConsentStore>);
static_assert(!std::is_move_assignable_v<MatomoQt::ConsentStore>);
static_assert(std::has_virtual_destructor_v<MatomoQt::ConsentStore>);

int main() {
    const bool validConsentState = hasExpectedMetadata(std::to_array<std::pair<ConsentState::Value, std::string_view>>({
            {ConsentState::Value::Unknown, "Unknown"},
            {ConsentState::Value::Granted, "Granted"},
            {ConsentState::Value::Denied, "Denied"},
            {ConsentState::Value::Withdrawn, "Withdrawn"},
    }));
    const bool validPrivacyMode = hasExpectedMetadata(std::to_array<std::pair<PrivacyMode::Value, std::string_view>>({
            {PrivacyMode::Value::Disabled, "Disabled"},
            {PrivacyMode::Value::RequiresConsent, "RequiresConsent"},
            {PrivacyMode::Value::ConsentExemptWithOptOut, "ConsentExemptWithOptOut"},
    }));
    const bool validRequestStatus = hasExpectedMetadata(std::to_array<std::pair<RequestStatus::Value, std::string_view>>({
            {RequestStatus::Value::RequestAccepted, "RequestAccepted"},
            {RequestStatus::Value::RequestDisabled, "RequestDisabled"},
            {RequestStatus::Value::RequestBlockedByPrivacy, "RequestBlockedByPrivacy"},
            {RequestStatus::Value::RequestInvalidConfig, "RequestInvalidConfig"},
            {RequestStatus::Value::RequestInvalidPayload, "RequestInvalidPayload"},
    }));
    const bool validDispatchStatus = hasExpectedMetadata(std::to_array<std::pair<DispatchStatus::Value, std::string_view>>({
            {DispatchStatus::Value::Success, "Success"},
            {DispatchStatus::Value::Timeout, "Timeout"},
            {DispatchStatus::Value::NetworkError, "NetworkError"},
            {DispatchStatus::Value::SslError, "SslError"},
            {DispatchStatus::Value::CircuitBreakerOpen, "CircuitBreakerOpen"},
    }));

    return validConsentState && validPrivacyMode && validRequestStatus && validDispatchStatus ? 0 : 1;
}

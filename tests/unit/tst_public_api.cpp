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
#include <MatomoQt/ConsentStore.h>

#include <type_traits>

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
    return 0;
}

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

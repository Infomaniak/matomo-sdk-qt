#include <MatomoQt/DispatchResult.h>

namespace MatomoQt {

bool DispatchResult::success() const {
    return status == Status::Success;
}

} // namespace MatomoQt

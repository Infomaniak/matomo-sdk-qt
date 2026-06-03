#include <MatomoQt/RequestBuildResult.h>

namespace MatomoQt {

bool RequestBuildResult::accepted() const {
    return result.accepted();
}

} // namespace MatomoQt

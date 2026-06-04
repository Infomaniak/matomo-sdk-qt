#include <MatomoQt/RequestResult.h>

namespace MatomoQt {

bool RequestResult::accepted() const {
    return status == Status::Accepted;
}

} // namespace MatomoQt

#include <MatomoQt/TrackerConfig.h>

namespace MatomoQt {

bool TrackerConfig::isValid() const {
    return endpoint.isValid() && !endpoint.isEmpty() && siteId > 0;
}

} // namespace MatomoQt

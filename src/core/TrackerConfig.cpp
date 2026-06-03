#include <MatomoQt/TrackerConfig.h>

namespace MatomoQt {

bool TrackerConfig::isValid() const {
    const auto scheme = endpoint.scheme();
    return endpoint.isValid() && !endpoint.isEmpty() && (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) &&
           siteId > 0;
}

} // namespace MatomoQt

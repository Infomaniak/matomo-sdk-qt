#include <MatomoQt/Event.h>

#include <algorithm>
#include <cmath>

namespace MatomoQt {

bool Event::isValid() const {
    if (category.trimmed().isEmpty() || action.trimmed().isEmpty()) {
        return false;
    }

    if (value.has_value() && !std::isfinite(*value)) {
        return false;
    }

    return std::ranges::all_of(customDimensions, [](const CustomDimension &dimension) { return dimension.isValid(); });
}

} // namespace MatomoQt

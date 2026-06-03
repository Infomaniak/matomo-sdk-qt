#include <MatomoQt/Event.h>

#include <algorithm>
#include <cmath>

namespace MatomoQt {

bool Event::isValid() const {
    const bool hasValidValue = !value.has_value() || std::isfinite(*value);
    const bool hasValidDimensions = std::all_of(customDimensions.cbegin(), customDimensions.cend(),
                                                [](const CustomDimension &dimension) { return dimension.isValid(); });

    return !category.trimmed().isEmpty() && !action.trimmed().isEmpty() && hasValidValue && hasValidDimensions;
}

} // namespace MatomoQt

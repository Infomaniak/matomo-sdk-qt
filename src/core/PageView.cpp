#include <MatomoQt/PageView.h>

#include <algorithm>

namespace MatomoQt {

bool PageView::isValid() const {
    return !path.trimmed().isEmpty() && std::all_of(customDimensions.cbegin(), customDimensions.cend(),
                                                    [](const CustomDimension &dimension) { return dimension.isValid(); });
}

} // namespace MatomoQt

#include <MatomoQt/PageView.h>

#include <algorithm>

namespace MatomoQt {

bool PageView::isValid() const {
    return !path.trimmed().isEmpty() && std::ranges::all_of(customDimensions,
                                                    [](const CustomDimension &dimension) { return dimension.isValid(); });
}

} // namespace MatomoQt

#include <MatomoQt/PageView.h>

namespace MatomoQt {

bool PageView::isValid() const {
    return !path.trimmed().isEmpty();
}

} // namespace MatomoQt

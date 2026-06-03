#include <MatomoQt/Event.h>

namespace MatomoQt {

bool Event::isValid() const {
    return !category.trimmed().isEmpty() && !action.trimmed().isEmpty();
}

} // namespace MatomoQt

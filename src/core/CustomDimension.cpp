#include <MatomoQt/CustomDimension.h>

namespace MatomoQt {

bool CustomDimension::isValid() const {
    return id >= 1 && id <= 999;
}

} // namespace MatomoQt

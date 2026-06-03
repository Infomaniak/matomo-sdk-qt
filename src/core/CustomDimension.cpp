#include <MatomoQt/CustomDimension.h>

namespace MatomoQt {

bool CustomDimension::isValid() const {
    return id > 0;
}

} // namespace MatomoQt

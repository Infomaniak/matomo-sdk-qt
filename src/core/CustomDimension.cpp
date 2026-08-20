#include <MatomoQt/CustomDimension.h>

namespace MatomoQt {

bool CustomDimension::isValid() const {
    return id >= MinId && id <= MaxId;
}

} // namespace MatomoQt

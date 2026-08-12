#include <MatomoQt/Logging.h>

Q_LOGGING_CATEGORY(matomoSdk, "matomo.sdk")

namespace MatomoQt {

const QLoggingCategory &matomoSdkCategory() {
    return matomoSdk();
}

} // namespace MatomoQt

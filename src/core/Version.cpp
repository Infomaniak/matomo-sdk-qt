#include <MatomoQt/Version.h>

#ifndef MATOMOQT_VERSION_STRING
#define MATOMOQT_VERSION_STRING "0.0.0"
#endif

namespace MatomoQt {

QString versionString() {
    return QStringLiteral(MATOMOQT_VERSION_STRING);
}

} // namespace MatomoQt

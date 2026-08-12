#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

/** Result of dispatching a tracking request over the network. */
struct MATOMOQT_CORE_EXPORT DispatchResult {
        enum class Status {
            Success,
            Timeout,
            NetworkError,
            SslError,
            CircuitBreakerOpen,
        };

        Status status = Status::NetworkError;
        int httpStatus = 0;
        QString message;

        /** Returns true when the request was sent and the server returned a 2xx response. */
        [[nodiscard]] bool success() const;
};

} // namespace MatomoQt

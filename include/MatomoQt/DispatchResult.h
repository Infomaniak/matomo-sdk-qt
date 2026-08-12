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

    [[nodiscard]]  inline QString enumToString(const DispatchResult::Status state) {
        switch (state) {
            case DispatchResult::Status::Success:
                return QStringLiteral("Success");
            case DispatchResult::Status::Timeout:
                return QStringLiteral("Timeout");
            case DispatchResult::Status::NetworkError:
                return QStringLiteral("NetworkError");
            case DispatchResult::Status::SslError:
                return QStringLiteral("SslError");
            case DispatchResult::Status::CircuitBreakerOpen:
                return QStringLiteral("CircuitBreakerOpen");
        }
        return QStringLiteral("Unknown");
    }
} // namespace MatomoQt

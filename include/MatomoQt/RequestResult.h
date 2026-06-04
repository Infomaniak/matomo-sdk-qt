#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QString>

namespace MatomoQt {

/** Result of accepting or rejecting a tracking call. */
struct MATOMOQT_CORE_EXPORT RequestResult {
        enum class Status {
            Accepted,
            Disabled,
            BlockedByPrivacy,
            InvalidConfig,
            InvalidPayload,
        };

        Status status = Status::InvalidConfig;
        QString message;

        /** Returns true when the tracking call passed local validation. */
        [[nodiscard]] bool accepted() const;
};

} // namespace MatomoQt

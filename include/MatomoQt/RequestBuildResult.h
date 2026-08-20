#pragma once

#include <MatomoQt/Export.h>
#include <MatomoQt/RequestResult.h>
#include <MatomoQt/TrackingRequest.h>

namespace MatomoQt {

/** Result of building a Matomo tracking request. */
struct MATOMOQT_CORE_EXPORT RequestBuildResult {
        RequestResult result;
        TrackingRequest request;

        /** Returns true when a complete tracking request was built. */
        [[nodiscard]] bool accepted() const;
};

} // namespace MatomoQt

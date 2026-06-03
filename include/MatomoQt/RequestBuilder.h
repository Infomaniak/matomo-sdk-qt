#pragma once

#include <MatomoQt/Event.h>
#include <MatomoQt/Export.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/RequestBuildOptions.h>
#include <MatomoQt/RequestBuildResult.h>
#include <MatomoQt/TrackerConfig.h>

#include <QtCore/QString>

namespace MatomoQt {

/** Builds deterministic Matomo HTTP Tracking API requests without sending them. */
class MATOMOQT_CORE_EXPORT RequestBuilder {
    public:
        explicit RequestBuilder(TrackerConfig config);

        /** Returns the current builder configuration. */
        [[nodiscard]] TrackerConfig config() const;

        /** Replaces the current builder configuration. */
        void setConfig(const TrackerConfig &config);

        /** Builds a page view tracking request. */
        [[nodiscard]] RequestBuildResult buildPageView(const PageView &pageView, const RequestBuildOptions &options = {}) const;

        /** Builds an event tracking request. */
        [[nodiscard]] RequestBuildResult buildEvent(const Event &event, const RequestBuildOptions &options = {}) const;

        /** Builds a ping tracking request for the supplied application path. */
        [[nodiscard]] RequestBuildResult buildPing(const QString &path, const RequestBuildOptions &options = {}) const;

    private:
        TrackerConfig m_config;
};

} // namespace MatomoQt

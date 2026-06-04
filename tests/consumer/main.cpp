#include <MatomoQt/Event.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/Tracker.h>
#include <MatomoQt/TrackerConfig.h>
#include <MatomoQt/Version.h>

#include <QtCore/QString>
#include <QtCore/QUrl>

int main() {
    MatomoQt::TrackerConfig config;
    config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
    config.siteId = 1;

    MatomoQt::Tracker tracker(config);
    tracker.setConsentState(MatomoQt::ConsentState::Granted);

    const auto pageViewResult = tracker.trackPageView({.path = QStringLiteral("preferences")});
    const auto eventResult = tracker.trackEvent({
            .category = QStringLiteral("preferences"),
            .action = QStringLiteral("click"),
    });

    return MatomoQt::versionString().isEmpty() || !pageViewResult.accepted() || !eventResult.accepted() ? 1 : 0;
}

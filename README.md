# Matomo Qt SDK

[![CI](https://github.com/Infomaniak/matomo-sdk-qt/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/Infomaniak/matomo-sdk-qt/actions/workflows/ci.yml?query=branch%3Adevelop)
[![Qt 6](https://img.shields.io/badge/Qt-6.x-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-supported-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![Privacy](https://img.shields.io/badge/privacy-GDPR%20%2B%20Swiss%20FADP%2FnLPD-2F855A)](#privacy-model)
[![Status](https://img.shields.io/badge/status-stable-2F855A)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)

C++ Qt SDK for the Matomo Tracking HTTP API.

> Status: stable since 1.0.0. The SDK exposes C++ and QML targets, sends
> Matomo Tracking HTTP API requests asynchronously and includes a runnable QML
> example. The public API follows [Semantic Versioning](https://semver.org/);
> see [CHANGELOG.md](CHANGELOG.md).

## Project Context

This project was started to support Matomo integration in the
[kDrive desktop](https://github.com/Infomaniak/desktop-kDrive) application on
Linux.

The SDK itself is not kDrive-specific. It is designed to stay reusable by any
Qt or QML application that needs to send tracking data to a Matomo instance.
Product-specific event names, routes, identifiers and business metadata should
be defined by the host application, not by this library.

## Goals

- Provide a Qt-native SDK for Matomo tracking.
- Support both C++ and QML applications.
- Keep the core library independent from any product-specific taxonomy.
- Make privacy, consent, GDPR and Swiss FADP/nLPD-oriented integration explicit
  in the public API.
- Stay testable without requiring a live Matomo server.
- Remain easy to consume from a parent CMake project.

## Scope

The repository builds the `MatomoQt::Core` target and can optionally build the
`MatomoQt::Qml` module. The core target depends on:

- `Qt6::Core`
- `Qt6::Network`

The public API includes:

- tracking and privacy-oriented value types such as `Tracker`, `TrackerConfig`,
  `PageView`, `Event`, `ConsentState`, `PrivacyMode`, `RequestResult`,
  `ConsentStore` and `ClientIdStore`;
- `RequestBuilder` builds page view, event and ping requests for the Matomo
  Tracking HTTP API without sending them;
- `RequestBuildOptions` carries optional request context such as client ID,
  User-Agent, language and screen resolution;
- `RequestBuildResult` and `TrackingRequest` expose the build status and the
  generated URL;
- `UserAgentBuilder` can generate desktop User-Agent strings with
  DeviceDetector-compatible OS tokens.
- `NetworkDispatcher` sends built tracking URLs over HTTP with
  `QNetworkAccessManager`, timeout, circuit breaker and `rand` cache-busting;
  it never silently ignores SSL errors and never logs full tracking URLs.

`NetworkDispatcherConfig::circuitBreakerCooldownMs` defaults to `60000`: after
the circuit breaker opens, the next request after the cooldown acts as a single
recovery probe. Set it to `0` to require `resetCircuitBreaker()` instead.

`Tracker` orchestrates privacy checks, request building and asynchronous
network dispatching. Tracking calls report local acceptance synchronously,
while `dispatchFinished()` reports the eventual network result.

### Action URLs

Matomo's `url` parameter is built from `TrackerConfig::actionUrlBase` and the
path of the page view, ping or event:

| `actionUrlBase`   | Path       | `url`                    |
|-------------------|------------|--------------------------|
| `app://desktop/`  | `homePage` | `app://desktop/homePage` |
| `app://`          | `homePage` | `app://homePage`         |

A base made of a scheme only is used as a prefix and keeps the path's case.
Events report the base itself unless `Event::path` is set. Paths must be
relative: a path with its own scheme or authority is rejected.

## Non-Goals

This SDK does not aim to collect application-specific identifiers, file paths,
file names, emails, usernames or other personally identifiable business data.

Automatic QML page introspection, ecommerce tracking, heatmaps, session
recording and Matomo administration APIs are outside the initial scope.

## Build

Requirements:

- CMake 3.16 or newer
- C++20 compiler
- Qt 6 with `Core` and `Network`

Configure and build:

```sh
cmake -S . -B build -DMATOMOQT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Useful options:

```sh
-DMATOMOQT_BUILD_TESTS=ON
-DMATOMOQT_BUILD_QML=ON
-DMATOMOQT_BUILD_EXAMPLES=ON
-DMATOMOQT_INSTALL=ON
-DMATOMOQT_WARNINGS_AS_ERRORS=ON
```

## CMake Consumption

As a subdirectory:

```cmake
add_subdirectory(path/to/matomo-sdk-qt)
target_link_libraries(my_app PRIVATE MatomoQt::Core)
```

After installation:

```cmake
find_package(MatomoQt CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE MatomoQt::Core)
```

QML applications can link `MatomoQt::Qml` and import `MatomoQt` from QML.

## Minimal C++ Example

```cpp
#include <MatomoQt/PageView.h>
#include <MatomoQt/RequestBuilder.h>
#include <MatomoQt/UserAgentBuilder.h>
#include <MatomoQt/TrackerConfig.h>

#include <QtCore/QString>
#include <QtCore/QUrl>

MatomoQt::TrackerConfig config;
config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
config.actionUrlBase = QUrl(QStringLiteral("app://desktop/"));
config.siteId = 1;

MatomoQt::RequestBuilder requestBuilder(config);
const auto userAgent = MatomoQt::UserAgentBuilder::buildDesktopUserAgent(
        MatomoQt::UserAgentBuilder::currentDesktopInfo(
                QStringLiteral("ExampleApp"),
                QStringLiteral("1.0.0")));

const auto requestResult = requestBuilder.buildPageView(
        {
                .path = QStringLiteral("preferences"),
                .actionName = QStringLiteral("Preferences"),
        },
        {
                .userAgent = userAgent,
                .language = QStringLiteral("fr-CH"),
                .screenResolution = QStringLiteral("1920x1080"),
        });

if (requestResult.accepted()) {
    const QUrl trackingUrl = requestResult.request.url;
    // The host application decides when and how to send the URL.
}
```

This builds the tracking URL only.

## QML Example

A runnable QML example lives in `examples/qml-basic`. It shows a minimal
`MatomoTracker` singleton wired to consent buttons and tracking calls; no
tracking is sent before the user explicitly grants consent.

```sh
cmake -S . -B build -DMATOMOQT_BUILD_QML=ON -DMATOMOQT_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/qml-basic/matomoqt-example-qml-basic
```

By default the example points at `http://127.0.0.1:8080/matomo.php`; edit
`examples/qml-basic/qml/Main.qml` to point it at your own Matomo instance
(e.g. a local Matomo `docker-compose.yml` for testing,
or a real server) to see tracked hits.

`MatomoTracker` is exposed as a QML singleton, so you configure and call it
directly by type name rather than instantiating it. The `PrivacyMode`,
`ConsentState`, `RequestStatus` and `DispatchStatus` enums are exported as QML
namespaces. Tracking methods report local acceptance immediately; use
`MatomoTracker.dispatchFinished` for the eventual network result.

```qml
import QtQuick
import QtQuick.Controls
import MatomoQt

Button {
    text: qsTr("Grant consent and track")

    Component.onCompleted: {
        // MatomoTracker is a singleton — configure it directly.
        MatomoTracker.endpoint = "https://matomo.example.com/matomo.php"
        MatomoTracker.actionUrlBase = "app://my-app/"
        MatomoTracker.siteId = 1
        MatomoTracker.privacyMode = PrivacyMode.RequiresConsent
    }

    onClicked: {
        // Only after explicit user consent:
        MatomoTracker.grantConsent()
        MatomoTracker.trackPageView("/settings", "Settings")
        // The optional last argument reports the event under app://my-app/settings.
        MatomoTracker.trackEvent("preferences", "click", "saveButton", 1, "settings")
    }
}
```

## Privacy Model

The SDK is intended to provide GDPR and Swiss FADP/nLPD-oriented,
privacy-ready primitives, not to make a host application compliant by itself.
It should help applications make tracking explicit, consent-aware and testable.

The SDK should support:

- no tracking before explicit configuration;
- explicit consent and opt-out states;
- resettable client identifiers;
- minimal data collection by default;
- deterministic tests for privacy-sensitive behavior.

### Data Sent

Each request carries only the following Matomo Tracking HTTP API parameters:

| Parameter | Content | When |
|---|---|---|
| `idsite`, `rec`, `apiv` | Site ID and protocol flags | Always |
| `url` | Action URL, see [Action URLs](#action-urls) | Always |
| `_id` | Random 16-hex client ID from the `ClientIdStore`, cleared on denial or withdrawal | Always |
| `lang` | Name of the default `QLocale`, e.g. `fr_FR` | Always |
| `ua` | `TrackerConfig::userAgent` | When configured |
| `res` | `TrackerConfig::screenResolution` | When configured |
| `action_name` | Page view action name | Page views, when set |
| `ca`, `e_c`, `e_a`, `e_n`, `e_v` | Event category, action, name and value | Events |
| `dimensionN` | Custom dimensions from the config and the call | When set; not on pings |
| `pv_id` | Random ID of the current page view | Page views, then events and pings that follow one |
| `new_visit` | `1` after `forceNewVisit()` | Once, when requested |
| `ping` | `1` | Pings |
| `rand` | Random cache-buster | Always |

The SDK never adds file paths, account identifiers or other application data
on its own; anything else comes from the values the host passes in.

The host application remains responsible for its legal basis, user notice,
consent or opt-out UX, Matomo server settings, retention policy and data subject
workflows. Server-side Matomo configuration, such as IP anonymization, retention
rules and visitor profile settings, is also part of the compliance story.

## Roadmap

Possible future work, without commitment:

- An optional, bounded offline queue for requests sent while offline.
- A Conan package.

## License

Apache License 2.0. See [LICENSE](LICENSE).

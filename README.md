# Matomo Qt SDK

[![CI](https://github.com/Infomaniak/matomo-sdk-qt/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/Infomaniak/matomo-sdk-qt/actions/workflows/ci.yml?query=branch%3Adevelop)
[![Qt 6](https://img.shields.io/badge/Qt-6.x-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-supported-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![Privacy](https://img.shields.io/badge/privacy-GDPR%20%2B%20Swiss%20FADP%2FnLPD-2F855A)](#privacy-model)
[![Status](https://img.shields.io/badge/status-early%20development-F59E0B)](#current-scope)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)

C++ Qt SDK for the Matomo Tracking HTTP API.

> Status: early development. The current codebase provides the C++ core target
> and public API skeleton. Tracking calls are validated locally, but production
> request building, network dispatching, QML support, examples and packaging are
> still in progress.

## Project Context

This project was started to support Matomo integration in the kDrive desktop
application on Linux.

The SDK itself is not kDrive-specific. It is designed to stay reusable by any
Qt or QML application that needs to send tracking data to a Matomo instance.
Product-specific event names, routes, identifiers and business metadata should
be defined by the host application, not by this library.

## Goals

- Provide a Qt-native SDK for Matomo tracking.
- Support C++ applications first, with QML support planned.
- Keep the core library independent from any product-specific taxonomy.
- Make privacy, consent, GDPR and Swiss FADP/nLPD-oriented integration explicit
  in the public API.
- Stay testable without requiring a live Matomo server.
- Remain easy to consume from a parent CMake project.

## Current Scope

The repository currently builds the `MatomoQt::Core` target. It depends on:

- `Qt6::Core`
- `Qt6::Network`

The public API includes the initial tracking and privacy-oriented value types,
including `Tracker`, `TrackerConfig`, `PageView`, `Event`, `ConsentState`,
`PrivacyMode`, `RequestResult`, `ConsentStore` and `ClientIdStore`.

The current `Tracker` implementation validates local state only. It does not
send HTTP requests yet.

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

Some options are already available before the corresponding feature is complete.

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

## Minimal C++ Example

```cpp
#include <MatomoQt/Event.h>
#include <MatomoQt/PageView.h>
#include <MatomoQt/Tracker.h>
#include <MatomoQt/TrackerConfig.h>

#include <QtCore/QString>
#include <QtCore/QUrl>

MatomoQt::TrackerConfig config;
config.endpoint = QUrl(QStringLiteral("https://matomo.example.com/matomo.php"));
config.siteId = 1;

MatomoQt::Tracker tracker(config);
tracker.setConsentState(MatomoQt::ConsentState::Granted);

const auto pageViewResult = tracker.trackPageView({
        .path = QStringLiteral("preferences"),
        .actionName = QStringLiteral("Preferences"),
});

const auto eventResult = tracker.trackEvent({
        .category = QStringLiteral("preferences"),
        .action = QStringLiteral("click"),
        .name = QStringLiteral("saveButton"),
});
```

At the current development stage, this validates the calls locally. Network
request generation and dispatch are planned as part of the core implementation.

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

The host application remains responsible for its legal basis, user notice,
consent or opt-out UX, Matomo server settings, retention policy and data subject
workflows. Server-side Matomo configuration, such as IP anonymization, retention
rules and visitor profile settings, is also part of the compliance story.

## Roadmap

The next development areas are expected to focus on:

- Matomo request generation for page views, events and pings.
- Consent and opt-out behavior in the tracker flow.
- Network dispatch with deterministic tests.
- Manual QML tracking APIs.
- Small C++ and QML examples.
- Install-tree validation and package consumption.

This roadmap is intentionally high level and may change as the public API
stabilizes.

## License

Apache License 2.0. See [LICENSE](LICENSE).

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-11-13

### Added

- `UserAgentBuilder::buildClientHints()` builds User-Agent Client Hints with the
  OS platform and version, for Linux and macOS.
- `TrackerConfig::clientHints` and `RequestBuildOptions::clientHints` send them
  as Matomo's `uadata` parameter, so Matomo reports the version of a Linux
  distribution, e.g. "Ubuntu 26.04" instead of "Ubuntu". A value that is not a JSON object rejects
  the request with `RequestInvalidPayload`.

### Changed

- On Linux, `UserAgentBuilder::currentDesktopInfo()` fills
  `operatingSystemVersion` with the distribution's `VERSION_ID` from
  `/etc/os-release`. The User-Agent string is unchanged.

## [1.0.0] - 2026-11-12

First stable release. The public C++ and QML APIs now follow Semantic Versioning.

### Added

- `MatomoQt::Core`: `Tracker` with page views, events, pings, custom dimensions,
  consent-aware privacy modes and persistent consent and client ID stores
  (in-memory and `QSettings`).
- `RequestBuilder` to build Matomo Tracking HTTP API requests without sending them.
- `NetworkDispatcher` with timeout, circuit breaker, `rand` cache-busting and
  strict SSL handling.
- `UserAgentBuilder` for desktop User-Agent strings recognized by Matomo's
  DeviceDetector, including the Linux distribution.
- `MatomoQt::Qml`: `MatomoTracker` QML singleton and exported enums.
- `NetworkDispatcherConfig::circuitBreakerCooldownMs` controls automatic recovery
  of the circuit breaker; set it to `0` to require a manual reset.
- `Event::path`, and an optional `path` argument to the QML `trackEvent()`, to
  report an event under its own action URL.
- `TrackerConfig::screenResolution` to send Matomo's `res` parameter.

### Changed

- The circuit breaker allows one recovery probe after a 60-second cooldown by
  default, closing on success or waiting another cooldown on failure.

### Fixed

- An action URL base made of a scheme only, such as `app://`, no longer produces
  requests with an empty `url`; the path is appended with its case preserved.
- A request aborted from outside the dispatcher now reports a result and no
  longer keeps the circuit breaker open forever.

[Unreleased]: https://github.com/Infomaniak/matomo-sdk-qt/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/Infomaniak/matomo-sdk-qt/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/Infomaniak/matomo-sdk-qt/releases/tag/v1.0.0

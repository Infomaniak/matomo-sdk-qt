#pragma once

namespace MatomoQt {

/** User consent state known by the tracker. */
enum class ConsentState {
    Unknown,
    Granted,
    Denied,
    Withdrawn,
};

} // namespace MatomoQt

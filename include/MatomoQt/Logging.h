#pragma once

#include <MatomoQt/Export.h>

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(matomoSdk)

namespace MatomoQt {

/**
 * SDK-wide logging category.
 *
 * Use this category for all SDK diagnostic messages.  Full tracking URLs and
 * query parameters must never be logged.
 */
MATOMOQT_CORE_EXPORT const QLoggingCategory &matomoSdkCategory();

} // namespace MatomoQt

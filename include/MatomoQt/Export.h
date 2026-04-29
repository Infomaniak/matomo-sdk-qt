#pragma once

#include <QtCore/QtGlobal>

#if defined(MATOMOQT_CORE_STATIC)
#define MATOMOQT_CORE_EXPORT
#elif defined(MATOMOQT_CORE_LIBRARY)
#define MATOMOQT_CORE_EXPORT Q_DECL_EXPORT
#else
#define MATOMOQT_CORE_EXPORT Q_DECL_IMPORT
#endif

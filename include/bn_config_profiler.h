/*
 * Project-level Butano profiler configuration.
 *
 * Kept in sync with the room-viewer profiler menu settings.
 */

#ifndef BN_CONFIG_PROFILER_H
#define BN_CONFIG_PROFILER_H

#include "bn_common.h"

#ifndef BN_CFG_PROFILER_ENABLED
    #define BN_CFG_PROFILER_ENABLED true
#endif

#ifndef BN_CFG_PROFILER_LOG_ENGINE
    #define BN_CFG_PROFILER_LOG_ENGINE false
#endif

#ifndef BN_CFG_PROFILER_LOG_ENGINE_DETAILED
    #define BN_CFG_PROFILER_LOG_ENGINE_DETAILED false
#endif

#ifndef BN_CFG_PROFILER_MAX_ENTRIES
    #define BN_CFG_PROFILER_MAX_ENTRIES 16
#endif

#endif

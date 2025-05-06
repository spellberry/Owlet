#pragma once

#if defined(BEE_PLATFORM_PC)
#include <platform/pc/core/device_pc.hpp>
#elif defined(BEE_PLATFORM_PROSPERO)
#include <platform/prospero/core/device_prospero.hpp>
#endif

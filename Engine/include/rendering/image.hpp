#pragma once

#if defined(BEE_PLATFORM_PC)
#include "platform/dx12/image_dx12.h"
#elif defined(BEE_PLATFORM_PROSPERO)
#include "platform/prospero/rendering/image_prospero.hpp"
#endif
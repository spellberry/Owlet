#pragma once

#if defined(BEE_PLATFORM_PC)
#include <platform/dx12/ui_render_data_dx12.hpp>
#elif defined(BEE_PLATFORM_PROSPERO)
#include <platform/prospero/rendering/ui_render_data_prospero.hpp>
#endif

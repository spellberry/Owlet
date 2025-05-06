#pragma once


#if defined(BEE_PLATFORM_PC)
//#if defined(OPEN_GL)
//#include <platform/opengl/ui_renderer_gl.hpp>
//#elif defined(DX12)
#include <platform/dx12/ui_renderer_dx12.hpp>
///#endif
#elif defined(BEE_PLATFORM_PROSPERO)
#include <platform/prospero/rendering/ui_renderer_prospero.hpp>
#endif

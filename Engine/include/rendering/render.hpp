#pragma once

#if defined(BEE_PLATFORM_PC)
#include <platform/opengl/render_gl.hpp>
#elif defined(BEE_PLATFORM_PROSPERO)
#include <platform/prospero/rendering/render_prospero.hpp>
#endif

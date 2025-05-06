#pragma once

#if defined(BEE_PLATFORM_PC)
#include <platform/dx12/mesh_dx12.h>//#include <platform/opengl/mesh_gl.hpp>
#elif defined(BEE_PLATFORM_PROSPERO)
#include <platform/prospero/rendering/mesh_prospero.hpp>
#endif

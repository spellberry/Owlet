#include "rendering/debug_render.hpp"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "tools/log.hpp"
#include "core/transform.hpp"
#include "rendering/render_components.hpp"
#include "core/engine.hpp"
#include "core/ecs.hpp"
#include "platform/opengl/shader_gl.hpp"
#include "platform/opengl/open_gl.hpp"
#include <vector>

using namespace bee;
using namespace glm;

 std::vector<Line> DebugRenderer::m_line_array;

class bee::DebugRenderer::Impl
{
public:
    Impl();
	bool AddLine(const vec3& from, const vec3& to, const vec4& color);
    bool AddLine(const vec2& from, const vec2& to, const vec4& color);
	void Render(const mat4& view, const mat4& projection);

    static int const				m_maxLines = 32760;
    int								m_linesCount = 0;
    struct VertexPosition3DColor { glm::vec3 Position; glm::vec4 Color; };
    VertexPosition3DColor*			m_vertexArray = nullptr;
    unsigned int					debug_program = 0;
    unsigned int					m_linesVAO = 0;
    unsigned int					m_linesVBO = 0;
};


bee::DebugRenderer::DebugRenderer()
{
    m_categoryFlags =
		DebugCategory::General |
		DebugCategory::Gameplay |
		DebugCategory::Physics |
		DebugCategory::Rendering |
                DebugCategory::AINavigation |
		DebugCategory::AIDecision |
		DebugCategory::Editor;


    if (!bee::Engine.IsHeadless())
    {
        m_impl = std::make_unique<Impl>();
    }
}

DebugRenderer::~DebugRenderer()
{
	// TODO: Cleanup!
}

void DebugRenderer::Render()
{
	for (const auto& [entity, transform, camera] : Engine.ECS().Registry.view<Transform, Camera>().each())
    {
        // Get the view and projection matrices from the camera
        mat4 view = inverse(transform.World());
        mat4 projection = camera.Projection;
        m_impl->Render(view, projection);
	}
}

void DebugRenderer::AddLine(
	DebugCategory::Enum category,
	const vec3& from,
	const vec3& to,
	const vec4& color)
{
    if (!(m_categoryFlags & category)) return;
	m_impl->AddLine(from, to, color);
}
void DebugRenderer::AddLine2D(DebugCategory::Enum category, const vec2& from, const vec2& to, const vec4& color)
{
        if (!(m_categoryFlags & category)) return;
        m_impl->AddLine(from, to, color);
}


bee::DebugRenderer::Impl::Impl()
{
	m_vertexArray = new VertexPosition3DColor[m_maxLines * 2];

	const auto* const vsSource =
		"#version 460 core												\n\
		layout (location = 1) in vec3 a_position;						\n\
		layout (location = 2) in vec4 a_color;							\n\
		layout (location = 1) uniform mat4 u_worldviewproj;				\n\
		out vec4 v_color;												\n\
																		\n\
		void main()														\n\
		{																\n\
			v_color = a_color;											\n\
			gl_Position = u_worldviewproj * vec4(a_position, 1.0);		\n\
		}";

	const auto* const fsSource =
		"#version 460 core												\n\
		in vec4 v_color;												\n\
		out vec4 frag_color;											\n\
																		\n\
		void main()														\n\
		{																\n\
			frag_color = v_color;										\n\
		}";

	GLuint vertShader = 0;
	GLuint fragShader = 0;
	GLboolean res = GL_FALSE;

	//debug_program = glCreateProgram();
	//LabelGL(GL_PROGRAM, debug_program, "Debug Renderer Program");

//	res = Shader::CompileShader(&vertShader, GL_VERTEX_SHADER, vsSource);
//	if (!res)
//	{
//		Log::Error("DebugRenderer failed to compile vertex shader");
//		return;
//	}
//
////	res = Shader::CompileShader(&fragShader, GL_FRAGMENT_SHADER, fsSource);
//	if (!res)
//	{
//		Log::Error("DebugRenderer failed to compile fragment shader");
//		return;
//	}

	//glAttachShader(debug_program, vertShader);
	//glAttachShader(debug_program, fragShader);

	/*if (!Shader::LinkProgram(debug_program))
	{
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		glDeleteProgram(debug_program);
		Log::Error("DebugRenderer failed to link shader program");
		return;
	}*/

	//glDeleteShader(vertShader);
	//glDeleteShader(fragShader);

	//glCreateVertexArrays(1, &m_linesVAO);
	//glBindVertexArray(m_linesVAO);
	//LabelGL(GL_VERTEX_ARRAY, m_linesVAO, "Debug Lines VAO");

	//// Allocate VBO
	//glGenBuffers(1, &m_linesVBO);	
	//glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);
	//LabelGL(GL_BUFFER, m_linesVBO, "Debug Lines VBO");

	//// Allocate into VBO
	//const auto size = sizeof(m_vertexArray);
	//glBufferData(GL_ARRAY_BUFFER, size, &m_vertexArray[0], GL_STREAM_DRAW);

	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(
	//	1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPosition3DColor),
	//	reinterpret_cast<void*>(offsetof(VertexPosition3DColor, Position)));

	//glEnableVertexAttribArray(2);
	//glVertexAttribPointer(
	//	2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPosition3DColor),
	//	reinterpret_cast<void*>(offsetof(VertexPosition3DColor, Color)));

	//glBindVertexArray(0); // TODO: Only do this when validating OpenGL

}

bool bee::DebugRenderer::Impl::AddLine(const vec3& from, const vec3& to, const vec4& color)
{
	if (m_linesCount < m_maxLines)
	{
		 Line line;
        line.color = color;
        line.p1 = from;
        line.p2 = to;
        m_line_array.push_back(line);

		++m_linesCount;
		return true;
	}
	return false;
}
bool bee::DebugRenderer::Impl::AddLine(const vec2& from, const vec2& to, const vec4& color)
{
        if (m_linesCount < m_maxLines)
        {
                Line line;
                line.color = color;
                line.p1 = vec3(from,0);
                line.p2 = vec3(to, 0);
                line.is2D = true;
                m_line_array.push_back(line);

                ++m_linesCount;
                return true;
        }
        return false;
}
void bee::DebugRenderer::Impl::Render(const mat4& view, const mat4& projection)
{
	// Render debug lines
	glm::mat4 vp = projection * view;
	//glUseProgram(debug_program);
	//glUniformMatrix4fv(1, 1, false, value_ptr(vp));
	//glBindVertexArray(m_linesVAO);

	//if (m_linesCount > 0)
	//{
	//	glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);
	//	glBufferData(
	//		GL_ARRAY_BUFFER,
	//		sizeof(VertexPosition3DColor) * (m_maxLines * 2),
	//		&m_vertexArray[0],
	//		GL_DYNAMIC_DRAW);
	//	glDrawArrays(GL_LINES, 0, m_linesCount * 2);
	//	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//}

	m_linesCount = 0;
}

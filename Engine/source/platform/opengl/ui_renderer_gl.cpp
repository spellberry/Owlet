#include "platform/opengl/ui_renderer_gl.hpp"

#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/transform.hpp"
#include "platform/opengl/open_gl.hpp"
#include "platform/opengl/ui_render_data_gl.hpp"
#include "rendering/render.hpp"
#include "tools/inspector.hpp"
#include "tools/log.hpp"
#include "user_interface/font_handler.hpp"
#include "user_interface/user_interface.hpp"

using namespace bee;
using namespace ui;
void FontRenderer::GenFont(Font& font, std::vector<float>& fim)
{
    glGenTextures(1, &font.tex.texture);
    glBindTexture(GL_TEXTURE_2D, font.tex.texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, font.width, font.height, 0, GL_RGB, GL_FLOAT, (void*)&fim.at(0));
    LabelGL(GL_TEXTURE, font.tex.texture, "TEX | font | " + font.name);
}

UIRenderer::UIRenderer()
{
    std::string error;
    GLuint textVertShader = LoadShader("font", ShaderType::vertex, error);
    GLuint textFragShader = LoadShader("font", ShaderType::fragment, error);

    m_textShaderProgram = glCreateProgram();
    glAttachShader(m_textShaderProgram, textVertShader);
    glAttachShader(m_textShaderProgram, textFragShader);
    glLinkProgram(m_textShaderProgram);

    GLint success = 0;
    glGetProgramiv(m_textShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        GLsizei length = 0;
        glGetProgramInfoLog(m_textShaderProgram, sizeof(log), &length, log);
        glDeleteProgram(m_textShaderProgram);
        std::string err = log;
        Log::Error(err);
        return;
    }
    glUseProgram(m_textShaderProgram);

    glUniform4f(glGetUniformLocation(m_textShaderProgram, "fgColor"), (GLfloat)1.0f, (GLfloat)1.0f, (GLfloat)1.0f,
                (GLfloat)1.0f);
    glUniform1f(glGetUniformLocation(m_textShaderProgram, "pxRange"), (GLfloat)1.0f);
    glUniform1i(glGetUniformLocation(m_textShaderProgram, "msdf"), 0);
    glUniform1i(glGetUniformLocation(m_textShaderProgram, "background"), 1);

    glm::mat4 trans = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_textShaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(trans));
    textprojection = glGetUniformLocation(m_textShaderProgram, "projection");
    texttrans = glGetUniformLocation(m_textShaderProgram, "transform");
    textop = glGetUniformLocation(m_textShaderProgram, "opacity");
    LabelGL(GL_PROGRAM, m_textShaderProgram, "Shader | text");

    //
    GLuint imgVertShader = LoadShader("img", ShaderType::vertex, error);
    GLuint imgFragShader = LoadShader("img", ShaderType::fragment, error);

    m_imgShaderProgram = glCreateProgram();
    glAttachShader(m_imgShaderProgram, imgVertShader);
    glAttachShader(m_imgShaderProgram, imgFragShader);
    glLinkProgram(m_imgShaderProgram);

    success = 0;
    glGetProgramiv(m_imgShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        GLsizei length = 0;
        glGetProgramInfoLog(m_imgShaderProgram, sizeof(log), &length, log);
        glDeleteProgram(m_imgShaderProgram);
        std::string err = log;
        Log::Error(err);
        return;
    }
    glUseProgram(m_imgShaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(m_imgShaderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(trans));
    imgprojection = glGetUniformLocation(m_imgShaderProgram, "projection");
    imgtrans = glGetUniformLocation(m_imgShaderProgram, "transform");
    imgop = glGetUniformLocation(m_imgShaderProgram, "opacity");
    LabelGL(GL_PROGRAM, m_imgShaderProgram, "Shader | Image");

    //
    GLuint proVertShader = LoadShader("ProgressBar", ShaderType::vertex, error);
    GLuint proFragShader = LoadShader("Progressbar", ShaderType::fragment, error);

    m_progressBarProgram = glCreateProgram();
    glAttachShader(m_progressBarProgram, proVertShader);
    glAttachShader(m_progressBarProgram, proFragShader);
    glLinkProgram(m_progressBarProgram);

    success = 0;
    glGetProgramiv(m_progressBarProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        GLsizei length = 0;
        glGetProgramInfoLog(m_progressBarProgram, sizeof(log), &length, log);
        glDeleteProgram(m_progressBarProgram);
        std::string err = log;
        Log::Error(err);
        return;
    }
    glUseProgram(m_progressBarProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_progressBarProgram, "transform"), 1, GL_FALSE, glm::value_ptr(trans));
    pbprojection = glGetUniformLocation(m_progressBarProgram, "projection");
    pbtrans = glGetUniformLocation(m_progressBarProgram, "transform");
    pbop = glGetUniformLocation(m_progressBarProgram, "opacity");
    LabelGL(GL_PROGRAM, m_progressBarProgram, "Shader | progressbar");

    int height = Engine.Device().GetHeight();
    int width = Engine.Device().GetWidth();
    glm::mat4 projection = glm::ortho(0.0f, (float)width / (float)height, 1.0f, 0.0f, 10.0f, -10.0f);
    // glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    glUseProgram(m_imgShaderProgram);
    glUniformMatrix4fv(imgprojection, 1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(m_progressBarProgram);
    glUniformMatrix4fv(pbprojection, 1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(m_textShaderProgram);
    glUniformMatrix4fv(textprojection, 1, GL_FALSE, glm::value_ptr(projection));
}
UIRenderer ::~UIRenderer() {}

std::string UIRenderer::readFile(const std::string& filename, std::string& error)
{
    std::ifstream stream(filename, std::ios::binary);
    if (!stream)
    {
        error = "failed to open: " + filename + "\n";
        return "";
    }

    stream.seekg(0, std::istream::end);
    size_t size = stream.tellg();
    stream.seekg(0, std::istream::beg);

    std::string result = std::string(size, 0);
    stream.read(&result[0], size);
    if (!stream)
    {
        error = "failed to read: " + filename + "\n";
        return "";
    }

    return result;
}

unsigned int UIRenderer::LoadShader(std::string name, ShaderType shaderType, std::string& error)
{
    std::string shaderPath = "";

    GLuint shader = 0;
    switch (shaderType)
    {
        case ShaderType::vertex:
        {
            shaderPath = "/shaders/" + name + ".vert";
            shader = glCreateShader(GL_VERTEX_SHADER);
            break;
        }
        case ShaderType::fragment:
        {
            shaderPath = "/shaders/" + name + ".frag";
            shader = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        }
    }
    std::string shaderData = readFile(Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, shaderPath), error);

    if (error != "")
    {
        Log::Error(error);
        return 0;
    }
    const char* shaderSource = shaderData.c_str();
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        GLsizei length = 0;
        glGetShaderInfoLog(shader, sizeof(log), &length, log);
        switch (shaderType)
        {
            case ShaderType::vertex:
            {
                Log::Error("error in loading " + name + "vertex " + "shader\n");
                break;
            }
            case ShaderType::fragment:
            {
                Log::Error("error in loading " + name + "fragment " + "shader\n");
                break;
            }
                return 0;
        }
    }
    return shader;
}

void UIRenderer::Render()
{
    auto view = Engine.ECS().Registry.view<UIElement, Transform>();

    auto& renderer = Engine.ECS().GetSystem<Renderer>();
    auto& UI = Engine.ECS().GetSystem<UserInterface>();

#ifdef BEE_INSPECTOR
    if (Engine.Inspector().GetVisible())
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer.m_finalFramebuffer);
    }
    else
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    //   glClear(GL_DEPTH_BUFFER_BIT);
#else
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glSampleCoverage(1.0, false);

    glActiveTexture(GL_TEXTURE0);

    for (auto [ent, element, trans] : view.each())
    {
        if (element.drawing)
        {
            auto transmat = trans.World();
            glUseProgram(m_imgShaderProgram);
            glUniformMatrix4fv(imgtrans, 1, GL_FALSE, glm::value_ptr(transmat));
            glUniform1f(imgop, element.opacity);
            for (auto& imgcomp : element.imageComponents)
            {
                glBindVertexArray(imgcomp.second.renderData.VAO);
                glBindTexture(GL_TEXTURE_2D, images.at(imgcomp.first).tex.texture);
                glDrawElements(GL_TRIANGLES, (GLsizei)imgcomp.second.indices.size(), GL_UNSIGNED_INT, 0);
#ifdef BEE_INSPECTOR
                drawCalls++;
#endif  // BEE_INSPECTOR
            }

            glUseProgram(m_progressBarProgram);
            glUniformMatrix4fv(pbtrans, 1, GL_FALSE, glm::value_ptr(transmat));
            glUniform1f(pbop, element.opacity);
            for (auto& barComp : element.progressBarimageComponents)
            {
                glBindVertexArray(barComp.second.renderData.VAO);
                glBindTexture(GL_TEXTURE_2D, images.at(barComp.first).tex.texture);
                glDrawElements(GL_TRIANGLES, (GLsizei)barComp.second.indices.size(), GL_UNSIGNED_INT, 0);
#ifdef BEE_INSPECTOR
                drawCalls++;
#endif  // BEE_INSPECTOR
            }

            glUseProgram(m_textShaderProgram);
            glBindVertexArray(element.textRenderData.VAO);
            glUniformMatrix4fv(texttrans, 1, GL_FALSE, glm::value_ptr(transmat));
            glUniform1f(textop, element.opacity);
            glBindTexture(GL_TEXTURE_2D, UI.fontHandler().GetFont(element.usedFont).tex.texture);
            glDrawElements(GL_TRIANGLES, (GLsizei)element.textindices.size(), GL_UNSIGNED_INT, 0);
#ifdef BEE_INSPECTOR
            drawCalls++;
#endif  // BEE_INSPECTOR
        }
    }
    // if (UI.selected)
    //{
    //     glUseProgram(m_imgShaderProgram);
    //     glBindTexture(GL_TEXTURE_2D, images.at(UI.selectedComponentOverlayImage).tex.texture);
    //     glm::mat4 trans = Engine.ECS().Registry.get<Transform>(UI.UIEntities.find(UI.CselectedElement)->second).World();
    //     glUniformMatrix4fv(imgtrans, 1, GL_FALSE, glm::value_ptr(trans));
    //     glBindVertexArray(UI.selectedComponentOverlay.renderData.VAO);
    //     glDrawElements(GL_TRIANGLES, (GLsizei)UI.selectedComponentOverlay.indices.size(), GL_UNSIGNED_INT, 0);
    // }
    glDisable(GL_BLEND);
}

void UIRenderer::ReplaceBuffers(RendererData& rData, size_t verticesSize, size_t indicesSize, float* VBOstart,
                                unsigned int* EBOstart, int VBOoffset, int EBOoffset)
{
    glNamedBufferSubData(rData.VBO, sizeof(float) * VBOoffset, sizeof(float) * verticesSize, VBOstart);
    glNamedBufferSubData(rData.EBO, sizeof(unsigned int) * EBOoffset, sizeof(unsigned int) * indicesSize, EBOstart);
}

void UIRenderer::ClearBuffers(UIElement& element) {}

void UIRenderer::GenElement(UIElement& element)
{  //
    // text
    //
    glGenVertexArrays(1, &element.textRenderData.VAO);
    glGenBuffers(1, &element.textRenderData.VBO);
    glGenBuffers(1, &element.textRenderData.EBO);

    glBindVertexArray(element.textRenderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, element.textRenderData.VBO);
    glBufferData(GL_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element.textRenderData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    LabelGL(GL_VERTEX_ARRAY, element.textRenderData.VAO, "VAO | text | element: " + std::to_string((int)element.ID));
    LabelGL(GL_BUFFER, element.textRenderData.VBO, "VBO | text | element: " + std::to_string((int)element.ID));
    LabelGL(GL_BUFFER, element.textRenderData.EBO, "EBO | text | element: " + std::to_string((int)element.ID));
}

void UIRenderer::GenTexture(UIImage& img, int c, unsigned char* imgData, std::string& name)
{
    glGenTextures(1, &img.tex.texture);
    glBindTexture(GL_TEXTURE_2D, img.tex.texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (c == 3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, imgData);
    }
    else if (c == 4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
    }
    glGenerateTextureMipmap(img.tex.texture);
    LabelGL(GL_TEXTURE, img.tex.texture, "TEX | " + name);
}

void UIRenderer::DelTexture(int image)
{
    glDeleteTextures(1, &images.at(image).tex.texture);
    images.erase(images.begin() + image);
}

void UIRenderer::ReplaceImg(UIElement& element, UIComponent& compo, std::vector<float>& data)
{
    glBindVertexArray(element.imageComponents.find(compo.image)->second.renderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, element.imageComponents.find(compo.image)->second.renderData.VBO);
    glNamedBufferSubData(element.imageComponents.find(compo.image)->second.renderData.VBO, compo.ID * sizeof(float),
                         sizeof(float) * data.size(), &data[0]);
}

void UIRenderer::UpdateProgressBarValue(UIElement& uiel, UIComponent& compo, float newValue)
{
    glBindVertexArray(uiel.progressBarimageComponents.find(compo.image)->second.renderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, uiel.progressBarimageComponents.find(compo.image)->second.renderData.VBO);
    for (int i = 7; i < 64; i += 16)
    {
        glNamedBufferSubData(uiel.progressBarimageComponents.find(compo.image)->second.renderData.VBO,
                             (compo.ID + i) * sizeof(float), sizeof(float) * 1, &newValue);
    }
}

void UIRenderer::UpdateProgressBarColors(UIElement& uiel, UIComponent& compo)
{
    glBindVertexArray(uiel.progressBarimageComponents.find(compo.image)->second.renderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, uiel.progressBarimageComponents.find(compo.image)->second.renderData.VBO);

    for (int i = 8; i < 64; i += 16)
    {
        glNamedBufferSubData(uiel.progressBarimageComponents.find(compo.image)->second.renderData.VBO,
                             (compo.ID + i) * sizeof(float), sizeof(float) * 8,
                             &uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i));
    }
}

void UIRenderer::genImageComp(ImgType type, UIImageComponent& imgcomp, std::string& name)
{
    glGenVertexArrays(1, &imgcomp.renderData.VAO);
    glGenBuffers(1, &imgcomp.renderData.VBO);
    glGenBuffers(1, &imgcomp.renderData.EBO);

    glBindVertexArray(imgcomp.renderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, imgcomp.renderData.VBO);
    glBufferData(GL_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imgcomp.renderData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    switch (type)
    {
        case ImgType::rawImage:
        {
            // position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            // texture coord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            break;
        }
        case ImgType::ProgressBarBackground:
        {
            // position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            // texture coord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            // start and value
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(5 * sizeof(float)));
            glEnableVertexAttribArray(2);
            // bColor
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(8 * sizeof(float)));
            glEnableVertexAttribArray(3);
            // fColor
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(12 * sizeof(float)));
            glEnableVertexAttribArray(4);
            break;
        }
    }
    LabelGL(GL_VERTEX_ARRAY, imgcomp.renderData.VAO, "VAO | img | element: " + name);
    LabelGL(GL_BUFFER, imgcomp.renderData.VBO, "VBO | img | element: " + name);
    LabelGL(GL_BUFFER, imgcomp.renderData.EBO, "EBO | img | element: " + name);
}

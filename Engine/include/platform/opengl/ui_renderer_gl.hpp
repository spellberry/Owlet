#pragma once

#include <string>

#include "rendering/ui_render_data.hpp"
#include "user_interface/user_interface_structs.hpp"
constexpr int BUFFER_SIZE = 40000;

namespace bee
{
namespace ui
{

class FontRenderer
{
public:
    void GenFont(Font& font, std::vector<float>& fim);
};
class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    void Render();
    void ClearBuffers(UIElement& element);

    void GenElement(UIElement& element);
    void GenTexture(UIImage& img, int c, unsigned char* imgData, std::string& name);
    void ReplaceImg(UIElement& element, UIComponent& compo, std::vector<float>& data);
    void UpdateProgressBarValue(UIElement& uiel, UIComponent& compo, float newValue);
    void UpdateProgressBarColors(UIElement& uiel, UIComponent& compo);
    void genImageComp(ImgType type, UIImageComponent& imgcomp, std::string& name);
    void DelTexture(int image);
    std::vector<UIImage> images;
    void ReplaceBuffers(RendererData& rData, size_t verticesSize, size_t indicesSize, float* VBOstart, unsigned int* EBOstart,
                        int VBOoffset, int EBOoffset);

private:
    friend class UserInterface;
    enum ShaderType
    {
        vertex = 1,
        fragment = 2
    };
    // shader loading
    std::string readFile(const std::string& filename, std::string& error);
    unsigned int LoadShader(std::string name, ShaderType shaderType, std::string& error);

    unsigned int m_textShaderProgram = 0;
    unsigned int m_imgShaderProgram = 0;
    unsigned int m_progressBarProgram = 0;

    unsigned int imgprojection = 0;
    unsigned int imgtrans = 0;
    unsigned int imgop = 0;
    unsigned int pbprojection = 0;
    unsigned int pbtrans = 0;
    unsigned int pbop = 0;
    unsigned int textprojection = 0;
    unsigned int texttrans = 0;
    unsigned int textop = 0;

#ifdef BEE_INSPECTOR
    int drawCalls = 0;
#endif
};
}  // namespace ui
}  // namespace bee
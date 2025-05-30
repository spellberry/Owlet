#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/fileio.hpp"
#include "core/resource.hpp"

namespace bee
{

class Shader;
struct Texture;
class Image;

///
/// ShaderParameter is a representation of an shader parameter.
/// It has a type and it will complain if the type declared in the
/// shader program is different.
///
class ShaderParameter
{
    friend class Shader;

public:
    /// Check if this parameter object is valid
    bool IsValid() const { return m_location != -1; }

    /// Gets the type of this parameter
    unsigned int GetType() const { return m_type; }

    /// Location of this parameter
    int GetLocation() const { return m_location; }

    /// Return the name of this parameter
    const std::string& GetName() const { return m_name; }

    /// Set the value. Parameter should be float.
    void SetValue(float val) const;

    /// Set the value. Parameter should be int.
    void SetValue(int val) const;

    /// Set the value. Parameter should be int.
    void SetValue(unsigned int val) const;

    /// Set the value. Parameter should be bool.
    void SetValue(bool val) const;

    /// Set the value. Parameter should be vec2.
    void SetValue(const glm::vec2& vec) const;

    /// Set the value. Parameter should be vec3.
    void SetValue(const glm::vec3& vec) const;

    /// Set the value. Parameter should be vec4.
    void SetValue(const glm::vec4& vec) const;

    /// Set the value. Parameter should be mat4
    void SetValue(const glm::mat4& mtx, bool transpose = false);

    /// Set the value. Parameter should be Texture.
    void SetValue(const Image& image);

    /// Destructor
    ~ShaderParameter() = default;

protected:
    /// The shader creates a parameter.
    ShaderParameter(Shader* shader, std::string name, unsigned int type, int location, int sampler = -1)
        : m_shader(shader), m_name(name), m_type(type), m_location(location), m_sampler(sampler)
    {
    }

    /// Constructor for an invalid ShaderParameter
    ShaderParameter() : m_shader(nullptr), m_name(""), m_type(0), m_location(-1), m_sampler(-1) {}

    /// The shader can reset the parameter after a reload
    void Reset(Shader* shader, std::string name, unsigned int type, int location, int sampler = -1)
    {
        this->m_shader = shader;
        this->m_type = type;
        this->m_location = location;
        this->m_name = name;
        this->m_sampler = sampler;
    }

    /// The shader should invalidate when reloading a new shader file
    /// a some information can be old
    void Invalidate()
    {
        m_shader = nullptr;
        m_name = "";
        m_type = 0;
        m_location = -1;
        m_sampler = -1;
    }

    /// The effect this parameter belongs to. Unused (beyond debugging)
    Shader* m_shader;

    /// Name as defined in the shader file.
    std::string m_name;

    /// Type as defined in the shader file.
    unsigned int m_type;

    /// Location for the setting for this.
    int m_location;

    /// Only valid for type sampler (GL_SAMPLER_2D)
    int m_sampler;
};

///
/// A vertex attribute
///
class ShaderAttribute
{
    friend class Shader;

public:
    /// Check if this parameter object is valid
    bool IsValid() const { return _location != -1; }

    /// Gets the type of this parameter
    unsigned int GetType() const { return _type; }

    /// Location of this parameter
    int GetLocation() const { return _location; }

    /// Check documentation for glVertexAttribPointer
    void SetAttributePointer(int size, unsigned int type, unsigned char normalized, int stride, const void* pointer);

    /// Check documentation for glVertexAttribPointer
    void DisableAttributePointer();

protected:
    /// The shader creates a parameter.
    ShaderAttribute(Shader* shader, std::string name, unsigned int type, int location)
        : _shader(shader), _name(name), _type(type), _location(location)
    {
    }

    /// Constructor for an invalid ShaderParameter
    ShaderAttribute() : _shader(nullptr), _name(""), _type(0), _location(-1) {}

    /// The shader can reset the parameter after a reload
    void Reset(Shader* shader, std::string name, unsigned type, int location)
    {
        this->_shader = shader;
        this->_type = type;
        this->_location = location;
        this->_name = name;
    }

    /// The shader should invalidate when reloading a new shader file
    /// as some information can be old
    void Invalidate()
    {
        _shader = nullptr;
        _name = "";
        _type = 0;
        _location = -1;
    }

protected:
    /// The effect this parameter belongs to. Unused (beyond debugging)
    Shader* _shader;

    /// Name as defined in the shader file.
    std::string _name;

    /// Type as defined in the shader file.
    unsigned _type;

    /// Location for the setting for this.
    int _location;
};

/// Shader is a class representing a compiled GPU program
class Shader : public Resource
{
    friend class Resources;

public:
    /// Create an empty shader. You will need to provide the source with LoadSource() before
    /// you can use the shader.
    Shader() : Resource(ResourceType::Shader) {}

    /// Create a shader with vertex and fragment programs
    Shader(FileIO::Directory directory, const std::string& vertexFilename, const std::string& fragmentFilename);

    /// Create a shader with vertex geometry and fragment programs
    Shader(FileIO::Directory directory, const std::string& vertexFilename, const std::string& geometryFilename,
           const std::string& fragmentFilename);

    Shader(FileIO::Directory directory, const std::string& computeFilename);

    Shader(const Shader& other) = delete;

    ~Shader() override;

    /// Load with source (as opposed to files)
    bool LoadSource(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader,
                    const std::string& computeShader);

    /// Get active parameter by name. If the parameter is not present/active
    /// you will get an invalid one.
    ShaderParameter* GetParameter(const std::string& name);

    /// Get active parameter by name id. If the parameter is not present/active
    /// you will get an invalid one.
    ShaderParameter* GetParameter(unsigned long long nameId);

    /// Get active attribute by name. If the attribute is not present/active
    /// you will get an invalid one.
    ShaderAttribute* GetAttribute(const std::string& name);

    /// Get the program in case you want to do everything manually
    unsigned int GetProgram() const;

    /// Call before drawing
    void Activate();

    /// Call after drawing
    void Deactivate();

    std::vector<ShaderParameter*> GetAllParameters();

    void Reload() override;

    const std::string& GetVertexFilename() const { return m_vertexFilename; }

    const std::string& GetFragmentFilename() const { return m_fragmentFilename; }

    const std::string& GetGeometryFilename() const { return m_geometryFilename; }

    void LoadParameters();

    static bool CompileShader(unsigned int* shader, unsigned int type, const char* source);
    static bool LinkProgram(unsigned int prog);
    static bool ValidateProgram(unsigned int prog);

private:
    static std::string GetPath(FileIO::Directory, const std::string& vertexFilename, const std::string& fragmentFilename);

    static std::string GetPath(FileIO::Directory, const std::string& vertexFilename, const std::string& geometryFilename,
                               const std::string& fragmentFilename);

    bool Validate();

    /// Just a helper for the constructors
    bool Load();

    /// When loading fails, load magenta shader
    bool LoadMagentaShader();

    std::string m_resourcePath;

    /// Store all the parameters
    std::unordered_map<unsigned long long, std::unique_ptr<ShaderParameter>> m_parameters;

    /// Store all the attributes
    std::unordered_map<std::string, std::unique_ptr<ShaderAttribute>> m_attributes;

    /// Relative path to the vertex shader
    std::string m_vertexFilename;

    /// Relative path to the fragment shader
    std::string m_fragmentFilename;

    /// Relative path to the geometry shader
    std::string m_geometryFilename;

    std::string m_computeFilename;

    /// GL id (name) of the compiled program
    unsigned int m_program = 0;
};

}  // namespace bee

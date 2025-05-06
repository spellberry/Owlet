#pragma once
#include <fstream>
#include <glm/glm.hpp>
#include <tinygltf/json.hpp>
#include <visit_struct/visit_struct.hpp>
#include "core/engine.hpp"
#include "core/resources.hpp"
#include "tools/serializable.hpp"


namespace bee
{



/// <summary>
/// Serilizers an object from a class to an object, provided the class has a VISITABLE_STRUCT macro
/// defined for its members
/// </summary>
class ToJsonSerializer
{
public:
    ToJsonSerializer(nlohmann::json& j) : m_json(j) {}

    template <typename T>
    void operator()(const char* name, const T& value)
    {
        SerializeTo(m_json, name, value);
    }

    nlohmann::json& Json() { return m_json; };

private:
    template <class T>
    static inline void SerializeTo(nlohmann::json& jsn, const char* name, T& v)
    {
        jsn[name] = v;
    }

    static inline void SerializeTo(nlohmann::json& jsn, const char* name, bool b) { jsn[name] = b; }

    static inline void SerializeTo(nlohmann::json& jsn, const char* name, glm::vec2 v)
    {
        jsn[name] = nlohmann::json::array({v.x, v.y});
    }

    static inline void SerializeTo(nlohmann::json& jsn, const char* name, glm::vec3 v)
    {
        jsn[name] = nlohmann::json::array({v.x, v.y, v.z});
    }

    static inline void SerializeTo(nlohmann::json& jsn, const char* name, glm::vec4 v)
    {
        jsn[name] = nlohmann::json::array({v.x, v.y, v.z, v.w});
    }

    nlohmann::json& m_json;
};

/// <summary>
/// Serilizers an object from json, provided the class has a VISITABLE_STRUCT macro defined for its members
/// </summary>
class FromJsonSerializer
{
public:
    FromJsonSerializer(nlohmann::json& a_j) : m_j(a_j) {}

    template <typename T>
    void operator()(const char* name, T& value)
    {
        SerializeFrom(m_j, name, value);
    }

private:
    template <class T>
    static void SerializeFrom(nlohmann::json& jsn, const char* name, T& v)
    {
        if (jsn.contains(name))
        {
            v = jsn[name].get<T>();
        }
    }

    static void SerializeFrom(nlohmann::json& jsn, const char* name, glm::vec2& v)
    {
        if (jsn.contains(name))
        {
            nlohmann::json::array_t arr = jsn[name];
            v.x = arr[0];
            v.y = arr[1];
        }
    }

    static void SerializeFrom(nlohmann::json& jsn, const char* name, glm::vec3& v)
    {
        if (jsn.contains(name))
        {
            nlohmann::json::array_t arr = jsn[name];
            v.x = arr[0];
            v.y = arr[1];
            v.z = arr[2];
        }
    }

    static void SerializeFrom(nlohmann::json& jsn, const char* name, glm::vec4& v)
    {
        if (jsn.contains(name))
        {
            nlohmann::json::array_t arr = jsn[name];
            v.x = arr[0];
            v.y = arr[1];
            v.z = arr[2];
            v.w = arr[2];
        }
    }

    nlohmann::json& m_j;
};

/// <summary>
/// Serializes objects to and from json, and keeps track of the mapping between the object and the
/// json file.
/// </summary>
class Serializer
{
    friend class Serializable;

public:
    Serializer() = default;
    ~Serializer();

    template <typename T>
    inline void SerializeTo(const std::string& mapping, T& value)
    {
        nlohmann::json j;
        ToJsonSerializer js(j);
        visit_struct::for_each(value, js);

        std::ofstream ofs;
        auto filename = Engine.FileIO().GetPath(FileIO::Directory::Asset, mapping);
        ofs.open(filename);
        if (ofs.is_open())
        {
            auto str = js.Json().dump(4);
            ofs << str;
            ofs.close();
        }

        Update(&value, mapping);
    }

    template <typename T>
    inline void SerializeTo(T& value)
    {
        auto mapping = m_mappings[&value];
        SerializeTo(mapping, value);
    }

    template <typename T>
    inline void SerializeFrom(const std::string& mapping, T& value)
    {
        std::string contents = Engine.FileIO().ReadTextFile(FileIO::Directory::Asset, mapping);
        
        if (!contents.empty())
        {
            nlohmann::json j = nlohmann::json::parse(contents);
            FromJsonSerializer fjs(j);
            visit_struct::for_each(value, fjs);
        }
        Update(&value, mapping);
    }

    bool HasItem(Serializable& s);
    const std::string& GetMapping(Serializable& s);

protected:
    void Add(Serializable* s);
    void Update(Serializable* s, const std::string& mapping);
    void Remove(Serializable* s);
    std::unordered_map<Serializable*, std::string> m_mappings;
};

}  // namespace Osm

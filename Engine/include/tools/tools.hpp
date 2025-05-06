#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

#include "core/input.hpp"
#include "inspector.hpp"
#include "rendering/render_components.hpp"
#include "core/transform.hpp"

namespace bee
{
// credit to stack overflow user:
// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void GetPngImageDimensions(const std::string& file_path, float& width, float& height);

inline void SwitchOnBitFlag(unsigned int& flags, unsigned int bit) { flags |= bit; }

inline void SwitchOffBitFlag(unsigned int& flags, unsigned int bit) { flags &= (~bit); }

inline bool CheckBitFlag(unsigned int flags, unsigned int bit) { return (flags & bit) == bit; }

inline bool CheckBitFlagOverlap(unsigned int flag0, unsigned int flag1) { return (flag0 & flag1) != 0; }

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
inline uint32_t HashCombine(uint32_t lhs, uint32_t rhs)
{
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    return lhs;
}

inline glm::vec3 to_vec3(const glm::vec2& vec) { return glm::vec3(vec.x, vec.y, 0.0f); }

inline glm::vec3 to_vec3(std::vector<double> array) { return glm::vec3((float)array[0], (float)array[1], (float)array[2]); }

inline glm::quat to_quat(std::vector<double> array)
{
    return glm::quat((float)array[3], (float)array[0], (float)array[1], (float)array[2]);
}

inline glm::vec4 to_vec4(std::vector<double> array)
{
    return glm::vec4((float)array[0], (float)array[1], (float)array[2], (float)array[3]);
}

/// Replace all occurrences of the search string with the replacement string.
///
/// @param subject The string being searched and replaced on, otherwise known as the haystack.
/// @param search The value being searched for, otherwise known as the needle.
/// @param replace The replacement value that replaces found search values.
/// @return a new string with all occurrences replaced.
///
std::string StringReplace(const std::string& subject, const std::string& search, const std::string& replace);

/// Determine whether or not a string ends with the given suffix. Does
/// not create an internal copy.
///
/// @param subject The string being searched in.
/// @param prefix The string to search for.
/// @return a boolean indicating if the suffix was found.
///
bool StringEndsWith(const std::string& subject, const std::string& suffix);

/// Determine whether or not a string starts with the given prefix. Does
/// not create an internal copy.
///
/// @param subject The string being searched in.
/// @param prefix The string to search for.
/// @return a boolean indicating if the prefix was found.
///
bool StringStartsWith(const std::string& subject, const std::string& prefix);

/// Splits a string according to a delimiter
///
/// @param input The string to split.
/// @param delimiter The string to use as a splitting point.
/// @return a list of strings, representing the input string split around each delimiter occurrence.
///
std::vector<std::string> SplitString(const std::string& input, const std::string& delim);

/// <summary>
/// Tries to find a substring between two other substrings. If you want to get a substring from the beginning or to the end, put
/// "" in the arguments instead of an actual string.
/// </summary>
/// <param name="input"></param>
/// <param name="start"></param>
/// <param name="end"></param>
/// <returns></returns>
std::string GetSubstringBetween(const std::string& input, const std::string& start, const std::string& end);

/// <summary>
/// Removes a substring from the main string.
/// </summary>
/// <param name="mainString">The string you want to edit.</param>
/// <param name="substringToRemove">The part you want to remove from the original string.</param>
void RemoveSubstring(std::string& mainString, const std::string& substringToRemove);

/// <summary>
/// Generates and returns a random floating-point number between two values.
/// </summary>
/// <param name="min">A minimum value.</param>
/// <param name="max">A maximum value.</param>
/// <param name="decimals">The number of decimals to which the number should be (approximately) rounded.
/// Use a higher value to get a denser range of possible outcomes.</param>
float GetRandomNumber(float min, float max, int decimals = 3);

/// <summary>
/// Generates and returns a random int number between two values.
/// </summary>
/// <param name="min">A minimum value.</param>
/// <param name="max">A maximum value.</param>
int GetRandomNumberInt(int min, int max);

template <typename T>
inline T& GetComponentInChildren(entt::entity parentEntity)
{
    auto& registry = bee::Engine.ECS().Registry;
    Transform& parentTransform = registry.get<Transform>(parentEntity);

    // Iterate through the children
    entt::entity childEntity = parentTransform.FirstChild();
    while (childEntity != entt::null)
    {
        // Check if the child has the component
        if (registry.try_get<T>(childEntity))
        {
            return registry.get<T>(childEntity);
        }

        childEntity = registry.get<Transform>(childEntity).FirstChild();
    }

    throw std::runtime_error("Component not found in any children.");
}

template <typename T>
inline T* TryGetComponentInChildren(entt::entity parentEntity)
{
    auto& registry = bee::Engine.ECS().Registry;
    Transform& parentTransform = registry.get<Transform>(parentEntity);

    // Iterate through the children
    entt::entity childEntity = parentTransform.FirstChild();
    while (childEntity != entt::null)
    {
        // Check if the child has the component
        if (registry.try_get<T>(childEntity))
        {
            return registry.try_get<T>(childEntity);
        }

        childEntity = registry.get<Transform>(childEntity).FirstChild();
    }

    return nullptr;
}



/// <summary>
/// Checks if two floats are equal up to a certain decimal point.
/// https://www.tutorialspoint.com/floating-point-comparison-in-cplusplus
/// </summary>
/// <param name="x">First float-point value.</param>
/// <param name="y">Second float-point value.</param>
/// <param name="epsilon">How many decimals should be taken into account when comparing. More decimals make a more precise
/// comparision.</param> <returns>Returns true if the two numbers are equal up to the specified decimals number.</returns>
bool CompareFloats(float x, float y, float epsilon = 0.1f);

/// <summary>
/// Swaps two values.
/// </summary>
/// <param name="a">The first value.</param>
/// <param name="b">The second value.</param>
void Swap(int& a, int& b);
void Swap(float& a, float& b);

void UpdateMeshRenderer(entt::entity entity, const std::vector<std::shared_ptr<bee::Material>>& materials, int& index);

}  // namespace bee
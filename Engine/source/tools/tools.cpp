#include "tools/tools.hpp"
#include "rendering/render_components.hpp"
#include "core/ecs.hpp"

using namespace std;
// thanks to: https://stackoverflow.com/questions/5354459/c-how-to-get-the-image-size-of-a-png-file-in-directory
void bee::GetPngImageDimensions(const std::string& file_path, float& width, float& height)
{
    unsigned char buf[8];

    std::ifstream in(file_path);
    in.seekg(16);
    in.read(reinterpret_cast<char*>(&buf), 8);

    const unsigned int uwidth = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
    const unsigned int uheight = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7] << 0);

    width = static_cast<float>(uwidth);
    height = static_cast<float>(uheight);
    in.close();
}

// Courtesy of: http://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
string bee::StringReplace(const string& subject, const string& search, const string& replace)
{
    string result(subject);
    size_t pos = 0;

    while ((pos = subject.find(search, pos)) != string::npos)
    {
        result.replace(pos, search.length(), replace);
        pos += search.length();
    }

    return result;
}

bool bee::StringEndsWith(const string& subject, const string& suffix)
{
    // Early out test:
    if (suffix.length() > subject.length()) return false;

    // Resort to difficult to read C++ logic:
    return subject.compare(subject.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool bee::StringStartsWith(const string& subject, const std::string& prefix)
{
    // Early out, prefix is longer than the subject:
    if (prefix.length() > subject.length()) return false;

    // Compare per character:
    for (size_t i = 0; i < prefix.length(); ++i)
        if (subject[i] != prefix[i]) return false;

    return true;
}

std::vector<std::string> bee::SplitString(const std::string& input, const std::string& delim)
{
    std::vector<std::string> result;
    size_t pos = 0, pos2 = 0;
    while ((pos2 = input.find(delim, pos)) != std::string::npos)
    {
        result.push_back(input.substr(pos, pos2 - pos));
        pos = pos2 + 1;
    }

    result.push_back(input.substr(pos));

    return result;
}

std::string bee::GetSubstringBetween(const std::string& input, const std::string& start, const std::string& end)
{
    size_t start_pos;
    size_t end_pos;
    if (start == "")
        start_pos = 0;
    else
    {
        start_pos = input.find(start);
        if (start_pos == std::string::npos) return "";  // start pos not found
    }

    std::string leftover_string = input.substr(start_pos + start.length());

    if (end == "")
        end_pos = input.size();
    else
    {
        end_pos = leftover_string.find(end) + start_pos + start.length();
        if (end_pos == std::string::npos) return "";  // end pos not found
    }

    std::string substr = input.substr(start_pos + start.length(), end_pos - (start_pos + start.length()));
    return substr;
}

void bee::RemoveSubstring(std::string& mainString, const std::string& substringToRemove)
{
    size_t pos = mainString.find(substringToRemove);

    // Check if the substring is found
    if (pos != std::string::npos)
    {
        // Erase the substring
        mainString.erase(pos, substringToRemove.length());
    }
}

float bee::GetRandomNumber(float min, float max, int decimals)
{
    int p = (int)pow(10, decimals);
    int imin = min * p;
    int imax = max * p;

    int irand = imin + rand() % (imax - imin);
    float val = (float)irand / p;
    return val;
}

int bee::GetRandomNumberInt(int min, int max) { return rand() % (max - min + 1) + min; }

bool bee::CompareFloats(float x, float y, float epsilon)
{
    if (fabs(x - y) < epsilon) return true;  // they are same
    return false;                            // they are not same
}

void bee::Swap(int& a, int& b)
{
    const int temp = a;
    a = b;
    b = temp;
}

void bee::Swap(float& a, float& b)
{
    const float temp = a;
    a = b;
    b = temp;
}

void bee::UpdateMeshRenderer(entt::entity entity,  const std::vector<std::shared_ptr<bee::Material>>& materials, int& index )
{
    auto& registry = bee::Engine.ECS().Registry;

    if (registry.valid(entity) && registry.all_of<bee::MeshRenderer>(entity)) {
        auto& meshRenderer = registry.get<bee::MeshRenderer>(entity);
        if (index < materials.size())
        {
            meshRenderer.Material = materials[index];
            index ++;
        }
    }
    if (registry.valid(entity) && registry.all_of<bee::Transform>(entity)) {
        auto& transform = registry.get<bee::Transform>(entity);
        for (auto child = transform.begin(); child != transform.end(); ++child) {
            UpdateMeshRenderer(*child, materials, index);
        }
    }
}

#include "tools/asset_explorer_system.hpp"
#include "level_editor/terrain_system.hpp"
#include "actors/units/unit_manager_system.hpp"

#include <iostream>

#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

using namespace bee;

AssetExplorer::AssetExplorer()
{
    //variables for ImGui
    Title = "Asset Explorer";
    Priority = 3;

    m_directory = Engine.FileIO().GetPath(FileIO::Directory::Asset, "");

    ExtractFilesFromFolder(m_directory);
}

void AssetExplorer::Update(float dt){}
#ifdef BEE_INSPECTOR

//Inspect function which displays the UI for the Asset Explorer
void AssetExplorer::Inspect()
{
    if (ImGui::Begin("Asset Explorer")) //creating a new window for the Asset Explorer
    {
        ImGui::InputText("Filter", &m_filter); //search bar to look for specific files
        ImGui::SameLine();
        if(ImGui::Button(u8"\uf021"))  //refresh button
        {
            m_files.clear();
            m_folders.clear();
            ExtractFilesFromFolder(m_directory);
        }
        ImGui::Separator();

        if (m_filter == "") //if the search bar is empty, display all folders and files
        {
            DisplayFolderContents("assets");
            std::filesystem::path assets_path = std::filesystem::path("assets");
            DisplayOnlyFilesInFolder(assets_path); //display all files in the current folder we have opened
        }
        else
        {
            DisplayFilteredContents(); //show only the files/folders the user is looking for
        }
    }
    ImGui::End();
}
#endif


void bee::AssetExplorer::ExtractFilesFromFolder(const std::string& path)
{
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_directory()) //if the entry is a folder, we add it to the vector of folders and then we call this function again
        {
            m_folders.push_back(entry);
            ExtractFilesFromFolder(entry.path().string());
            std::cout << entry.path().string() << "\n"; //printing the path for debug purposes
        }
        else
        {
            //if the entry is a file, we create a new object of type ProjectFile, record the directory, give it an icon and add it to a vector of files
            ProjectFile myFile;
            myFile.m_fileDirectory = entry;
            myFile.m_Icon = DetermineIcon(myFile.m_fileDirectory.path().extension().string());
            m_files.push_back(myFile);
        }
    }
}

void bee::AssetExplorer::DisplayFolderContents(const std::filesystem::path& root)
{
    for (const auto& folder : m_folders)
    {
        auto folderRoot = folder.path().parent_path();
        if (folderRoot == root)
        {
            ImGuiTreeNodeFlags folderNodeFlags = ImGuiTreeNodeFlags_None;
            if (!std::filesystem::exists(folder)) continue; //if the folder does not exist, go to the next element in the vector
            if (std::filesystem::is_empty(folder)) folderNodeFlags |= ImGuiTreeNodeFlags_Leaf; //if the folder is empty, apply appropriate ImGui flag

            std::string folder_name = u8"\uf07b " + folder.path().filename().string(); //the name of the folder along with a folder icon (??) at the start
            if (ImGui::TreeNodeEx((folder_name + "##" + folder.path().string()).c_str(), folderNodeFlags))
            {
                DisplayFolderContents(folder.path()); //calling the function again, displaying any folders inside the current one
                DisplayOnlyFilesInFolder(folder.path()); //display any files if there are any in the folder

                ImGui::TreePop();
            }
        }
    }
}

void bee::AssetExplorer::DisplayFilteredContents()
{
    for (const auto& folder : m_folders)
    {
        if (IsEntryFiltered(folder))
        {
            ImGuiTreeNodeFlags folderNodeFlags = ImGuiTreeNodeFlags_None;
            if (std::filesystem::is_empty(folder)) folderNodeFlags |= ImGuiTreeNodeFlags_Leaf;
            std::string folderName = u8"\uf07b " + folder.path().filename().string();
            if (ImGui::TreeNodeEx((folderName + "##" + folder.path().string()).c_str(), folderNodeFlags))
            {
                DisplayFolderContents(folder.path());
                DisplayOnlyFilesInFolder(folder.path());

                ImGui::TreePop();
            }
        }
    }
    for (const auto& file : m_files)
    {
        if (IsEntryFiltered(file.m_fileDirectory))
        {
            ImGuiTreeNodeFlags fileNodeFlags = ImGuiTreeNodeFlags_Leaf;
            std::string fileName = file.m_Icon + file.m_fileDirectory.path().filename().string();
            if (ImGui::TreeNodeEx((fileName + "##" + file.m_fileDirectory.path().string()).c_str(), fileNodeFlags))
            {
                ImGui::TreePop();
            }
            SetDragDropSource(file);
        }
    }
}

void bee::AssetExplorer::DisplayOnlyFilesInFolder(std::filesystem::path directory)
{
    ImGuiTreeNodeFlags file_node_flags = ImGuiTreeNodeFlags_Leaf; //file nodes should be leaves since they don't have children
    for (const auto& file : m_files)
    {
        const auto& fileRoot = file.m_fileDirectory.path().parent_path();

        //show only files that are in the current directory
        if (fileRoot == directory)
        {

            std::string file_name = file.m_Icon + file.m_fileDirectory.path().filename().string(); //combining the name of the file with the file (??) icon 
            if (ImGui::TreeNodeEx((file_name + "##" + file.m_fileDirectory.path().string()).c_str(), file_node_flags))
            {
                ImGui::TreePop(); // we close the tree node immediately because files can't have children
            }
            SetDragDropSource(file); //allowing the file to be used as drag and drop source in the editor
        }
    }
}

bool bee::AssetExplorer::IsEntryFiltered(std::filesystem::directory_entry entry)
{
    std::string mainStringLowercase = entry.path().filename().string();
    std::transform(mainStringLowercase.begin(), mainStringLowercase.end(), mainStringLowercase.begin(), ::towlower);
    std::string subStringLowercase = m_filter;
    std::transform(subStringLowercase.begin(), subStringLowercase.end(), subStringLowercase.begin(), ::towlower);
    size_t foundPosition = mainStringLowercase.find(subStringLowercase);
    if (foundPosition != std::string::npos)
    {
        return true;
    }
    return false;
}

bool bee::AssetExplorer::SetDragDropSource(ProjectFile file)
{
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        std::string extension = file.m_fileDirectory.path().extension().string();
        std::string path = file.m_fileDirectory.path().string();
        ImGui::SetDragDropPayload(extension.c_str(), path.c_str(), sizeof(char) * (path.size() + 1));
        ImGui::Text("%s", (file.m_Icon + path).c_str());
        ImGui::EndDragDropSource();

        return true;
    }
    return false;
}

bool bee::AssetExplorer::SetDragDropTarget(std::filesystem::path& payloadData, const std::initializer_list<std::string>& dataTypes)
{
    if (ImGui::BeginDragDropTarget())
    {
        for (const auto& dataType : dataTypes)
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dataType.c_str()))
            {
                payloadData = std::filesystem::path(static_cast<const char*>(payload->Data));

                ImGui::EndDragDropTarget();
                return true;
            }
        }
    }

    return false;
}

const std::string bee::AssetExplorer::DetermineIcon(const std::string& extension) const
{
    if (extension == ".txt")
    {
        return u8"\uf15b ";
    }
    else if (extension == ".gltf")
    {
        return u8"\uf1b3 ";
    }
    else if (extension == ".glb")
    {
        return u8"\uf1b3 ";
    }
    else if (extension == ".obj")
    {
        return u8"\uf1b2 ";
    }
    else if (extension == ".png")
    {
        return u8"\uf03e ";
    }
    else if (extension == ".ttf")
    {
        return u8"\uf031 ";
    }
    else if (extension == ".json")
    {
        return u8"\uf15c ";
    }
    else if (extension == ".wav")
    {
        return u8"\uf1c7 ";
    }

    return "";
}

#include <cassert>
#include <filesystem>
#include <fstream>

#include "core/fileio.hpp"
#include "tools/log.hpp"

using namespace bee;
using namespace std;

FileIO::FileIO()
{
    Paths[Directory::Asset] = "assets/";
    Paths[Directory::Terrain] = "assets/levels/";
    Paths[Directory::UserInterface] = "assets/userinterface/";
}

FileIO::~FileIO() = default;

#include "imp_Importer.hpp"

#include <iostream>

namespace imp
{
    void Importer::SetBaseDir(const std::filesystem::path& path)
    {
        base_dir = path;
    }

    void Importer::LoadFile(const std::filesystem::path& path)
    {
        std::cout << std::format("Loading file [{}]\n", path.string());
    }

    void Importer::ReportStatistics()
    {
        std::cout << std::format("Statistics:\n  Base dir [{}]\n", base_dir.string());
    }
}
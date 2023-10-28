#pragma once

#include <filesystem>

namespace imp
{
    class Importer
    {
        std::filesystem::path base_dir;

    public:
        void SetBaseDir(const std::filesystem::path& path);
        void LoadFile(const std::filesystem::path& path);
        void ReportStatistics();
    };
}
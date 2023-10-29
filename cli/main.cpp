#include "imp.hpp"

#include <fmt/printf.h>

int main(int argc, char* argv[])
{
    std::filesystem::path path;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        path = arg;
    }

    if (!std::filesystem::exists(path)) {
        fmt::println("Did not pass a valid path: {}", path.string());
        return 1;
    }

    imp::Importer importer;
    importer.SetBaseDir(path.parent_path());
    importer.LoadFile(path);
    importer.ReportStatistics();

    auto scene = importer.GenerateScene();

    fmt::println("Scene[geometries = {}, geometry ranges = {}, meshes = {}]",
        scene.geometries.count, scene.geometry_ranges.count, scene.meshes.count);
}

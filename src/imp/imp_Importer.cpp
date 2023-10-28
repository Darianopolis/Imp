#include "imp_Importer.hpp"

#include <iostream>

namespace imp
{
    namespace loaders
    {
        std::vector<ModelLoaderFn> loader_fns;
    }

    std::monostate loaders::RegisterModelLoader(ModelLoaderFn fn)
    {
        loader_fns.emplace_back(fn);
        return {};
    }

// -----------------------------------------------------------------------------

    void Importer::SetBaseDir(const std::filesystem::path& path)
    {
        base_dir = path;
    }

    void Importer::LoadFile(const std::filesystem::path& path)
    {
        std::cout << std::format("Loading file [{}]\n", path.string());

        for (auto& loader_fn : loaders::loader_fns) {
            auto _loader = loader_fn();
            if (_loader->Import(*this, path)) {
                loader = std::move(_loader);
                break;
            }
        }

        if (!loader) {
            std::cout << std::format("Could not find loader for [{}]", path.string());
        }
    }

    void Importer::ReportStatistics()
    {
        for (uint32_t i = 0; i < geometries.size(); ++i) {
            auto& geometry = geometries[i];

            std::cout << std::format(
                "Geometry[{}]:\n"
                "  Positions: {}\n"
                "  Indices:   {}\n",
                i,
                geometry.positions.count,
                geometry.indices.count);
        }

        for (uint32_t i = 0; i < meshes.size(); ++i) {
            auto& mesh = meshes[i];
            auto& M = mesh.transform;

            std::cout << std::format(
                "Mesh[{}]:\n"
                "  Geometry: {}\n"
                "  Transform:\n"
                "    {:12.5f} {:12.5f} {:12.5f} {:12.5f}\n"
                "    {:12.5f} {:12.5f} {:12.5f} {:12.5f}\n"
                "    {:12.5f} {:12.5f} {:12.5f} {:12.5f}\n",
                i,
                mesh.geometry_idx,
                M[0][0], M[1][0], M[2][0], M[3][0],
                M[0][1], M[1][1], M[2][1], M[3][1],
                M[0][2], M[1][2], M[2][2], M[3][2]);
        }
    }
}
#pragma once

#include "imp_Core.hpp"
#include "imp_Scene.hpp"

#include <filesystem>
#include <span>

#include <vendor/glm_include.hpp>

namespace imp
{
    struct Importer;

    namespace loaders
    {
        struct ModelLoader
        {
            virtual bool Import(Importer& importer, const std::filesystem::path& path) = 0;
            virtual ~ModelLoader() = 0;
        };

        inline
        ModelLoader::~ModelLoader() = default;

        using ModelLoaderFn = std::unique_ptr<ModelLoader>(*)();
        std::monostate RegisterModelLoader(ModelLoaderFn fn);
    };

    struct InGeometry
    {
        Range<glm::vec3> positions;
        Range<glm::vec3> normals;
        Range<glm::vec2> tex_coords;
        Range<uint32_t>  indices;
    };

    struct InMesh
    {
        uint32_t    geometry_idx;
        glm::mat4x3 transform;
    };

    struct Importer
    {
        std::filesystem::path base_dir;

        std::unique_ptr<loaders::ModelLoader> loader;

        std::vector<InGeometry> geometries;
        std::vector<InMesh>     meshes;

        MemoryPool memory_pool;

    public:
        void SetBaseDir(const std::filesystem::path& path);
        void LoadFile(const std::filesystem::path& path);

        void ReportStatistics();
        void ReportDetailed();

        Scene GenerateScene();
    };
}
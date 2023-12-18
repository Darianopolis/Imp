#pragma once

#include "imp_Core.hpp"
#include "imp_Scene.hpp"

#include <filesystem>
#include <variant>

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

    struct InImageFileURI
    {
        std::string uri;
    };

    struct InImageFileBuffer
    {
        std::vector<std::byte> data;
    };

    struct InImageBuffer
    {
        Range<std::byte> data;
        glm::uvec2       size;
        TextureFormat    format;
    };

    using InImageDataSource = std::variant<InImageBuffer, InImageFileBuffer, InImageFileURI>;

    struct InTexture
    {
        InImageDataSource data;
    };

    struct InMaterial
    {
        // struct Property
        // {
        //     std::string_view      name;
        //     int32_t               texture_idx = -1;
        //     std::array<float, 4>  values = {};
        // };

        // std::vector<Property> properties;

        struct TextureProcess
        {
            int32_t       source = -1;
            TextureFormat format;

            std::function<glm::vec4(glm::vec4)> fn;
        };

        TextureProcess basecolor_alpha;
    };

    struct Importer
    {
        std::filesystem::path base_dir;

        std::unique_ptr<loaders::ModelLoader> loader;

        std::vector<InGeometry> geometries;
        std::vector<InMesh>     meshes;

        std::vector<InTexture>  textures;
        std::vector<InMaterial> materials;

        MemoryPool memory_pool;

    public:
        void SetBaseDir(const std::filesystem::path& path);
        void LoadFile(const std::filesystem::path& path);

        void ReportStatistics();
        void ReportDetailed();

        Scene GenerateScene();
    };
}
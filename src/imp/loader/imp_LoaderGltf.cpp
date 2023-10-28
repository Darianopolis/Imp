#include <imp/imp_Importer.hpp>

#include <fastgltf/parser.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <ankerl/unordered_dense.h>
#include <vendor/ankerl_hashes.hpp>

namespace imp::loaders
{
    struct ModelLoaderGltf : ModelLoader
    {
        fastgltf::Asset asset;
        Importer*       importer;

        std::vector<void*> allocations;

    public:
        ~ModelLoaderGltf()
        {
            for (auto* ptr : allocations) {
                Free(ptr);
            }
        }

        template<class T>
        InRange<T> MakeRangeForAccessor(const fastgltf::Accessor& accessor)
        {
            auto* arr = Alloc<T>(accessor.count);
            allocations.emplace_back(arr);
            fastgltf::copyFromAccessor<T>(asset, accessor, arr);
            return InRange<T> { arr, accessor.count };
        }

    public:
        ankerl::unordered_dense::map<std::pair<uint32_t, uint32_t>, uint32_t> geometries;

        void ProcessGeometry()
        {
            for (auto& mesh : asset.meshes) {
                for (auto& prim : mesh.primitives) {
                    auto findAccessor = [&](std::string_view name) -> fastgltf::Accessor* {
                        auto iter = prim.findAttribute(name);
                        return iter == prim.attributes.end() ? nullptr : &asset.accessors[iter->second];
                    };

                    auto pos_accessor = findAccessor("POSITION");
                    if (!pos_accessor) {
                        continue;
                    }

                    InGeometry geom;

                    geom.positions = MakeRangeForAccessor<glm::vec3>(*pos_accessor);

                    if (prim.indicesAccessor) {
                        geom.indices = MakeRangeForAccessor<uint32_t>(asset.accessors[prim.indicesAccessor.value()]);
                    }

                    if (auto* normal_accessor = findAccessor("NORMAL")) {
                        geom.normals = MakeRangeForAccessor<glm::vec3>(*normal_accessor);
                    }

                    if (auto* texcoord_accessor = findAccessor("TEXCOORD_0")) {
                        geom.tex_coords = MakeRangeForAccessor<glm::vec2>(*texcoord_accessor);
                    }

                    uint32_t geom_idx = uint32_t(importer->geometries.size());
                    importer->geometries.emplace_back(geom);
                    geometries.insert({
                        { uint32_t(&mesh - asset.meshes.data()), uint32_t(&prim - mesh.primitives.data()) },
                        geom_idx,
                    });
                }
            }
        }

        void ProcessNode(fastgltf::Node& node, const glm::mat4& parent_transform)
        {
            glm::mat4 transform(1.f);
            if (auto* trs = std::get_if<fastgltf::Node::TRS>(&node.transform)) {
                transform = glm::translate(glm::mat4(1.f), std::bit_cast<glm::vec3>(trs->translation))
                    * glm::mat4_cast(glm::quat(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]))
                    * glm::scale(glm::mat4(1.f), std::bit_cast<glm::vec3>(trs->scale));
            } else if (auto* m = std::get_if<fastgltf::Node::TransformMatrix>(&node.transform)) {
                transform = std::bit_cast<glm::mat4>(*m);
            }

            transform = parent_transform * transform;

            if (node.meshIndex.has_value()) {
                auto& mesh = asset.meshes[node.meshIndex.value()];
                for (auto& prim : mesh.primitives) {
                    auto geom_iter = geometries.find(std::make_pair(
                        uint32_t(node.meshIndex.value()), uint32_t(&prim - mesh.primitives.data())));
                    if (geom_iter == geometries.end()) {
                        continue;
                    }

                    importer->meshes.emplace_back(InMesh {
                        .geometry_idx = geom_iter->second,
                        .transform = transform,
                    });
                }
            }

            for (auto child_idx : node.children) {
                ProcessNode(asset.nodes[child_idx], transform);
            }
        }

    public:
        virtual bool Import(Importer& _importer, const std::filesystem::path& path) override
        {
            fastgltf::GltfType type;
            if (path.extension() == ".gltf") {
                type = fastgltf::GltfType::glTF;
            } else if (path.extension() == ".glb") {
                type = fastgltf::GltfType::GLB;
            } else {
                return false;
            }

            importer = &_importer;

            fastgltf::Parser parser {
                fastgltf::Extensions::KHR_texture_transform
                | fastgltf::Extensions::KHR_texture_basisu
                | fastgltf::Extensions::MSFT_texture_dds
                | fastgltf::Extensions::KHR_mesh_quantization
                | fastgltf::Extensions::EXT_meshopt_compression
                | fastgltf::Extensions::KHR_lights_punctual
                | fastgltf::Extensions::EXT_texture_webp
                | fastgltf::Extensions::KHR_materials_specular
                | fastgltf::Extensions::KHR_materials_ior
                | fastgltf::Extensions::KHR_materials_iridescence
                | fastgltf::Extensions::KHR_materials_volume
                | fastgltf::Extensions::KHR_materials_transmission
                | fastgltf::Extensions::KHR_materials_clearcoat
                | fastgltf::Extensions::KHR_materials_emissive_strength
                | fastgltf::Extensions::KHR_materials_sheen
                | fastgltf::Extensions::KHR_materials_unlit
            };

            fastgltf::GltfDataBuffer data;
            data.loadFromFile(path);

            constexpr auto GltfOptions =
                fastgltf::Options::DontRequireValidAssetMember
                | fastgltf::Options::AllowDouble
                | fastgltf::Options::LoadGLBBuffers
                | fastgltf::Options::LoadExternalBuffers;

            type = fastgltf::determineGltfFileType(&data);

            if (type == fastgltf::GltfType::Invalid) {
                Error("fastgltf-loader: Corrupt gltf file, could not determine file type");
            }

            auto res = type == fastgltf::GltfType::glTF
                ? parser.loadGLTF(&data, importer->base_dir, GltfOptions)
                : parser.loadBinaryGLTF(&data, importer->base_dir, GltfOptions);

            if (!res) {
                Error("fastgltf-loader: Error parsing gltf");
            }

            asset = std::move(res.get());

            ProcessGeometry();

            for (auto& node_idx : asset.scenes[asset.defaultScene.value()].nodeIndices) {
                ProcessNode(asset.nodes[node_idx], glm::mat4(1.f));
            }

            return true;
        }
    };

    namespace
    {
        auto _loaded = RegisterModelLoader(
            +[]()->std::unique_ptr<ModelLoader>{
                return std::make_unique<ModelLoaderGltf>();
            });
    }
}
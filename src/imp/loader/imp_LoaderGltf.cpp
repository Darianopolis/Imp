#include <imp/imp_Importer.hpp>

#include <fastgltf/parser.hpp>
#include <fastgltf/util.hpp>
#include <fastgltf/glm_element_traits.hpp>

namespace imp::loaders
{
    struct ModelLoaderGltf : ModelLoader
    {
        fastgltf::Asset asset;
        Importer*       importer;

        MemoryPool memory_pool;

    public:
        template<class T>
        Range<T> MakeRangeForAccessor(const fastgltf::Accessor& accessor)
        {
            auto* arr = memory_pool.Allocate<T>(accessor.count);
            fastgltf::copyFromAccessor<T>(asset, accessor, arr);
            return Range<T> { arr, accessor.count };
        }

    public:
        ankerl::unordered_dense::map<std::pair<uint32_t, uint32_t>, uint32_t> geometries;

        void LoadGeometry()
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

    public:
        ankerl::unordered_dense::map<size_t, int32_t> textures;

        void LoadMaterials()
        {
            for (auto& texture_in : asset.textures) {
                auto add_image = [&](auto&& source) {
                    textures.insert({ int32_t(&texture_in - asset.textures.data()), int32_t(importer->textures.size()) });
                    importer->textures.emplace_back(std::move(source));
                };

                if (texture_in.imageIndex) {
                    std::visit(OverloadSet {
                        [&](const fastgltf::sources::URI& uri) {
                            add_image(InImageFileURI(fmt::format("{}/{}", importer->base_dir.string(), uri.uri.path())));
                        },
                        [&](const fastgltf::sources::Vector& vec) {
                            InImageFileBuffer source;
                            source.data.resize(vec.bytes.size());
                            std::memcpy(source.data.data(), vec.bytes.data(), vec.bytes.size());
                            add_image(std::move(source));
                        },
                        [&](const fastgltf::sources::ByteView& byteView) {
                            InImageFileBuffer source;
                            source.data.resize(byteView.bytes.size());
                            std::memcpy(source.data.data(), byteView.bytes.data(), byteView.bytes.size());
                            add_image(std::move(source));
                        },
                        [&](const fastgltf::sources::BufferView& bufferViewIdx) {
                            auto& view = asset.bufferViews[bufferViewIdx.bufferViewIndex];
                            auto& buffer = asset.buffers[view.bufferIndex];
                            auto* bytes = fastgltf::DefaultBufferDataAdapter{}(buffer) + view.byteOffset;
                            InImageFileBuffer source;
                            source.data.resize(view.byteLength);
                            std::memcpy(source.data.data(), bytes, view.byteLength);
                            add_image(std::move(source));
                        },
                        [&](auto&&) {},
                    }, asset.images[texture_in.imageIndex.value()].data);
                }
            }

            for (auto& material_in : asset.materials) {
                auto& material = importer->materials.emplace_back();

                auto find_texture = [&](auto& info) -> int32_t {
                    if (!info) return -1;
                    auto f = textures.find(info->textureIndex);
                    if (f == textures.end()) return -1;
                    return f->second;
                };

                auto set_values = [](auto& values) -> std::array<float, 4> {
                    std::array<float, 4> out{};
                    for (uint32_t i = 0; i < values.size(); ++i) {
                        out[i] = values[i];
                    }
                    return out;
                };

                // auto& albedo_alpha = material.properties.emplace_back();
                // albedo_alpha.name = "base_color";
                // albedo_alpha.texture_idx = find_texture(material_in.pbrData.baseColorTexture);
                // albedo_alpha.values = material_in.pbrData.baseColorFactor;

                // auto& normal = material.properties.emplace_back();
                // normal.name = "normal";
                // normal.texture_idx = find_texture(material_in.normalTexture);

                // auto& metalness_roughness = material.properties.emplace_back();
                // metalness_roughness.name = "metalness_roughness";
                // metalness_roughness.texture_idx = find_texture(material_in.pbrData.metallicRoughnessTexture);

                // auto& metalness = material.properties.emplace_back();
                // metalness.name = "metalness";
                // metalness.values[0] = material_in.pbrData.metallicFactor;

                // auto& roughness = material.properties.emplace_back();
                // roughness.name = "roughness";
                // roughness.values[0] = material_in.pbrData.roughnessFactor;

                // auto& emission = material.properties.emplace_back();
                // emission.name = "emission";
                // emission.texture_idx = find_texture(material_in.emissiveTexture);
                // emission.values = set_values(material_in.emissiveFactor);
                // emission.values[3] = material_in.emissiveStrength.value_or(1.f);

                material.basecolor_alpha = InMaterial::TextureProcess {
                    { find_texture(material_in.pbrData.baseColorTexture) }, TextureFormat::RGBA8_SRGB,
                    [&](glm::vec4 v) -> glm::vec4 { return v; },
                };
            }
        }

    public:
        void LoadNode(fastgltf::Node& node, const glm::mat4& parent_transform)
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
                LoadNode(asset.nodes[child_idx], transform);
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

            LoadMaterials();
            LoadGeometry();

            for (auto& node_idx : asset.scenes[asset.defaultScene.value()].nodeIndices) {
                LoadNode(asset.nodes[node_idx], glm::mat4(1.f));
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
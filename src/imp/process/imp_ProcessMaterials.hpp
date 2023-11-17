#pragma once

#include <imp/imp_Importer.hpp>

#include <stb_image.h>

namespace imp::detail
{
    inline
    void ProcessMaterials(Importer& importer, Scene& scene)
    {
        auto& memory_pool = importer.memory_pool;
        auto& textures = importer.textures;
        auto& materials = importer.materials;

        struct LoadedTexture
        {
            glm::uvec2             size;
            std::vector<glm::vec4> pixels;

            void Resize(glm::uvec2 _size)
            {
                size = _size;
                pixels.resize(size.x * size.y);
            }

            glm::vec4& Get(glm::uvec2 pos)
            {
                return pixels[pos.x + pos.y * size.x];
            }
        };

        struct Rgba8 { uint8_t r, g, b, a; };

        std::vector<LoadedTexture> loaded_textures(textures.size());

#pragma omp parallel for
        for (uint32_t i = 0; i < textures.size(); ++i) {
            auto& texture = textures[i];
            auto& loaded_texture = loaded_textures[i];

            if (auto uri = std::get_if<InImageFileURI>(&texture.data)) {
                int32_t width, height, channels;
                auto data = stbi_load(uri->uri.c_str(), &width, &height, &channels, STBI_rgb_alpha);
                auto* pixels = reinterpret_cast<Rgba8*>(data);

                loaded_texture.Resize({ uint32_t(width), uint32_t(height) });
                for (int32_t j = 0; j < width * height; ++j) {
                    loaded_texture.pixels[j] = {
                        float(pixels[j].r) / 255.f,
                        float(pixels[j].g) / 255.f,
                        float(pixels[j].b) / 255.f,
                        float(pixels[j].a) / 255.f,
                    };
                }

                stbi_image_free(data);
            }
        }

        ankerl::unordered_dense::map<std::pair<int32_t, int32_t>, InMaterial::TextureProcess> processes;

        scene.materials = { memory_pool.Allocate<Material>(materials.size()), materials.size() };

        for (int32_t i = 0; i < materials.size(); ++i) {
            auto& material_in = materials[i];
            auto& material_out = scene.materials[i];

            material_out = {};

            if (material_in.basecolor_alpha.source != -1) {
                processes.insert({ std::make_pair(material_in.basecolor_alpha.source, 0), material_in.basecolor_alpha });
            }

            material_out.albedo_alpha_texture = material_in.basecolor_alpha.source;
        }

        scene.textures = { memory_pool.Allocate<Texture>(processes.size()), processes.size() };

        uint32_t texture_idx = 0;
        for (auto&[key, process] : processes) {
            auto& texture_out = scene.textures[texture_idx];
            auto& texture_in = loaded_textures[process.source];

            auto size = texture_in.size;
            texture_out.size = size;
            texture_out.format = process.format;

            auto pixel_stride = 0;
            auto channels = 0;
            switch (process.format) {
                    using enum TextureFormat;
                break;case RGBA8_UNORM:
                      case RGBA8_SRGB:
                    pixel_stride = 4;
                    channels = 4;
                break;case RG8_UNORM:
                    pixel_stride = 2;
                    channels = 2;
                break;case R8_UNORM:
                    pixel_stride = 1;
                    channels = 1;
                break;default:
                    std::unreachable();
            }

            auto byte_size = size.x * size.y * pixel_stride;

            texture_out.data = { memory_pool.Allocate<std::byte>(byte_size), byte_size };

            for (uint32_t y = 0; y < texture_in.size.y; ++y) {
                for (uint32_t x = 0; x < texture_in.size.x; ++x) {
                    uint8_t* pixel = reinterpret_cast<uint8_t*>(&texture_out.data[(x + y * size.x) * pixel_stride]);
                    auto res = process.fn(texture_in.Get({ uint32_t(x), uint32_t(y) }));
                    for (int32_t i = 0; i < channels; ++i) {
                        pixel[i] = uint8_t(res[i] * 255.f);
                    }
                }
            }

            texture_idx++;
        }
    }
}
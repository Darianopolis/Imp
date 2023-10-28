# 😈 Imp 😈

Imp provides functionality for asset importing, processing, and exporting.

- Model file formats
    - Fast importing for gltf / obj / fbx
    - Fallback importing with assimp
- Image file formats
    - jpg / png / bmp / psd / tga / gif / hdr / pic / pnm / dds
- Vertex deduplication
- Meshletization
- Vertex quantization
    - float -> unorm/snorm/sfloat
    - tangent space encoding
    - bone selection optimization
- Image processing
    - Channel repacking
    - BC7/BC6h/BC5/BC4 encoding
- Runtime-ready compression
    - GDeflate (de)compression utilities
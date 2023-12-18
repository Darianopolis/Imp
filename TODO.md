#### image ops
- load
- swizzle/merge
- resize
- mipmap gen
- reformat

#### mesh ops
- generate normals / tangents / bitangents
- optimizations
    - vertex cache optimization
- meshletize
    - cone cull optimizations
- quantization
    - stable position quantization (with meshlets)
    - tangent space quantization
    - uv quantization
    - bone filtering and quantization
- generate lods

#### runtime
- (de)serialization
- file mapping
- on demand loading
- compression
    - GDeflate
    - LZ4
    - zlib

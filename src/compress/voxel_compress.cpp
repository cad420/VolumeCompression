//
// Created by wyz on 2021/2/5.
//
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include<iencoder.h>
#include<memory>

/**
 * @note most 3 instances of encoders can create!
 * 1.just receive a complete block data
 * 2.return compressed data
 * @brief first pass encoding options then giving a src ptr with raw data and packets to store results
 */
class VoxelCompressImpl{
public:
    /**
     * @brief just need initialize once, all blocks share the same options
     */
    explicit VoxelCompressImpl(const VoxelCompressOptions& opts);
    bool compress(void* src_ptr,std::vector<std::vector<uint8_t>>& packets);
private:
    /**
     * @brief two implication: OpenGL and CUDA
     */
    std::unique_ptr<IEncoder> encoder;
    VoxelCompressOptions compress_opts;
};

VoxelCompress::VoxelCompress(const VoxelCompressOptions& opts) {
    impl=std::make_unique<VoxelCompressImpl>(opts);
}

bool VoxelCompress::compress(void* src_ptr,std::vector<std::vector<uint8_t>>& packets) {
    return impl->compress(src_ptr,packets);
}

//
// Created by wyz on 2021/2/5.
//
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include<iencoder.h>
#include<memory>
#include<vector>
/**
 * 1.each read a slice with w x h, total read depth(slice_num) times.(in this way, read is fast)
 */
class VoxelCompressImpl{
public:
    explicit VoxelCompressImpl(const VoxelCompressOptions& opts);
    bool compress();
private:
    /**
     * number of encoders is (w+block_length-1)/block_length * (h+block_length-1)/block_length
     */
    std::vector<std::unique_ptr<IEncoder>> encoders;
    VoxelCompressOptions compress_opts;
};

VoxelCompress::VoxelCompress(const VoxelCompressOptions& opts) {
    impl=std::make_unique<VoxelCompressImpl>(opts);
}

bool VoxelCompress::compress() {
    return impl->compress();
}

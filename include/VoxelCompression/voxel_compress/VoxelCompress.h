//
// Created by wyz on 2021/2/5.
//

#ifndef VOXELCOMPRESSION_VOXELCOMPRESS_H
#define VOXELCOMPRESSION_VOXELCOMPRESS_H
#include<memory>
#include<cstdint>
#include<string>
#include<vector>
struct VoxelCompressOptions{
    uint32_t width;
    uint32_t height;
};
class VoxelCompressImpl;
class VoxelCompress{
public:
    explicit VoxelCompress(const VoxelCompressOptions& opts);
    bool compress(void* src_ptr,std::vector<std::vector<uint8_t>>& packets);
private:
    std::unique_ptr<VoxelCompressImpl> impl;
};



#endif //VOXELCOMPRESSION_VOXELCOMPRESS_H

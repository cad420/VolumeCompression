//
// Created by wyz on 2021/2/5.
//

#ifndef VOXELCOMPRESSION_VOXELCOMPRESS_H
#define VOXELCOMPRESSION_VOXELCOMPRESS_H
#include<memory>
#include<cstdint>
#include<string>
struct VoxelCompressOptions{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t block_length;
    uint32_t padding;
    std::string in_file_path;
    std::string out_file_path;
};
class VoxelCompressImpl;
class VoxelCompress{
public:
    explicit VoxelCompress(const VoxelCompressOptions& opts);
    bool compress();
private:
    std::unique_ptr<VoxelCompressImpl> impl;
};



#endif //VOXELCOMPRESSION_VOXELCOMPRESS_H

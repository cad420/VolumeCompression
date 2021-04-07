//
// Created by wyz on 2021/2/5.
//

#ifndef VOXELCOMPRESSION_VOXELCOMPRESS_H
#define VOXELCOMPRESSION_VOXELCOMPRESS_H
#include<memory>
#include<cstdint>
#include<string>
#include<vector>
#include<nvEncodeAPI.h>
#include<VoxelCompression/utils/NvEncoderCLIOptions.h>
struct VoxelCompressOptions{
    NV_ENC_BUFFER_FORMAT input_buffer_format=NV_ENC_BUFFER_FORMAT_NV12;
    uint32_t width=0;
    uint32_t height=0;
    NvEncoderInitParam encode_init_params;
};
class VoxelCompressImpl;
class VoxelCompress{
public:
    explicit VoxelCompress(const VoxelCompressOptions& opts);
    ~VoxelCompress();
    //allocated src buffer
    bool compress(uint8_t* src_ptr,int64_t len,std::vector<std::vector<uint8_t>>& packets);
private:
    std::unique_ptr<VoxelCompressImpl> impl;
};



#endif //VOXELCOMPRESSION_VOXELCOMPRESS_H

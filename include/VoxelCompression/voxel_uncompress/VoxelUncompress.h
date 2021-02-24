//
// Created by wyz on 2021/2/7.
//

#ifndef VOXELCOMPRESSION_VOXELUNCOMPRESS_H
#define VOXELCOMPRESSION_VOXELUNCOMPRESS_H
#include<nvEncodeAPI.h>
#include<cuda.h>
#include<nvcuvid.h>
#include<memory>
#include<vector>
struct VoxelUncompressOptions{
    uint32_t width=0;
    uint32_t height=0;
    NV_ENC_BUFFER_FORMAT output_buffer_format=NV_ENC_BUFFER_FORMAT_NV12;
    cudaVideoCodec codec_method=cudaVideoCodec_H264;
    bool use_device_frame_buffer=true;
    CUcontext cu_ctx=nullptr;
};

class VoxelUncompressImpl;
class VoxelUncompress{
public:
    VoxelUncompress(const VoxelUncompressOptions& opts);
    ~VoxelUncompress();
    //allocated dest buffer
    bool uncompress(uint8_t* dest_ptr,int64_t len,std::vector<std::vector<uint8_t>>& packets);
private:
    std::unique_ptr<VoxelUncompressImpl> impl;
};




#endif //VOXELCOMPRESSION_VOXELUNCOMPRESS_H

//
// Created by wyz on 2021/2/7.
//
#include<VoxelCompression/voxel_uncompress/VoxelUncompress.h>
#include<VoxelCompression/utils/NvCodecUtils.h>
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include<fstream>
#include<iostream>

using namespace std;
#define volume_file_name_0 "aneurism_256_256_256_uint8.raw"
#define volume_0_len 256*256*256
bool readVolumeData(uint8_t* &data, int64_t& len)
{
    std::fstream in(volume_file_name_0,ios::in|ios::binary);
    if(!in.is_open()){
        throw std::runtime_error("file open failed");
    }
    in.seekg(0,ios::beg);
    data=new uint8_t[volume_0_len];
    len=in.read(reinterpret_cast<char*>(data),volume_0_len).gcount();
    if(len==volume_0_len)
        return true;
    else
        return false;
}
int main(int argc,char** argv)
{

    checkCUDAErrors(cuInit(0));
    CUdevice cuDevice=0;
    checkCUDAErrors(cuDeviceGet(&cuDevice, 0));
    CUcontext cuContext = nullptr;
    checkCUDAErrors(cuCtxCreate(&cuContext,0,cuDevice));
    uint8_t* res_ptr=nullptr;
    int64_t length=256*256*256;
    checkCUDAErrors(cuMemAlloc((CUdeviceptr*)&res_ptr,length));//driver api prefix cu
//    cudaMalloc((void**)(CUdeviceptr*)&res_ptr,length);//runtime api prefix cuda

    std::cout<<"compress"<<std::endl;
    VoxelCompressOptions c_opts;
    c_opts.height=256;
    c_opts.width=256;
    c_opts.input_buffer_format=NV_ENC_BUFFER_FORMAT_NV12;
    VoxelCompress v_cmp(c_opts);
    uint8_t* data=nullptr;
    int64_t len=0;
    readVolumeData(data,len);
    std::vector<std::vector<uint8_t>> packets;
    v_cmp.compress(data,len,packets);
    int64_t cmp_size=0;
    for(size_t i=0;i<packets.size();i++){
        cmp_size+=packets[i].size();
    }
    std::cout<<"packets size is: "<<cmp_size<<std::endl;


    std::cout<<"uncompress"<<std::endl;
    VoxelUncompressOptions opts;
    opts.height=256;
    opts.width=256;
    opts.cu_ctx=cuContext;
    VoxelUncompress v_uncmp(opts);

    v_uncmp.uncompress(res_ptr,length,packets);
    std::vector<uint8_t> uncmp_res;
    uncmp_res.resize(length);
    auto res=cuMemcpyDtoH(uncmp_res.data(), (CUdeviceptr)(res_ptr), length);
    if(res!=CUDA_SUCCESS){
        throw std::runtime_error("cuMemcpyDtoH failed");
    }
    std::fstream out("uncmpres_256_256_256_uint8.raw",std::ios::out|std::ios::binary);
    out.write(reinterpret_cast<const char *>(uncmp_res.data()), uncmp_res.size());
    out.close();
    std::cout<<"finish"<<std::endl;
    return 0;
}

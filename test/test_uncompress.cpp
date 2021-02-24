//
// Created by wyz on 2021/2/7.
//
#include<VoxelCompression/voxel_uncompress/VoxelUncompress.h>
#include<VoxelCompression/utils/NvCodecUtils.h>
#include<cuda_runtime.h>
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

    VoxelUncompressOptions opts;
    opts.height=256;
    opts.width=256;
    opts.cu_ctx=cuContext;
    VoxelUncompress v_uncmp(opts);
    std::vector<std::vector<uint8_t>> packets;
    v_uncmp.uncompress(res_ptr,length,packets);

    std::cout<<"finish"<<std::endl;
    return 0;
}

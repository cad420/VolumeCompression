//
// Created by wyz on 2021/2/5.
//
#include<cuda.h>
#include<nvdec/NvDecoder.h>
#include<VoxelCompression/voxel_uncompress/VoxelUncompress.h>
#include<VoxelCompression/utils/NvCodecUtils.h>

simplelogger::Logger* logger;
/**
 *
 */
class VoxelUncompressImpl{
public:
    explicit VoxelUncompressImpl(const VoxelUncompressOptions& opts);
    ~VoxelUncompressImpl(){}
    bool uncompress(uint8_t* dest_ptr,int64_t len,std::vector<std::vector<uint8_t>>& packets);
private:
    bool initCUDA();
    bool initDecoder();
private:
    CUcontext cu_ctx;

    int width,height;
    NV_ENC_BUFFER_FORMAT output_format;
};

VoxelUncompressImpl::VoxelUncompressImpl(const VoxelUncompressOptions &opts)
{
    if(opts.cu_ctx== nullptr){
        initCUDA();
    }
    else{
        this->cu_ctx=opts.cu_ctx;
    }

    NvDecoder decoder(cu_ctx,opts.use_device_frame_buffer,opts.codec_method);
    width=opts.width;
    height=opts.height;
    output_format=opts.output_buffer_format;
}

bool VoxelUncompressImpl::initCUDA()
{
    checkCUDAErrors(cuInit(0));
    int cu_device_cnt=0;
    CUdevice cu_device;
    int using_gpu=0;
    char using_device_name[80];
    checkCUDAErrors(cuDeviceGetCount(&cu_device_cnt));
    checkCUDAErrors(cuDeviceGet(&cu_device,using_gpu));
    checkCUDAErrors(cuDeviceGetName(using_device_name,sizeof(using_device_name),cu_device));
    std::cout<<"GPU in use: "<<using_device_name<<std::endl;
    this->cu_ctx=nullptr;
    checkCUDAErrors(cuCtxCreate(&cu_ctx,0,cu_device));
    return true;
}

bool VoxelUncompressImpl::initDecoder()
{

    return true;
}

bool VoxelUncompressImpl::uncompress(uint8_t *dest_ptr, int64_t len, std::vector<std::vector<uint8_t>> &packets) {
    return false;
}


VoxelUncompress::VoxelUncompress(const VoxelUncompressOptions &opts)
{
    impl=std::make_unique<VoxelUncompressImpl>(opts);
}

bool VoxelUncompress::uncompress(uint8_t *dest_ptr, int64_t len, std::vector<std::vector<uint8_t>> &packets)
{
    return impl->uncompress(dest_ptr,len,packets);
}

VoxelUncompress::~VoxelUncompress()
{

}
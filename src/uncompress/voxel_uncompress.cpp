//
// Created by wyz on 2021/2/5.
//
#include<cuda.h>
#include<nvdec/NvDecoder.h>
#include<VoxelCompression/voxel_uncompress/VoxelUncompress.h>
#include<VoxelCompression/utils/NvCodecUtils.h>



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

    std::unique_ptr<NvDecoder> decoder;
    bool use_device_ptr;
};

VoxelUncompressImpl::VoxelUncompressImpl(const VoxelUncompressOptions &opts)
{
    if(opts.cu_ctx== nullptr){
        initCUDA();
    }
    else{
        this->cu_ctx=opts.cu_ctx;
    }

    decoder=std::make_unique<NvDecoder>(cu_ctx,opts.use_device_frame_buffer,opts.codec_method);
    std::cout<<"create decoder"<<std::endl;
    width=opts.width;
    height=opts.height;
    output_format=opts.output_buffer_format;
    use_device_ptr=opts.use_device_frame_buffer;
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

bool VoxelUncompressImpl::uncompress(uint8_t *dest_ptr, int64_t len, std::vector<std::vector<uint8_t>> &packets)
{
    if(dest_ptr==nullptr || len<=0)
        return false;
    cuCtxSetCurrent(cu_ctx);

    int total_decode_frame_num=0;
    int64_t offset=0;
    for(size_t i=0;i<packets.size()+1;i++){
        uint8_t* packet=i<packets.size()?packets[i].data():nullptr;
        size_t len=i<packets.size()?packets[i].size():0;
//        std::cout<<"len: "<<len<<std::endl;
//        std::cout<<"i: "<<i<<"\tpackets.size: "<<packets.size()<<std::endl;
        int frame_decode_num=decoder->Decode(packet,len);
        total_decode_frame_num+=frame_decode_num;
//        std::cout<<"frame decode num: "<<frame_decode_num<<"\ttotal decode frame num: "<<total_decode_frame_num<<std::endl;
        for(size_t j=0;j<frame_decode_num;j++){
            auto frame_ptr=decoder->GetFrame();//device ptr
            auto decode_frame_bytes=decoder->GetFrameSize();

                auto res=cuMemcpy((CUdeviceptr)(dest_ptr+offset),(CUdeviceptr)frame_ptr,decode_frame_bytes);
                offset+=decode_frame_bytes;
                if(res!=CUDA_SUCCESS){
                    throw std::runtime_error("cuMemcpy error");
                }

        }
    }

//    std::cout<<"total_decode_frame_num: "<<total_decode_frame_num<<std::endl;
    return true;
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
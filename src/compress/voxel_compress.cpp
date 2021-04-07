//
// Created by wyz on 2021/2/5.
//
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include<VoxelCompression/utils/NvEncoderCLIOptions.h>
#include<VoxelCompression/utils/NvCodecUtils.h>
#include<memory>
#include<cuda.h>
#include<iostream>
#include<nvenc/CUDA/NvEncoderCuda.h>


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
    ~VoxelCompressImpl(){}
    bool compress(uint8_t* src_ptr,int64_t len,std::vector<std::vector<uint8_t>>& packets);
private:
    bool initCUDA();
    bool initEncoder();
private:
    /**
     * @brief two implication: CUDA
     */
    std::unique_ptr<NvEncoderCuda> encoder;
    VoxelCompressOptions compress_opts;

    CUcontext cu_ctx;
    int cu_device_cnt;
    CUdevice cu_device;
    int using_gpu=0;
    char using_device_name[80];
};

VoxelCompressImpl::VoxelCompressImpl(const VoxelCompressOptions &opts)
:compress_opts(opts)
{
    initCUDA();
    initEncoder();
}

bool VoxelCompressImpl::compress(uint8_t *src_ptr,int64_t len,std::vector<std::vector<uint8_t>> &packets) {
    if(src_ptr==nullptr || len<=0)
        return false;
    initEncoder();

    int64_t frame_size=encoder->GetFrameSize();//base on pixel format
    int frame_num=0;
    int64_t cur_frame_offset=0;
    int64_t compressed_size=0;
    int encode_turn=0;

    while(true){
        std::vector<std::vector<uint8_t>> tmp_packets;
        if(cur_frame_offset+frame_size<=len){
            const NvEncInputFrame* encoderInputFrame = encoder->GetNextInputFrame();
            NvEncoderCuda::CopyToDeviceFrame(cu_ctx, src_ptr+cur_frame_offset, 0, (CUdeviceptr)encoderInputFrame->inputPtr,
                                             (int)encoderInputFrame->pitch,
                                             encoder->GetEncodeWidth(),
                                             encoder->GetEncodeHeight(),
                                             CU_MEMORYTYPE_HOST,
                                             encoderInputFrame->bufferFormat,
                                             encoderInputFrame->chromaOffsets,
                                             encoderInputFrame->numChromaPlanes);

            encoder->EncodeFrame(tmp_packets);
        }
        else{
            encoder->EndEncode(tmp_packets);
            std::cout<<"end encode"<<std::endl;
        }
        frame_num+=tmp_packets.size();
//        std::cout<<"encode turn: "<<encode_turn++<<std::endl;
//        std::cout<<"tmp_packets.size(): "<<tmp_packets.size()<<std::endl;
//        LOG(INFO)<<"encode turn: "<<encode_turn++<<std::endl;
//        LOG(INFO)<<"tmp_packets.size(): "<<tmp_packets.size()<<std::endl;
        cur_frame_offset+=frame_size;
        for(int i=0;i<tmp_packets.size();i++){
            compressed_size+=tmp_packets[i].size();
            packets.push_back(std::move(tmp_packets[i]));
        }
        if(cur_frame_offset>len)
            break;
    }
    std::cout<<"frame num is: "<<frame_num<<std::endl;
    std::cout<<"compressed size is: "<<compressed_size<<std::endl;
    encoder.reset();
    return true;
}

bool VoxelCompressImpl::initCUDA() {
    checkCUDAErrors(cuInit(0));
    cu_device_cnt=0;
    checkCUDAErrors(cuDeviceGetCount(&cu_device_cnt));
    checkCUDAErrors(cuDeviceGet(&cu_device,using_gpu));
    checkCUDAErrors(cuDeviceGetName(using_device_name,sizeof(using_device_name),cu_device));
    std::cout<<"GPU in use: "<<using_device_name<<std::endl;
    cu_ctx=nullptr;
    checkCUDAErrors(cuCtxCreate(&cu_ctx,0,cu_device));


    return true;
}

bool VoxelCompressImpl::initEncoder() {
    encoder=std::make_unique<NvEncoderCuda>(cu_ctx,compress_opts.width,compress_opts.height,compress_opts.input_buffer_format);

    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

    initializeParams.encodeConfig = &encodeConfig;
    auto& encode_opts=compress_opts.encode_init_params;
    encoder->CreateDefaultEncoderParams(&initializeParams, encode_opts.GetEncodeGUID(), encode_opts.GetPresetGUID(), encode_opts.GetTuningInfo());
    encode_opts.SetInitParams(&initializeParams, compress_opts.input_buffer_format);

    encoder->CreateEncoder(&initializeParams);

    return true;
}

VoxelCompress::VoxelCompress(const VoxelCompressOptions& opts) {
    impl=std::make_unique<VoxelCompressImpl>(opts);
}

bool VoxelCompress::compress(uint8_t* src_ptr,int64_t len,std::vector<std::vector<uint8_t>>& packets) {
    return impl->compress(src_ptr,len,packets);
}

VoxelCompress::~VoxelCompress()
{

}

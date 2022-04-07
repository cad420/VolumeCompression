//
// Created by wyz on 2022/3/23.
//
#include <iostream>
#include <string>
#include <cassert>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include <fstream>
#include <vector>

#include <chrono>
#include <iostream>

#include <VoxelCompression/voxel_uncompress/VoxelUncompress.h>
#include <VoxelCompression/voxel_compress/VoxelCompress.h>
#include <VoxelCompression/voxel_compress/VoxelCmpDS.h>
#define START_TIMER auto __start = std::chrono::steady_clock::now();

#define STOP_TIMER(msg) \
auto __end = std::chrono::steady_clock::now();\
auto __t = std::chrono::duration_cast<std::chrono::milliseconds>(__end-__start);\
std::cout<<msg<<" cost time: "<<__t.count()<<"ms"<<std::endl;

void RightShiftArrayOfUInt16(uint16_t* wp,size_t n,uint8_t k)
{
    assert(sizeof(uint16_t) == 2);
    while( n --> 0 ){
        (*wp) >>= k;
        wp++;
    }
}
void LeftShiftArrayOfUInt16(uint16_t* wp,size_t n,uint8_t k)
{
    assert(sizeof(uint16_t) == 2);
    while( n --> 0 ){
        (*wp) <<= k;
        wp++;
    }
}

void encode(AVCodecContext* c,AVFrame* frame,AVPacket* pkt,std::vector<std::vector<uint8_t>>& packets,size_t& packets_size){
    int ret = avcodec_send_frame(c,frame);

    if(ret < 0){
        std::cerr<<"error sending a frame for encoding"<<std::endl;
        exit(1);
    }

    while(ret >= 0){
        ret = avcodec_receive_packet(c,pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break;
        }
        else if(ret < 0){
            std::cerr<<"error encoding"<<std::endl;
            exit(1);
        }
        packets_size += pkt->size;
        std::vector<uint8_t> tmp(pkt->size);
        memcpy(tmp.data(),pkt->data,pkt->size);
        packets.push_back(std::move(tmp));
        av_packet_unref(pkt);
    }
}

#define VoxelT uint8_t

size_t decode(AVCodecContext* c,AVFrame* frame,AVPacket* pkt,uint8_t* buf){
    int ret = avcodec_send_packet(c,pkt);
    if(ret < 0){
        std::cerr<<"error sending a packet for decoding"<<std::endl;
        exit(1);
    }
    size_t frame_pos = 0;
    while(ret >= 0){
        ret = avcodec_receive_frame(c,frame);
        if(ret == AVERROR(EAGAIN) || ret ==AVERROR_EOF)
            break;
        else if(ret < 0){
            std::cerr<<"error during decoding"<<std::endl;
            exit(1);
        }
        memcpy(buf + frame_pos,frame->data[0],frame->linesize[0]*frame->height);
        frame_pos += frame->linesize[0] * frame->height;
    }
    return frame_pos;
}

void Decode(std::vector<std::vector<uint8_t>>& packets,std::vector<VoxelT>& dst){
    auto codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    assert(codec);
    auto parser = av_parser_init(codec->id);
    assert(parser);

    auto c = avcodec_alloc_context3(codec);
    c->thread_count = 16;
    c->delay = 0;
    assert(c);
    int ret = avcodec_open2(c,codec,nullptr);
    assert(ret >= 0);
    auto frame = av_frame_alloc();
    assert(frame);
    auto pkt = av_packet_alloc();
    assert(pkt);
    uint8_t* p =(uint8_t*)dst.data();
    size_t offset = 0;
    START_TIMER
    for(auto& packet:packets){
        pkt->data = packet.data();
        pkt->size = packet.size();
        offset += decode(c,frame,pkt,p+offset);
    }
    decode(c,frame,nullptr,p+offset);
    STOP_TIMER("decode")
}

const std::string filename = "D:/backpack/0_0_0.raw";
const int raw_x = 512;
const int raw_y = 512;
const int raw_z = 512;
const int voxel_size = 2;

void Encode(std::vector<VoxelT>& volume,std::vector<std::vector<uint8_t>>& packets,size_t packets_size){
    const auto codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if(!codec){
        std::cerr<<"not find codec"<<std::endl;
        exit(1);
    }
    AVCodecContext* c = avcodec_alloc_context3(codec);
    if(!c){
        std::cerr<<"alloc video codec context failed"<<std::endl;
        exit(1);
    }
    AVPacket* pkt = av_packet_alloc();
    if(!pkt){
        exit(1);
    }
//    c->bit_rate = 400000;
    c->width = raw_x;
    c->height = raw_y;
    c->time_base = {1,30};
    c->framerate = {30,1};
//    c->gop_size = 11;
    c->max_b_frames = 1;
    c->bits_per_raw_sample = sizeof(VoxelT) * 8;
//    c->level = 3;
//    c->thread_count = 10;
    c->pix_fmt = AV_PIX_FMT_YUV420P12;//AV_PIX_FMT_YUV420P12LE
    av_opt_set(c->priv_data,"preset","medium",0);
    av_opt_set(c->priv_data,"tune","fastdecode",0);
    int ret = avcodec_open2(c,codec,nullptr);
    if(ret<0){
        std::cerr<<"open codec failed"<<std::endl;
        exit(1);
    }
    AVFrame* frame = av_frame_alloc();
    if(!frame){
        std::cerr<<"frame alloc failed"<<std::endl;
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;
    ret = av_frame_get_buffer(frame,0);
    std::cout<<"linesize "<<frame->linesize[0]<<std::endl;
    std::cout<<"linesize "<<frame->linesize[1]<<std::endl;
    std::cout<<"linesize "<<frame->linesize[2]<<std::endl;
    std::cout<<"linesize "<<frame->linesize[3]<<std::endl;
    if(ret<0){
        std::cerr<<"could not alloc frame data"<<std::endl;
        exit(1);
    }
    //    LeftShiftArrayOfUInt16(volume.data(),voxel_count,6);
    for(int i = 0;i<raw_z;i++){
        ret = av_frame_make_writable(frame);

        if(ret<0){
            std::cerr<<"can't make frame write"<<std::endl;
            exit(1);
        }

        VoxelT * p = (VoxelT*)frame->data[0];

        for(int y = 0;y<raw_y;y++){
            for(int x = 0;x<raw_x;x++){
                p[y * raw_y + x] = volume[i * raw_x * raw_y + y * raw_y + x];
            }
        }
        frame->pts = i;

        encode(c,frame,pkt,packets,packets_size);
    }
    encode(c,nullptr,pkt,packets,packets_size);

    std::cout<<"packets count: "<<packets.size()<<" , size: "<<packets_size<<std::endl;
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

void NVDecode(std::vector<std::vector<uint8_t>>& packets,std::vector<VoxelT>& dst){
    VoxelUncompressOptions opt;
    opt.codec_method = cudaVideoCodec_HEVC;
    opt.use_device_frame_buffer = false;
    VoxelUncompress decoder(opt);
    START_TIMER
    decoder.uncompress(reinterpret_cast<uint8_t*>(dst.data()),dst.size()*sizeof(VoxelT),packets);
    STOP_TIMER("nv decode")
}
void NVEncode(std::vector<VoxelT>& volume,std::vector<std::vector<uint8_t>>& packets,size_t packets_size){
    VoxelCompressOptions opt;
    opt.width = raw_x;
    opt.height = raw_y;
    opt.input_buffer_format = NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
    std::function<void(NV_ENC_INITIALIZE_PARAMS *)> f = [](NV_ENC_INITIALIZE_PARAMS* p){
      p->encodeGUID = NV_ENC_CODEC_HEVC_GUID;
      p->encodeConfig->profileGUID = NV_ENC_HEVC_PROFILE_MAIN10_GUID;
      p->encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 2;
      p->encodeConfig->encodeCodecConfig.hevcConfig.chromaFormatIDC = 1;
    };
    opt.encode_init_params= NvEncoderInitParam("",&f);
    VoxelCompress encoder(opt);
    encoder.compress(reinterpret_cast<uint8_t*>(volume.data()),volume.size()*sizeof(VoxelT),packets);
}
void ReadPacket(uint32_t x,uint32_t y,uint32_t z,std::vector<std::vector<uint8_t>>& packets){
    static std::string h264_file_name = "D:/ffmpegyuv420_9p2_max_lod0_23389_29581_10296.h264";
    static sv::Reader reader(h264_file_name.c_str());
    reader.read_header();
    reader.read_packet({x,y,z},packets);
}
int main(){
    {
        std::vector<std::vector<uint8_t>> packets;
        ReadPacket(33,39,10,packets);
        size_t size = 512 * 512 *512;
        std::vector<uint8_t> volume(size,0);
        Decode(packets,volume);

        std::ofstream out("D:/mouselod0_33_39_10_uint8.raw",std::ios::binary);
        out.write(reinterpret_cast<char*>(volume.data()),size);
        out.close();
        return 0;
    }
    size_t voxel_count = raw_x * raw_y * raw_z;
    std::ifstream in(filename,std::ios::binary);
    std::vector<VoxelT> volume(voxel_count);
    in.read(reinterpret_cast<char*>(volume.data()),voxel_count * voxel_size);
    in.close();


    std::vector<std::vector<uint8_t>> packets;
    size_t packets_size = 0;

    Encode(volume,packets,packets_size);
//    NVEncode(volume,packets,packets_size);

    std::vector<VoxelT> dst(voxel_count,0);

    size_t diff_count = 0,diff_sum = 0;
    Decode(packets,dst);
//    RightShiftArrayOfUInt16(dst.data(),voxel_count,6);
    std::vector<int> table0(65536,0),table1(65536,0);
    for(int i = 0;i<voxel_count;i++){
        table0[volume[i]]++;
        table1[dst[i]]++;
        if(volume[i] != dst[i]){
            diff_count++;
            diff_sum += std::abs(volume[i] - dst[i]);
        }
    }
    std::cout<<"diff_count: "<<diff_count<<" , diff_sum: "<<diff_sum<<std::endl;
//    for(int i = 0;i<table0.size();i++){
//        std::cout<<i<<" "<<table0[i]<<" "<<table1[i]<<std::endl;
//    }
    std::string outname = "D:/backpack/ffmpeg12bitnvdecode_0_0_0_uint16.raw";
    std::ofstream out(outname,std::ios::binary);
    assert(out.is_open());
    out.write(reinterpret_cast<char*>(dst.data()),voxel_count * voxel_size);
    out.close();
    return 0;
}
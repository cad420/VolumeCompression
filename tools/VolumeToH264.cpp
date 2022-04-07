#include <VoxelCompression/utils/VolumeReader.h>
#include<VoxelCompression/voxel_compress/VoxelCmpDS.h>
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include <cmdline.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

struct Encoder{
    int encode_w = 0;
    int encode_h = 0;
    int bits_per_sample = 0;
    sv::CodecMethod codec_method = sv::UNDEFINED;
    size_t get_frame_size(){
        return encode_w * encode_h * bits_per_sample / 8;
    }
    size_t compress(uint8_t* src,size_t size,std::vector<std::vector<uint8_t>>& packets){
        try{
            if(!encode_w || !encode_h || !bits_per_sample || codec_method == sv::UNDEFINED){
                throw std::runtime_error("invalid compress params");
            }

            const auto codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
            if(!codec){
                throw std::runtime_error("not find codec");
            }
            AVCodecContext* c = avcodec_alloc_context3(codec);
            if(!c){
                throw std::runtime_error("alloc video codec context failed");
            }
            AVPacket* pkt = av_packet_alloc();
            if(!pkt){
                throw std::runtime_error("alloc packet failed");
            }
//    c->bit_rate = 400000;
            c->width = encode_w;
            c->height = encode_h;
            c->time_base = {1,30};
            c->framerate = {30,1};
//    c->gop_size = 11;
            c->max_b_frames = 1;
            c->bits_per_raw_sample = bits_per_sample;
//    c->level = 3;
//    c->thread_count = 10;
            if(codec_method == sv::H264_YUV420 || codec_method == sv::HEVC_YUV420)
                c->pix_fmt = AV_PIX_FMT_YUV420P;
            else if(codec_method == sv::HEVC_YUV420_12BIT)
                c->pix_fmt = AV_PIX_FMT_YUV420P12;//
            else
                throw std::runtime_error("invalid codec method or buffer format");

            av_opt_set(c->priv_data,"preset","medium",0);
            av_opt_set(c->priv_data,"tune","fastdecode",0);

            int ret = avcodec_open2(c,codec,nullptr);
            if(ret<0){
                throw std::runtime_error("open codec failed");
            }

            AVFrame* frame = av_frame_alloc();
            if(!frame){
                throw std::runtime_error("frame alloc failed");
            }
            frame->format = c->pix_fmt;
            frame->width = c->width;
            frame->height = c->height;
            ret = av_frame_get_buffer(frame,0);
            if(ret<0){
                throw std::runtime_error("could not alloc frame data");
            }

            int frame_size = get_frame_size();
            int frame_count = size / frame_size;
            size_t packets_size = 0;
            for(int i = 0;i<frame_count;i++){
                ret = av_frame_make_writable(frame);

                if(ret<0){
                    std::cerr<<"can't make frame write"<<std::endl;
                    exit(1);
                }

                memcpy(frame->data[0],src + i * frame_size,frame_size);

                frame->pts = i;

                encode(c,frame,pkt,packets,packets_size);
            }
            encode(c,nullptr,pkt,packets,packets_size);

            std::cout<<"packets count: "<<packets.size()<<" , size: "<<packets_size<<std::endl;
            avcodec_free_context(&c);
            av_frame_free(&frame);
            av_packet_free(&pkt);
            return packets_size;
        }
        catch (const std::exception& err)
        {
            std::cerr<<err.what()<<std::endl;
            packets.clear();
            return 0;
        }
    }
    void encode(AVCodecContext* c,AVFrame* frame,AVPacket* pkt,std::vector<std::vector<uint8_t>>& packets,size_t& packets_size){
        int ret = avcodec_send_frame(c,frame);

        if(ret < 0){
            throw std::runtime_error("error sending a frame for encoding");
        }

        while(ret >= 0){
            ret = avcodec_receive_packet(c,pkt);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            }
            else if(ret < 0){
                throw std::runtime_error("error encoding");
            }
            packets_size += pkt->size;
            std::vector<uint8_t> tmp(pkt->size);
            memcpy(tmp.data(),pkt->data,pkt->size);
            packets.push_back(std::move(tmp));
            av_packet_unref(pkt);
        }
    }
};


int main(int argc,char** argv){
    try{
        cmdline::parser cmd;
        cmd.add<std::string>("input",'i',"input volume file path",true);
        cmd.add<int>("raw_x",'x'," x resolution of input raw volume",true);
        cmd.add<int>("raw_y",'y'," y resolution of input raw volume",true);
        cmd.add<int>("raw_z",'z'," z resolution of input raw volume",true);
        cmd.add<int>("log",'l'," log block length",true);
        cmd.add<int>("padding",'p',"padding of a block",false,1);
        cmd.add<std::string>("output",'o',"output h264 file path",true);
        cmd.add<std::string>("voxel_type",'t',"voxel type",false,"uint8",
                             cmdline::oneof<std::string>("uint8","int8","int16","uint16","float16",
                                            "uint32","int32","float32"));
        cmd.add<uint32_t>("encode_frame_w",0,"",true,0);
        cmd.add<uint32_t>("encode_frame_h",0,"",true,0);
        cmd.parse_check(argc,argv);

        auto input = cmd.get<std::string>("input");
        auto output = cmd.get<std::string>("output");
        uint32_t raw_x = cmd.get<int>("raw_x");
        uint32_t raw_y = cmd.get<int>("raw_y");
        uint32_t raw_z = cmd.get<int>("raw_z");
        uint32_t log = cmd.get<int>("log");
        uint32_t padding = cmd.get<int>("padding");
        uint32_t block_length = std::pow(2,log);
        uint32_t frame_width = cmd.get<uint32_t>("encode_frame_w");
        uint32_t frame_height = cmd.get<uint32_t>("encode_frame_h");
        uint32_t voxel_type = VOXEL_UNKNOWN;
        auto vt = cmd.get<std::string>("voxel_type");
        if(vt == "uint8"){
            voxel_type = VOXEL_UINT8;
        }
        else if(vt == "int8"){
            voxel_type = VOXEL_INT8;
        }
        else if(vt == "uint16"){
            voxel_type = VOXEL_UINT16;
        }
        else if(vt == "int16"){
            voxel_type = VOXEL_INT16;
        }
        else if(vt == "float16"){
            voxel_type = VOXEL_FLOAT16;
        }
        else if(vt =="uint32"){
            voxel_type = VOXEL_UINT32;
        }
        else if(vt == "int32"){
            voxel_type = VOXEL_INT32;
        }
        else if(vt == "float32"){
            voxel_type = VOXEL_FLOAT32;
        }
        BlockVolumeReader reader(
            {input,raw_x,raw_y,raw_z,sv::GetVoxelSize(voxel_type),block_length,padding}
            );
        auto dim = reader.getDim();

        sv::Header header{};
        header.identifier = VOXEL_COMPRESS_FILE_IDENTIFIER;
        header.version = VOXEL_COMPRESS_VERSION;
        header.raw_x = raw_x;
        header.raw_y = raw_y;
        header.raw_z = raw_z;
        header.block_dim_x = dim[0];
        header.block_dim_y = dim[1];
        header.block_dim_z = dim[2];
        header.log_block_length = log;
        header.padding = padding;
        header.frame_width = frame_width;
        header.frame_height = frame_height;
        header.voxel = voxel_type;

        Encoder encoder{};
        encoder.encode_w = frame_width;
        encoder.encode_h = frame_height;
        encoder.bits_per_sample = sv::GetVoxelSize(voxel_type) * 8;
        if(encoder.bits_per_sample == 8){
            encoder.codec_method = sv::HEVC_YUV420;
        }
        else if(encoder.bits_per_sample == 16){
            encoder.codec_method = sv::HEVC_YUV420_12BIT;
        }

        sv::Writer writer(output.c_str());
        writer.write_header(header);

        reader.startRead();
        while(!reader.isEmpty()){
            auto block = reader.getBlock();
            std::vector<std::vector<uint8_t >> packets;
            encoder.compress(block.data.data(),block.data.size(),packets);
            writer.write_packet(block.index,packets);
        }
    }
    catch (const std::exception& err)
    {
        std::cout<<err.what()<<std::endl;
        return -1;
    }
    return 0;
}
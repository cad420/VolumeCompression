//
// Created by wyz on 2022/3/20.
//
#include <iomanip>
#include <cmdline.hpp>
#include <VoxelCompression/utils/VolumeReader.h>
#include <VoxelCompression/utils/RawStream.hpp>
#include <memory>
#include <VoxelCompression/voxel_compress/VoxelCmpDS.h>
#include <omp.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}
template<typename T,typename F>
void Resample(T* src1,T* src2,F&& f,uint32_t ox,uint32_t oy,TIFOutStream& out){
    uint32_t nx = (ox + 1) / 2;
    uint32_t ny = (oy + 1) / 2;
    T* buf = (T*)::operator new(nx * sizeof(T));
    assert(buf);
    for(int y = 0; y < ny; y++){
#pragma omp parallel for
        for(int x = 0; x < nx; x++){
            T t1 = f(f(src1[ox * y * 2 + x * 2],(x*2+2 > ox) ? 0 : src1[ox * y * 2 + x * 2 + 1]),
                     f((y*2+2 > oy)?0 : src1[ox * (y * 2 + 1) + x * 2],(x*2+2 > ox || y*2+2 > oy) ? 0 : src1[ox * (y * 2 + 1) + x * 2 + 1]));
            T t2 = f(f(src2[ox * y * 2 + x * 2],(x*2+2 > ox) ? 0 : src2[ox * y * 2 + x * 2 + 1]),
                     f((y*2+2 > oy)?0 : src2[ox * (y * 2 + 1) + x * 2],(x*2+2 > ox || y*2+2 > oy) ? 0 : src2[ox * (y * 2 + 1) + x * 2 + 1]));
            buf[x] = f(t1,t2);
        }

        out.writeRow(buf,y,1);
    }

    ::operator delete(buf);

}
template <typename T,typename = void>
struct AvgResample{
    static T resample(T t1, T t2){
        return (t1+t2)/2;
    }
};
template <typename T>
struct AvgResample<T,std::enable_if_t<!std::is_floating_point_v<T>>>{
    static T resample(T t1, T t2){
        return t1 + ((t2 - t1) >> 2);
    }
};

template <typename T>
struct MaxResample{
    static T resample(T t1, T t2){
        return (std::max)(t1,t2);
    }
};

#define ResampleHelper(Type)                                                                                           \
    std::function<Type(Type, Type)> f;                                                                                 \
    if (resample_method == "max")                                                                                      \
    {                                                                                                                  \
        f = MaxResample<Type>::resample;                                                                               \
    }                                                                                                                  \
    else if (resample_method == "avg")                                                                                 \
    {                                                                                                                  \
        f = AvgResample<Type>::resample;                                                                               \
    }                                                                                                                  \
    Resample<Type>(reinterpret_cast<Type *>(src1), reinterpret_cast<Type *>(src2), f, raw_x, raw_y, out);


uint32_t GetVoxel(const std::string& vt){
    uint32_t voxel_type = VOXEL_UNKNOWN;
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
    return voxel_type;
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

    cmdline::parser cmd;

    cmd.add<std::string>("input",'i',"input tif image directory",false,"./");
    cmd.add<std::string>("save_dir",'s',"resampled tif image save directory",false,"./");
    cmd.add<std::string>("output",'o',"output h264 file path",true);
    cmd.add<std::string>("prefix",0,"prefix for tif image name",false,"");
    cmd.add<std::string>("postfix",0,"postfix for tif image name",false,"");
    cmd.add<std::string>("tag",0,"tag for resampled tif image name after postfix to avoid same name in same directory",true);
    cmd.add<uint32_t>("name_fill",'f',"length of string which generated by z-index",true);
    cmd.add<uint32_t>("raw_x",'x',"volume data width",true);
    cmd.add<uint32_t>("raw_y",'y',"volume data height",true);
    cmd.add<uint32_t>("raw_z",'z',"volume data depth",true);
    cmd.add<uint32_t>("log",'l',"block length log value",true);
    cmd.add<uint32_t>("padding",'p',"block padding",true);
    cmd.add<std::string>("voxel_type",'v',"voxel type",true,"unknown");
    cmd.add<uint32_t>("rows_per_strip",0,"rows per strip for tif image",false,6);
    cmd.add<std::string>("resample_method",'m',"resample method",true,"max",cmdline::oneof<std::string>("max","avg"));
    cmd.add<uint32_t>("begin_z",0,"volume read z-index begin(include)",false,0);
    cmd.add<uint32_t>("end_z",0,"volume read z-index end(not include)",false,uint32_t(-1));
    cmd.add<uint32_t>("compress",'c',"0/1 represent if tif saved with compression",false,0,cmdline::oneof<uint32_t>(0,1));
    cmd.add<uint32_t>("save_tif",0,"0/1 represent if save tif",true,0,cmdline::oneof<uint32_t>(0,1));
    cmd.add<uint32_t>("encode_frame_w",0,"",true);
    cmd.add<uint32_t>("encode_frame_h",0,"",true);
    cmd.parse_check(argc,argv);

    std::string input = cmd.get<std::string>("input");
    if(input.back()!='/' && input.back() != '\\') input += '/';
    std::string save_dir = cmd.get<std::string>("save_dir");//only save for tif
    if(save_dir.back()!='/' && save_dir.back() != '\\') save_dir += '/';
    std::string output = cmd.get<std::string>("output");
    std::string prefix = cmd.get<std::string>("prefix");
    std::string postfix = cmd.get<std::string>("postfix");
    std::string tag = cmd.get<std::string>("tag");//for save use
    uint32_t name_fill = cmd.get<uint32_t>("name_fill");
    uint32_t padding = cmd.get<uint32_t>("padding");
    uint32_t log = cmd.get<uint32_t>("log");
    uint32_t raw_x = cmd.get<uint32_t>("raw_x");
    uint32_t raw_y = cmd.get<uint32_t>("raw_y");
    uint32_t raw_z = cmd.get<uint32_t>("raw_z");
    uint32_t voxel = GetVoxel(cmd.get<std::string>("voxel_type"));
    uint32_t ele_size = sv::GetVoxelSize(voxel);
    uint32_t rows_per_strip = cmd.get<uint32_t>("rows_per_strip");
    std::string resample_method = cmd.get<std::string>("resample_method");
    //[begin_z,end_z)
    uint32_t begin_z = cmd.get<uint32_t>("begin_z");
    uint32_t end_z = cmd.get<uint32_t>("end_z");
    if(end_z == uint32_t(-1)) end_z = raw_z;

    bool compress = cmd.get<uint32_t>("compress");
    bool save_tif = cmd.get<uint32_t>("save_tif");

    uint32_t frame_width = cmd.get<uint32_t>("encode_frame_w");
    uint32_t frame_height = cmd.get<uint32_t>("encode_frame_h");

    uint32_t block_length = 1 << log;
    uint32_t no_padding_block_length = block_length - padding * 2;
    uint32_t x_count = (raw_x + no_padding_block_length - 1) / no_padding_block_length;
    uint32_t y_count = (raw_y + no_padding_block_length - 1) / no_padding_block_length;
    uint32_t xy_count = x_count * y_count;
    uint32_t turn = (begin_z + no_padding_block_length - 1) / no_padding_block_length * y_count;
    uint32_t count = (end_z - begin_z + no_padding_block_length - 1) / no_padding_block_length * y_count;
    //check and print infos
    {
        bool ok = true;
        if(begin_z % no_padding_block_length){
            std::cerr<<"begin_z should be divided by no padding block length"<<std::endl;
            ok = false;
        }
        if(end_z != uint32_t(-1) && end_z != raw_z && end_z % no_padding_block_length){
            std::cerr<<"end_z should be raw_z or divided by no padding block length"<<std::endl;
            ok = false;
        }

        if(!ele_size){
            std::cerr<<"invalid voxel size equals to zero"<<std::endl;
            ok = false;
        }

        if(padding >= block_length /4 ){
            std::cerr<<"padding to large for block length"<<std::endl;
            ok = false;
        }

        std::cout<<"[Program infos]"
            <<"\n\traw_xyz: ("<<raw_x<<", "<<raw_y<<", "<<raw_z<<")"
            <<"\n\tblock length: "<<block_length<<" padding: "<<padding
            <<"\n\tvoxel size: "<<ele_size
            <<"\n\tresample method: "<<resample_method
            <<"\n\tz-region: "<<begin_z<<" - "<<end_z
            <<"\n\tcompress: "<<(compress?"yes":"no")
            <<"\n\tbatch read count: "<<count
            <<std::endl;

        if(!ok){
            exit(-1);
        }
    }

    auto volume_stream = std::make_unique<RawStream>(raw_x,raw_y,raw_z,ele_size);

    if(save_tif){
        volume_stream->setTifReadAndResampleWrite();
    }

    volume_stream->setNamePattern([=](uint32_t z)->std::string{
        std::stringstream ss;
        ss<<input<<prefix<<std::setw(name_fill)<<std::setfill('0')<<std::to_string(z+1)<<postfix<<".tif";
        return ss.str();
    },[=](uint32_t z)->std::string{
          std::stringstream ss;
          ss<<save_dir<<prefix<<std::setw(name_fill)<<std::setfill('0')<<std::to_string(z+1)<<postfix<<tag<<".tif";
          return ss.str();
                                  });

    std::function<void(void*,void*,const std::string&)> resample_func = [=](void* src1,void* src2,const std::string& filename){
        uint32_t nx = (raw_x + 1) /2;
        uint32_t ny = (raw_y + 1) / 2;
        TIFOutStream out;
        bool e = out.open(filename);
        if(!e){
            std::cerr<<"write file "<<filename<<" failed!"<<std::endl;
            return;
        }
        out.setTIFInfo(
            {nx,ny,ele_size * 8,rows_per_strip,compress}
            );
        if(voxel == VOXEL_UINT8){
            ResampleHelper(uint8_t);
        }
        else if(voxel == VOXEL_UINT16){
            ResampleHelper(uint16_t);
        }
        else if(voxel == VOXEL_INT8){
            ResampleHelper(int8_t);
        }
        else if(voxel == VOXEL_INT16){
            ResampleHelper(int16_t);
        }
        else if(voxel == VOXEL_UINT32){
            ResampleHelper(uint32_t);
        }
        else if(voxel == VOXEL_INT32){
            ResampleHelper(int32_t);
        }
        else if(voxel == VOXEL_FLOAT32){
            ResampleHelper(float_t);
        }
        out.close();
        std::cout<<"saved tif "<<filename<<std::endl;
    };

    volume_stream->setResampleFunc(std::move(resample_func));

    BlockVolumeReader reader(
        {"",raw_x,raw_y,raw_z,sv::GetVoxelSize(voxel),block_length,padding}
    );
    reader.set_raw_stream(std::move(volume_stream));

    reader.set_batch_read_turn_and_count(turn,count);


    sv::Header header{};
    header.identifier = VOXEL_COMPRESS_FILE_IDENTIFIER;
    header.version = VOXEL_COMPRESS_VERSION;
    header.raw_x = raw_x;
    header.raw_y = raw_y;
    header.raw_z = raw_z;
    header.block_dim_x = reader.getDim()[0];
    header.block_dim_y = reader.getDim()[1];
    header.block_dim_z = reader.getDim()[2];
    header.log_block_length = log;
    header.padding = padding;
    header.frame_width = frame_height;
    header.frame_height = frame_width;
    header.codec_method = 0;
    header.voxel = voxel;

    Encoder encoder{};
    encoder.encode_w = frame_width;
    encoder.encode_h = frame_height;
    encoder.bits_per_sample = sv::GetVoxelSize(voxel) * 8;
    if(encoder.bits_per_sample == 8){
        encoder.codec_method = sv::HEVC_YUV420;
    }
    else if(encoder.bits_per_sample == 16){
        encoder.codec_method = sv::HEVC_YUV420_12BIT;
    }

    sv::Writer writer(output.c_str());
    writer.write_header(header);

    try{
        reader.startRead();
        auto read_count = count * x_count;
        size_t encode_file_size = 0;
        while(!reader.isEmpty()){
            auto block = reader.getBlock();
            std::vector<std::vector<uint8_t>> packets;
            encode_file_size += encoder.compress(block.data.data(),block.data.size(),packets);
            writer.write_packet(block.index,packets);
            read_count--;
            std::cout<<"remain read block count: "<<read_count<<std::endl;
        }
        if(read_count){
            std::cerr<<"error: read count not get to zero! remain is: "<<read_count<<std::endl;
        }
        std::cout<<"encode file size: "<<encode_file_size<<std::endl;
    }
    catch (const std::exception& err)
    {
        std::cerr<<err.what()<<std::endl;
    }
    return 0;
}
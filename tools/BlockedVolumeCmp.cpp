//
// Created by wyz on 2021/3/8.
//

#include<iostream>
#include<VoxelCompression/voxel_compress/VoxelCmpDS.h>
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include<VolumeReader/volume_reader.h>
#include<string>
int main(int argc,char** argv){

    const char* out_file_name="E:/mouse_23389_29581_10296_9p2.h264";
    uint32_t m_raw_x=23389;
    uint32_t m_raw_y=29581;
    uint32_t m_raw_z=10296;
    uint32_t m_log_block_length=9;
    uint32_t m_padding=2;
    uint32_t m_frame_width=512;
    uint32_t m_frame_height=512;
    uint64_t m_codec_method=1;

    VoxelCompressOptions opts;
    opts.width=m_frame_width;
    opts.height=m_frame_height;
    VoxelCompress cmp(opts);


    sv::Header header;
    header.identifier=VOXEL_COMPRESS_FILE_IDENTIFIER;
    header.version=VOXEL_COMPRESS_VERSION;
    header.raw_x=m_raw_x;
    header.raw_y=m_raw_y;
    header.raw_z=m_raw_z;
    header.block_dim_x=47;
    header.block_dim_y=59;
    header.block_dim_z=21;
    header.log_block_length=m_log_block_length;
    header.padding=m_padding;
    header.frame_width=m_frame_width;
    header.frame_height=m_frame_height;
    header.codec_method=m_codec_method;

    sv::Writer writer(out_file_name);
    writer.write_header(header);
    std::cout<<header<<std::endl;

    RawVolumeReader raw_reader;
    std::string file_dir="E:/mouse23389x29581x10296_512_2/";
    std::vector<uint8_t> buffer;
    buffer.assign(512*512*512,0);
    for(uint32_t z=0;z<header.block_dim_z;z++){
        for(uint32_t y=0;y<header.block_dim_y;y++){
            for(uint32_t x=0;x<header.block_dim_x;x++){
                std::string name=file_dir+std::to_string(x)+"_"+std::to_string(y)+"_"+std::to_string(z);
                raw_reader.read(buffer,name.c_str(),512,512,512);
                std::vector<std::vector<uint8_t >> packets;
                cmp.compress(buffer.data(),buffer.size(),packets);
                writer.write_packet({x,y,z},packets);
                std::cout<<"finish :"<<name<<std::endl;
            }
        }
    }

    return 0;
}
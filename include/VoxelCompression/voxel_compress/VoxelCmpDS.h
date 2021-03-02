//
// Created by wyz on 2021/3/1.
//

#ifndef VOXELCOMPRESSION_VOXELCMPDS_H
#define VOXELCOMPRESSION_VOXELCMPDS_H
#include<iostream>
#define VOXEL_COMPRESS_FILE_IDENTIFIER 0x123456
#define VOXEL_COMPRESS_VERSION 1
namespace sv{
    struct Header{
        uint64_t identifier;
        uint64_t version;
        uint32_t raw_x;
        uint32_t raw_y;
        uint32_t raw_z;
        uint32_t block_dim_x;
        uint32_t block_dim_y;
        uint32_t block_dim_z;
        uint32_t log_block_length;
        uint32_t padding;
        uint32_t frame_width;
        uint32_t frame_height;
        uint64_t codec_method;
        friend std::ostream& operator<<(std::ostream& os,const Header& header){
            os<<"[Header Info]"
            <<"\n[identifier]: "<<header.identifier
            <<"\n[version]: "<<header.version
            <<"\n[raw_x,raw_y,raw_z]: "<<header.raw_x<<" "<<header.raw_y<<" "<<header.raw_z
            <<"\n[block dim]: "<<header.block_dim_x<<" "<<header.block_dim_y<<" "<<header.block_dim_z
            <<"\n[log_block_length]: "<<header.log_block_length
            <<"\n[padding]: "<<header.padding
            <<"\n[frame_width]: "<<header.frame_width
            <<"\n[frame_height]: "<<header.frame_height
            <<"\n[codec_method]: "<<header.codec_method<<std::endl;
            return os;
        }
    };
    struct Index{
        uint32_t index_x;
        uint32_t index_y;
        uint32_t index_z;
        uint32_t reserve;
        uint64_t offset;
        uint64_t size;
        friend std::ostream& operator<<(std::ostream& os,const Index& index){
            os<<"[Index Info]"
            <<"\n[index_x,index_y,index_z]: "<<index.index_x<<" "<<index.index_y<<" "<<index.index_z
            <<"\n[reserve]: "<<index.reserve
            <<"\n[offset]: "<<index.offset
            <<"\n[size]: "<<index.size<<std::endl;
            return os;
        }
    };
    struct Data{
        uint32_t frame_size;
        //std::vector<uint8_t> frame_data;
    };

}



#endif //VOXELCOMPRESSION_VOXELCMPDS_H

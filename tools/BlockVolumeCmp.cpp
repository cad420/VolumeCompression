//
// Created by wyz on 2021/3/1.
//

#include<iostream>
#include<VoxelCompression/voxel_compress/VoxelCmpDS.h>
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include<VolumeReader/volume_reader.h>


class Writer{
public:
    Writer(const char* file_name){
        out.open(file_name,std::ios::out|std::ios::binary);
        if(!out.is_open()){
            std::cout<<"File open failed!"<<std::endl;
        }
        header_beg=0;
        index_beg=0;
        data_beg=0;
        index_offset=0;
        data_offset=0;
    }
    ~Writer(){
        out.close();
    }
    void write_header(const sv::Header& header){
        uint64_t block_num=header.block_dim_x*header.block_dim_y*header.block_dim_z;
        this->index_beg=sizeof(header);
        this->data_beg=block_num*sizeof(sv::Index)+this->index_beg;
        out.seekp(std::ios::beg);
        out.write(reinterpret_cast<const char*>(&header),sizeof(header));
    }

    void write_packet(const std::array<uint32_t,3>& index,const std::vector<std::vector<uint8_t>>& packet){
        uint64_t packet_size=0;
        out.seekp(std::ios::beg+data_beg+data_offset);
        for(size_t i=0;i<packet.size();i++){
            packet_size+=sizeof(uint64_t)+packet[i].size();
            uint64_t frame_size=packet[i].size();
            out.write(reinterpret_cast<char*>(&frame_size),sizeof(uint64_t));
            out.write(reinterpret_cast<const char*>(packet[i].data()),frame_size);
        }

        sv::Index idx{index[0],index[1],index[2],(uint32_t)packet.size(),data_offset,packet_size};
        out.seekp(std::ios::beg+index_beg+index_offset);
        out.write(reinterpret_cast<const char*>(&idx),sizeof(idx));
        index_offset+=sizeof(idx);
        data_offset+=packet_size;
    }

private:
    std::fstream out;
    uint64_t header_beg;
    uint64_t index_beg;
    uint64_t data_beg;
    uint64_t index_offset;
    uint64_t data_offset;
};
int main(int argc,char** argv)
{
    const char* in_file_name="aneurism_256_256_256_uint8.raw";
    const char* out_file_name="aneurism_256_256_256_7p1.h264";
    uint32_t m_raw_x=256;
    uint32_t m_raw_y=256;
    uint32_t m_raw_z=256;
    uint32_t m_log_block_length=7;
    uint32_t m_padding=1;
    uint32_t m_frame_width=256;
    uint32_t m_frame_height=256;
    uint64_t m_codec_method=1;

    uint32_t m_block_length=std::pow(2,m_log_block_length);
    BlockVolumeReader reader;
    reader.setupRawVolumeInfo({in_file_name,"",m_raw_x,m_raw_y,m_raw_z,m_block_length,m_padding});

    VoxelCompressOptions opts;
    opts.width=256;
    opts.height=256;
    VoxelCompress cmp(opts);



    sv::Header header;
    header.identifier=VOXEL_COMPRESS_FILE_IDENTIFIER;
    header.version=VOXEL_COMPRESS_VERSION;
    header.raw_x=m_raw_x;
    header.raw_y=m_raw_y;
    header.raw_z=m_raw_z;
    auto dim=reader.get_dim();
    header.block_dim_x=dim[0];
    header.block_dim_y=dim[1];
    header.block_dim_z=dim[2];
    header.log_block_length=m_log_block_length;
    header.padding=m_padding;
    header.frame_width=m_frame_width;
    header.frame_height=m_frame_height;
    header.codec_method=m_codec_method;

    Writer writer(out_file_name);
    writer.write_header(header);

    reader.start_read();
    while(!reader.is_read_finish() || !reader.isBlockWareHouseEmpty()){
        std::vector<uint8_t> block_data;
        std::array<uint32_t,3> block_index;
        reader.get_block(block_data,block_index);
        std::vector<std::vector<uint8_t >> packets;
        cmp.compress(block_data.data(),block_data.size(),packets);
        writer.write_packet(block_index,packets);
    }

    return 0;
}
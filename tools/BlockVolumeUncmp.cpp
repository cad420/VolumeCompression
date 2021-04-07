//
// Created by wyz on 2021/3/1.
//
#include<iostream>
#include<fstream>
#include<VoxelCompression/voxel_compress/VoxelCmpDS.h>
#include<VoxelCompression/voxel_uncompress/VoxelUncompress.h>
#include<vector>
#include<array>
class Reader{
public:
    Reader(const char* file_name){
        in.open(file_name,std::ios::in|std::ios::binary);
        if(!in.is_open()){
            std::cout<<"File open failed!"<<std::endl;
        }
    }

    void read_header(sv::Header& header){
        in.seekg(std::ios::beg);
        in.read(reinterpret_cast<char*>(&header),sizeof(header));
        std::cout<<header;
        index_beg=sizeof(header);
        block_num=header.block_dim_x*header.block_dim_y*header.block_dim_z;
        data_beg=index_beg+block_num*sizeof(sv::Index);
        reader_indexes(this->indexes);
    }
    void reader_indexes(std::vector<sv::Index>& indexes){
        if(indexes.size()!=block_num){
            indexes.resize(block_num);
        }
        in.seekg(std::ios::beg+index_beg);
        for(size_t i=0;i<block_num;i++){
            in.read(reinterpret_cast<char*>(&indexes[i]),sizeof(sv::Index));
            std::cout<<indexes[i];
        }
    }
    void read_packet(const std::array<uint32_t,3>& index,std::vector<std::vector<uint8_t>>& packet){
        for(auto& it:indexes){
            if(it.index_x==index[0] && it.index_y==index[1] && it.index_z==index[2]){
                std::cout<<"find!!!"<<std::endl;
                in.seekg(std::ios::beg+data_beg+it.offset);
                uint64_t offset=0,frame_size=0;
                while(offset<it.size){
                    in.read(reinterpret_cast<char*>(&frame_size),sizeof(uint64_t));
                    std::cout<<"frame_size: "<<frame_size<<std::endl;
                    std::vector<uint8_t> tmp;
                    tmp.resize(frame_size);
                    in.read(reinterpret_cast<char*>(tmp.data()),frame_size);
                    packet.push_back(tmp);
                    offset+=sizeof(uint64_t)+frame_size;
                }
                return;
            }
        }
    }
private:
    std::vector<sv::Index> indexes;
    uint32_t block_num;
    uint64_t index_beg;
    uint64_t data_beg;
    std::fstream in;
};

int main(int argc,char** argv)
{
    const char* in_file_name="aneurism_256_256_256_7p1.h264";
    sv::Header header;
//    std::vector<sv::Index> indexes;
    Reader reader(in_file_name);
    reader.read_header(header);
//    reader.reader_indexes(indexes);
    VoxelUncompressOptions opts;
    opts.width=header.frame_width;
    opts.height=header.frame_height;

    VoxelUncompress un_cmp(opts);
    std::vector<uint8_t> buffer;
    uint64_t block_length=std::pow(2,header.log_block_length);
    buffer.resize(block_length*block_length*block_length);
    std::vector<std::vector<uint8_t>> packet;
    reader.read_packet({1,1,1},packet);
    un_cmp.uncompress(buffer.data(),buffer.size(),packet);
    std::fstream out("test000_128_128_128_uint8.raw",std::ios::binary|std::ios::out);
    if(!out.is_open()){
        std::cout<<"file open failed"<<std::endl;
    }
    out.write(reinterpret_cast<char*>(buffer.data()),buffer.size());
    out.close();
    std::cout<<"finish"<<std::endl;
    return 0;
}
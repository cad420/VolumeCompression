//
// Created by wyz on 2021/3/1.
//

#ifndef VOXELCOMPRESSION_VOXELCMPDS_H
#define VOXELCOMPRESSION_VOXELCMPDS_H
#include<iostream>
#include<vector>
#include<fstream>
#include<array>
#include<condition_variable>
#include<mutex>
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
    class Reader{
    public:
        Reader(const char* file_name){
            in.open(file_name,std::ios::in|std::ios::binary);
            if(!in.is_open()){
                std::cout<<"File open failed!"<<std::endl;
            }
        }
        Reader(std::fstream&& in):in(std::forward<std::fstream>(in)){}
        void read_header(){
            read_header(this->header);
        }
        void read_header(sv::Header& header){
            in.seekg(std::ios::beg);
            in.read(reinterpret_cast<char*>(&header),sizeof(header));
            std::cout<<header;
            if(header.identifier!=VOXEL_COMPRESS_FILE_IDENTIFIER)\
                throw std::runtime_error("file format error!");
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
            uint64_t max_size=0;
            for(size_t i=0;i<block_num;i++){
                in.read(reinterpret_cast<char*>(&indexes[i]),sizeof(sv::Index));
//                std::cout<<indexes[i];
                if(indexes[i].size>max_size)
                    max_size=indexes[i].size;
            }
//            std::cout<<"max size: "<<max_size<<std::endl;
        }
        void read_packet(const std::array<uint32_t,3>& index,std::vector<std::vector<uint8_t>>& packet){
            {
                std::unique_lock<std::mutex> lk(mtx);
                cv.wait(lk,[](){return true;});
                std::cout<<"start read"<<std::endl;
                for(auto& it:indexes){
                    if(it.index_x==index[0] && it.index_y==index[1] && it.index_z==index[2]){
//                    std::cout<<"find!!!"<<std::endl;
//                    std::cout<<it<<std::endl;
                        in.seekg(std::ios::beg+data_beg+it.offset);
                        uint64_t offset=0,frame_size=0;
                        while(offset<it.size){
//                        std::cout<<"offset: "<<offset<<"\tsize: "<<it.size<<std::endl;
                            in.read(reinterpret_cast<char*>(&frame_size),sizeof(uint64_t));
//                        std::cout<<"frame_size: "<<frame_size<<std::endl;
                            std::vector<uint8_t> tmp;
                            tmp.resize(frame_size);
                            in.read(reinterpret_cast<char*>(tmp.data()),frame_size);
                            packet.push_back(tmp);
                            offset+=sizeof(uint64_t)+frame_size;
                        }
                        std::cout<<"finish read packet"<<std::endl;
                        break;
                    }
                }
            }
            cv.notify_one();
        }
        sv::Header get_header() const{
            return this->header;
        }
    private:
        sv::Header header;
        std::vector<sv::Index> indexes;
        uint32_t block_num;
        uint64_t index_beg;
        uint64_t data_beg;
        std::fstream in;
        std::mutex mtx;
        std::condition_variable cv;
    };

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
}



#endif //VOXELCOMPRESSION_VOXELCMPDS_H

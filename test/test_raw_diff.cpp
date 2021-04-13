//
// Created by wyz on 2021/4/13.
//
#include<fstream>
#include<iostream>
#include<string>
#include<vector>
#include<cassert>
#include<cmath>
#define raw_x 128
#define raw_y 128
#define raw_z 128
std::vector<uint8_t> open_raw(const std::string& name){
    std::ifstream in(name,std::ios::binary);
    if(!in.is_open()){
        std::cout<<"open file failed: "<<name<<std::endl;
        exit(-1);
    }
    std::vector<uint8_t> data((size_t)raw_x*raw_y*raw_z);
    in.read(reinterpret_cast<char*>(data.data()),data.size());
    return data;
}
void test_raw(const std::string& file1,const std::string& file2){
    auto data1=open_raw(file1);
    auto data2=open_raw(file2);
    assert(data1.size()==data2.size());
    int abs_diff=0;
    for(int i=0;i<data1.size();i++){
        auto diff=std::abs((int)data1[i]-(int)data2[i]);
        if(diff){
            std::cout<<"index: "<<i<<" diff: "<<diff<<" data1: "<<(int)data1[i]<<" data2: "<<(int)data2[i]<<std::endl;
        }
        abs_diff+=diff;
    }
    std::cout<<"abs_diff: "<<abs_diff<<std::endl;
}

int main(){

//    test_raw("testraw2tif_128_128_128_uint8.raw","testrsraw_128_128_128_uint8.raw");
//    test_raw("tif2raw_256_256_256_uint8.raw","aneurism_256_256_256_uint8.raw");
    test_raw("testrstif_128_128_128_uint8.raw","testrsraw_128_128_128_uint8.raw");

    return 0;
}

//
// Created by wyz on 2022/3/22.
//
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <map>
#include <VoxelCompression/voxel_compress/VoxelCompress.h>
#include <VoxelCompression/voxel_uncompress/VoxelUncompress.h>
template <typename T>
auto ReadVolume(const std::string& name,int x,int y,int z,std::vector<int>& table){
    std::ifstream in(name,std::ios::binary);
    assert(in.is_open());
    size_t size = (size_t) x * y * z;
    std::vector<T> buffer(size,0);
    in.read(reinterpret_cast<char*>(buffer.data()),size);

    for(auto t:buffer){
        table[t]++;
    }

    return buffer;
}
void SwabArrayOfShort( uint16_t* wp, size_t n)
{
     unsigned char* cp;
     unsigned char t;
    assert(sizeof(uint16_t) == 2);
    /* XXX unroll loop some */
    while (n-- > 0) {
        cp = (unsigned char*) wp;
        t = cp[1]; cp[1] = cp[0]; cp[0] = t;
        wp++;
    }
}
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
#define VoxelT uint16_t
int main(){
    std::string filename = "D:/backpack/0_0_0.raw";
    int x = 512;
    int y = 512;
    int z = 512;
    std::vector<int> table0((std::numeric_limits<VoxelT>::max)()+1,0);
    auto volume = ReadVolume<uint16_t>(filename,x,y,z,table0);


//    NvEncGetEncodeCaps()

    VoxelCompressOptions opt1;
    opt1.width = 512;
    opt1.height = 512;
    opt1.input_buffer_format = NV_ENC_BUFFER_FORMAT_YUV444_10BIT ;
    std::function<void(NV_ENC_INITIALIZE_PARAMS *)> f = [](NV_ENC_INITIALIZE_PARAMS* p){
        p->encodeGUID = NV_ENC_CODEC_HEVC_GUID;
        p->encodeConfig->profileGUID = NV_ENC_HEVC_PROFILE_MAIN10_GUID;
        p->encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 2;
        p->encodeConfig->encodeCodecConfig.hevcConfig.chromaFormatIDC = 3;
    };
    opt1.encode_init_params= NvEncoderInitParam("",&f);
    VoxelCompress encoder(opt1);
    std::vector<std::vector<uint8_t>> packets;
//    SwabArrayOfShort(volume.data(),volume.size());
//    https://docs.microsoft.com/en-us/windows/win32/medfound/10-bit-and-16-bit-yuv-video-formats
    LeftShiftArrayOfUInt16(volume.data(),volume.size(),6);
    encoder.compress(reinterpret_cast<uint8_t*>(volume.data()),volume.size()*sizeof(VoxelT),packets);

    VoxelUncompressOptions opt2;
    opt2.codec_method = cudaVideoCodec_HEVC;
    opt2.output_buffer_format = NV_ENC_BUFFER_FORMAT_YUV444_10BIT;
    VoxelUncompress decoder(opt2);

    auto buffer = volume;
    memset(buffer.data(),0,buffer.size()*sizeof(VoxelT));
    decoder.uncompress(reinterpret_cast<uint8_t*>(buffer.data()),buffer.size()*sizeof(VoxelT),packets);
    size_t _count = 0;
    for(int i = 0;i<buffer.size();i++){
        uint8_t v = *(uint8_t*)(&buffer[i]);
        if(v > 3) _count++;
    }
    std::cout<<"_count: "<<_count<<std::endl;

    RightShiftArrayOfUInt16(buffer.data(),buffer.size(),6);
//    SwabArrayOfShort(buffer.data(),buffer.size());
    std::vector<int> table((std::numeric_limits<VoxelT>::max)()+1,0);
    size_t diff_count = 0;
    size_t diff_value_sum = 0;
    for(size_t i = 0;i<buffer.size();i++){
        if(buffer[i] != volume[i]){
            diff_count++;
            diff_value_sum += std::abs(buffer[i] - volume[i]);
        }
        table[buffer[i]]++;
    }
    std::cout<<"diff count: "<<diff_count<<" , diff value sum: "<<diff_value_sum<<std::endl;
    for(size_t i = 0;i<table.size();i++){
        std::cout<<i<<" "<<table0[i]<<" "<<table[i]<<std::endl;
    }
    std::string test_outname = "D:/backpack/yuvtestout_512_512_512_uint16.raw";
    std::ofstream test_out(test_outname,std::ios::binary);
    test_out.write(reinterpret_cast<char*>(buffer.data()),buffer.size()*sizeof(uint16_t));

    return 0;
}
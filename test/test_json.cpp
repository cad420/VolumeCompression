//
// Created by wyz on 2021/4/7.
//
#include<VoxelCompression/voxel_compress/VoxelCmpDS.h>

void test_write(){
    sv::LodFile lod_file;

    lod_file.add_lod_file(0,"lod0.h264");
    lod_file.add_lod_file(1,"lod1.h264");
    lod_file.add_lod_file(2,"lod2.h264");
    lod_file.add_lod_file(3,"lod3.h264");
    lod_file.add_lod_file(4,"lod4.h264");
    lod_file.save_lod_files("test_json","json");

}
void test_read(){
    sv::LodFile lod_file;
    lod_file.open_lod_file("test_json.json");
    std::cout<<"min_lod: "<<lod_file.get_min_lod()<<" max_lod: "<<lod_file.get_max_lod()<<std::endl;
    for(int i=lod_file.get_min_lod();i<=lod_file.get_max_lod();i++){
        std::cout<<std::to_string(i)<<": "<<lod_file.get_lod_file_path(i)<<std::endl;
    }
}
int main(int argc,char** argv)
{
    test_write();
    test_read();

    return 0;
}

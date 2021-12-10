#include <VoxelCompression/utils/VolumeReader.h>
#include<VoxelCompression/voxel_compress/VoxelCmpDS.h>
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include <cmdline.hpp>

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

        cmd.parse_check(argc,argv);

        auto input = cmd.get<std::string>("input");
        auto output = cmd.get<std::string>("output");
        uint32_t raw_x = cmd.get<int>("raw_x");
        uint32_t raw_y = cmd.get<int>("raw_y");
        uint32_t raw_z = cmd.get<int>("raw_z");
        uint32_t log = cmd.get<int>("log");
        uint32_t padding = cmd.get<int>("padding");
        uint32_t block_length = std::pow(2,log);
        uint32_t frame_width = block_length;
        uint32_t frame_height = block_length;

        BlockVolumeReader reader(
            {input,raw_x,raw_y,raw_z,block_length,padding}
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

        VoxelCompressOptions opts;
        opts.width = frame_width;
        opts.height = frame_height;
        VoxelCompress cmp(opts);

        sv::Writer writer(output.c_str());
        writer.write_header(header);

        reader.startRead();
        while(!reader.isEmpty()){
            auto block = reader.getBlock();
            std::vector<std::vector<uint8_t >> packets;
            cmp.compress(block.data.data(),block.data.size(),packets);
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
//
// Created by wyz on 2021/2/25.
//
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<VoxelCompression/voxel_uncompress/VoxelUncompress.h>
#include<VoxelCompression/utils/NvCodecUtils.h>
#include<VoxelCompression/voxel_compress/VoxelCompress.h>
#include<iostream>
#include<map>
#include<vector>
#include<math.h>
#include<array>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<cuda.h>
#include<cudaGL.h>
#include"Shader.h"
#define TF_DIM 256
using namespace std;
#define volume_file_name_0 "aneurism_256_256_256_uint8.raw"
#define volume_0_len 256*256*256
bool readVolumeData(uint8_t* &data, int64_t& len)
{
    std::fstream in(volume_file_name_0,ios::in|ios::binary);
    if(!in.is_open()){
        throw std::runtime_error("file open failed");
    }
    in.seekg(0,ios::beg);
    data=new uint8_t[volume_0_len];
    len=in.read(reinterpret_cast<char*>(data),volume_0_len).gcount();
    if(len==volume_0_len)
        return true;
    else
        return false;
}

class TransferFunc{
public:
    explicit TransferFunc(std::map<uint8_t ,std::array<double,4>> color_setting):color_setting(color_setting){};
    auto getTransferFunction()->std::vector<float>&{
        if(transfer_func.empty())
            generateTransferFunc();
        return transfer_func;
    }
    void resetTransferFunc(std::map<uint8_t,std::array<double,4>> color_setting){
        this->color_setting=color_setting;
        transfer_func.clear();
        preint_transfer_func.clear();
    }
    auto getPreIntTransferFunc()->std::vector<float>&{
        if(preint_transfer_func.empty())
            generatePreIntTransferFunc();
        return preint_transfer_func;
    }
private:
    void generateTransferFunc();
    void generatePreIntTransferFunc();
private:
    std::map<uint8_t,std::array<double,4>> color_setting;
    std::vector<float> transfer_func;
    std::vector<float> preint_transfer_func;
    const int base_sampler_number=20;
    const int ratio=1;
};

inline void TransferFunc::generateTransferFunc()
{
    transfer_func.resize(TF_DIM*4);
    std::vector<uint8_t> keys;
    for(auto it:color_setting)
        keys.emplace_back(it.first);
    size_t size=keys.size();
    for(size_t i=0;i<keys[0];i++){
        transfer_func[i*4+0]=color_setting[keys[0]][0];
        transfer_func[i*4+1]=color_setting[keys[0]][1];
        transfer_func[i*4+2]=color_setting[keys[0]][2];
        transfer_func[i*4+3]=color_setting[keys[0]][3];
    }
    for(size_t i=keys[size-1];i<TF_DIM;i++){
        transfer_func[i*4+0]=color_setting[keys[size-1]][0];
        transfer_func[i*4+1]=color_setting[keys[size-1]][1];
        transfer_func[i*4+2]=color_setting[keys[size-1]][2];
        transfer_func[i*4+3]=color_setting[keys[size-1]][3];
    }
    for(size_t i=1;i<size;i++){
        int left=keys[i-1],right=keys[i];
        auto left_color=color_setting[left];
        auto right_color=color_setting[right];

        for(size_t j=left;j<=right;j++){
            transfer_func[j*4+0]=1.0f*(j-left)/(right-left)*right_color[0]+1.0f*(right-j)/(right-left)*left_color[0];
            transfer_func[j*4+1]=1.0f*(j-left)/(right-left)*right_color[1]+1.0f*(right-j)/(right-left)*left_color[1];
            transfer_func[j*4+2]=1.0f*(j-left)/(right-left)*right_color[2]+1.0f*(right-j)/(right-left)*left_color[2];
            transfer_func[j*4+3]=1.0f*(j-left)/(right-left)*right_color[3]+1.0f*(right-j)/(right-left)*left_color[3];
        }
    }
}

inline void TransferFunc::generatePreIntTransferFunc()
{
    if(transfer_func.empty())
        generateTransferFunc();
    preint_transfer_func.resize(4*TF_DIM*TF_DIM);

    float rayStep=1.0;
    for(int sb=0;sb<TF_DIM;sb++){
        for(int sf=0;sf<=sb;sf++){
            int offset=sf!=sb;
            int n=base_sampler_number+ratio*std::abs(sb-sf);
            float stepWidth=rayStep/n;
            float rgba[4]={0,0,0,0};
            for(int i=0;i<n;i++){
                float s=sf+(sb-sf)*(float)i / n;
                float sFrac=s-std::floor(s);
                float opacity=(transfer_func[int(s)*4+3]*(1.0-sFrac)+transfer_func[((int)s+offset)*4+3]*sFrac)*stepWidth;
                float temp=std::exp(-rgba[3])*opacity;
                rgba[0]+=(transfer_func[(int)s*4+0]*(1.0-sFrac)+transfer_func[(int(s)+offset)*4+0]*sFrac)*temp;
                rgba[1]+=(transfer_func[(int)s*4+1]*(1.0-sFrac)+transfer_func[(int(s)+offset)*4+1]*sFrac)*temp;
                rgba[2]+=(transfer_func[(int)s*4+2]*(1.0-sFrac)+transfer_func[(int(s)+offset)*4+2]*sFrac)*temp;
                rgba[3]+=opacity;
            }
            preint_transfer_func[(sf*TF_DIM+sb)*4+0]=preint_transfer_func[(sb*TF_DIM+sf)*4+0]=rgba[0];
            preint_transfer_func[(sf*TF_DIM+sb)*4+1]=preint_transfer_func[(sb*TF_DIM+sf)*4+1]=rgba[1];
            preint_transfer_func[(sf*TF_DIM+sb)*4+2]=preint_transfer_func[(sb*TF_DIM+sf)*4+2]=rgba[2];
            preint_transfer_func[(sf*TF_DIM+sb)*4+3]=preint_transfer_func[(sb*TF_DIM+sf)*4+3]=1.0-std::exp(-rgba[3]);
        }
    }
}

class Camera{
public:
    enum class CameraDefinedKey{
        FASTER,SLOWER
    };
    enum class CameraMoveDirection{
        FORWARD,BACKWARD,
        LEFT,RIGHT,
        UP,DOWN
    };
public:
    Camera(glm::vec3 camera_pos):
            pos(camera_pos),up(glm::vec3(0.0f,1.0f,0.0f)),
            front(glm::vec3(0.0f,0.0f,-1.0f)),
//        right(glm::vec3(1.0f,0.0f,0.0f)),
            world_up(glm::vec3(0.0f,1.0f,0.0f)),
            yaw(-90.0f),pitch(0.0f),
            move_speed(0.03f),
            mouse_sensitivity(0.1f),
            zoom(20.0f)
    {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix();
    void processMovementByKeyboard(CameraMoveDirection direction,float delta_t);
    void processMouseMovement(float xoffset,float yoffset);
    void processMouseScroll(float yoffset);
    void processKeyboardForArgs(CameraDefinedKey arg);
    void updateCameraVectors();

private:
    glm::vec3 pos;//point
    glm::vec3 front;//vector
    glm::vec3 up;//vector
    glm::vec3 right;//vector

    glm::vec3 world_up;//vector

    float yaw;
    float pitch;

    float move_speed;
    float mouse_sensitivity;
    float zoom;
public:
    float getZoom(){return zoom;}
};
inline glm::mat4 Camera::getViewMatrix()
{
//        std::cout<<pos.x<<" "<<pos.y<<" "<<pos.z<<std::endl;
//        std::cout<<front.x<<" "<<front.y<<" "<<front.z<<std::endl;
//        std::cout<<up.x<<" "<<up.y<<" "<<up.z<<std::endl;
    return glm::lookAt(pos,pos+front,up);
}
inline void Camera::processMovementByKeyboard(CameraMoveDirection direction,float delta_t)
{

    float ds=move_speed*delta_t;
    switch (direction) {
        case CameraMoveDirection::FORWARD: pos+=front*ds;break;
        case CameraMoveDirection::BACKWARD: pos-=front*ds;break;
        case CameraMoveDirection::LEFT: pos-=right*ds;break;
        case CameraMoveDirection::RIGHT: pos+=right*ds;break;
        case CameraMoveDirection::UP: pos+=up*ds;break;
        case CameraMoveDirection::DOWN: pos-=up*ds;break;
    }
}
inline void Camera::processMouseMovement(float xoffset,float yoffset)
{
    yaw+=xoffset*mouse_sensitivity;
    pitch+=yoffset*mouse_sensitivity;
    if(pitch>60.0f)
        pitch=60.0f;
    if(pitch<-60.0f)
        pitch=-60.0f;
    updateCameraVectors();
}
inline void Camera::processMouseScroll(float yoffset)
{
    zoom-=yoffset;
    if(zoom<0.1f)
        zoom=0.1f;
    if(zoom>45.0f)
        zoom=45.0f;
}
inline void Camera::processKeyboardForArgs(CameraDefinedKey arg)
{
    if(arg==CameraDefinedKey::FASTER)
        move_speed*=2;
    else if(arg==CameraDefinedKey::SLOWER)
        move_speed/=2;
}
inline void Camera::updateCameraVectors()
{
    glm::vec3 f;
    f.x=std::cos(glm::radians(yaw))*std::cos(glm::radians(pitch));
    f.y=std::sin(glm::radians(pitch));
    f.z=std::sin(glm::radians(yaw))*std::cos(glm::radians(pitch));
    front=glm::normalize(f);
    right=glm::normalize(glm::cross(front,world_up));
    up=glm::normalize(glm::cross(right,front));
}


GLFWwindow *window;
void initGL()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(1);
    window = glfwCreateWindow(1200,900, "Volume Render", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return ;
    }

    glfwMakeContextCurrent(window);

//    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(0);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glEnable(GL_DEPTH_TEST);
}

Camera camera({1.f,1.f,1.f});
bool first_mouse=true;
float last_x,last_y;
float delta_time=0.f;
float last_frame=0.f;
void FramebufferSizeCallback(GLFWwindow *window, int w, int h)
{
    glViewport(0,0,w,h);
}

void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    if(first_mouse){
        last_x=xpos;
        last_y=ypos;
        first_mouse=false;
    }
    double dx=xpos-last_x;
    double dy=last_y-ypos;

    last_x=xpos;
    last_y=ypos;

    camera.processMouseMovement(dx,dy);
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.processMouseScroll(yoffset);
}

void KeyCallback(GLFWwindow *, int, int, int, int)
{

}

void MouseButtonCallback(GLFWwindow *, int, int, int)
{

}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processMovementByKeyboard(Camera::CameraMoveDirection::FORWARD, delta_time);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processMovementByKeyboard(Camera::CameraMoveDirection::BACKWARD, delta_time);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processMovementByKeyboard(Camera::CameraMoveDirection::LEFT, delta_time);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processMovementByKeyboard(Camera::CameraMoveDirection::RIGHT, delta_time);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.processMovementByKeyboard(Camera::CameraMoveDirection::UP, delta_time);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.processMovementByKeyboard(Camera::CameraMoveDirection::DOWN, delta_time);
}

int main(int argc,char** argv)
{
    std::cout<<"start compress"<<std::endl;
    VoxelCompressOptions c_opts;
    c_opts.height=256;
    c_opts.width=256;
    c_opts.input_buffer_format=NV_ENC_BUFFER_FORMAT_NV12;
    VoxelCompress v_cmp(c_opts);
    uint8_t* data=nullptr;
    int64_t len=0;
    readVolumeData(data,len);
    std::vector<std::vector<uint8_t>> packets;
    v_cmp.compress(data,len,packets);
    int64_t cmp_size=0;
    for(size_t i=0;i<packets.size();i++){
        cmp_size+=packets[i].size();
    }
    std::cout<<"compress size is: "<<cmp_size<<std::endl;
    std::cout<<"finish compress"<<std::endl;

    checkCUDAErrors(cuInit(0));
    CUdevice cuDevice=0;
    checkCUDAErrors(cuDeviceGet(&cuDevice, 0));
    CUcontext cuContext = nullptr;
    checkCUDAErrors(cuCtxCreate(&cuContext,0,cuDevice));
    checkCUDAErrors(cuCtxSetCurrent(cuContext));
    uint8_t* res_ptr=nullptr;//device ptr
    int64_t length=256*256*256;
    checkCUDAErrors(cuMemAlloc((CUdeviceptr*)&res_ptr,length));//driver api prefix cu
//    cudaMalloc((void**)(CUdeviceptr*)&res_ptr,length);//runtime api prefix cuda

    std::cout<<"start uncompress"<<std::endl;
    VoxelUncompressOptions opts;
    opts.height=256;
    opts.width=256;
    opts.cu_ctx=cuContext;
    VoxelUncompress v_uncmp(opts);
    v_uncmp.uncompress(res_ptr,length,packets);
    std::cout<<"finish uncompress"<<std::endl;

    initGL();
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window,KeyCallback);
    glfwSetMouseButtonCallback(window,MouseButtonCallback);

    const char* filename="uncmpres_256_256_256_uint8.raw";

    std::map<uint8_t ,std::array<double,4>> color_map;
    color_map[0]={0.0,0.0,0.0,0.0};
    color_map[114]={127/255.0,63/255.0,27/255.0,0.0};
    color_map[165]={127/255.0,63/255.0,27/255.0,0.6};
    color_map[216]={127/255.0,63/255.0,27/255.0,0.3};
    color_map[255]={0.0,0.0,0.0,0.0};

    TransferFunc tf(color_map);
    auto tf_array=tf.getTransferFunction();

    GLuint volume_tex;
    glGenTextures(1,&volume_tex);
    glBindTexture(GL_TEXTURE_3D,volume_tex);
    glBindTextureUnit(2,volume_tex);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D,0,GL_RED,256,256,256,0,GL_RED,GL_UNSIGNED_BYTE,nullptr);

    GLuint pbo;
    glGenBuffers(1,&volume_tex);

    CUgraphicsResource cuResource;
    checkCUDAErrors(cuGraphicsGLRegisterImage(&cuResource, volume_tex,GL_TEXTURE_3D, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
    checkCUDAErrors(cuGraphicsMapResources(1, &cuResource, 0));
    CUdeviceptr tex_cuda_ptr;
    size_t size=0;

//    checkCUDAErrors(cuGraphicsResourceGetMappedPointer(&tex_cuda_ptr,&size,cuResource));
    CUarray cu_array;
//    CUDA_ARRAY3D_DESCRIPTOR des;
//    des.Height=des.Depth=des.Width=256;
//    des.Format=CU_AD_FORMAT_UNSIGNED_INT8;
//    des.NumChannels=1;
//    cuArray3DCreate(&cu_array,&des);
    checkCUDAErrors(cuGraphicsSubResourceGetMappedArray(&cu_array,cuResource,0,0));

//    std::cout<<"tex_cuda_ptr: "<<tex_cuda_ptr<<"\tsize: "<<size<<std::endl;
//    checkCUDAErrors(cuMemcpy((CUdeviceptr)tex_cuda_ptr,(CUdeviceptr)res_ptr,size));

//    checkCUDAErrors(cuMemcpyDtoA(cu_array,0,(CUdeviceptr)res_ptr,length));
    CUDA_MEMCPY3D m = { 0 };
    m.srcMemoryType=CU_MEMORYTYPE_DEVICE;
    m.srcDevice=(CUdeviceptr)res_ptr;

    m.dstMemoryType=CU_MEMORYTYPE_ARRAY;
    m.dstArray=cu_array;
    m.WidthInBytes=256;
    m.Height=256;
    m.Depth=256;
    checkCUDAErrors(cuMemcpy3D(&m));
    checkCUDAErrors(cuGraphicsUnmapResources(1, &cuResource, 0));
    checkCUDAErrors(cuGraphicsUnregisterResource(cuResource));

#define RAYCAST_POS_FRAG "C:/Users/wyz/projects/VoxelCompression/test/shader/raycast_pos_f.glsl"
#define RAYCAST_POS_VERT "C:/Users/wyz/projects/VoxelCompression/test/shader/raycast_pos_v.glsl"
#define RAYCASTING_FRAG "C:/Users/wyz/projects/VoxelCompression/test/shader/raycasting_f.glsl"
#define RAYCASTING_VERT "C:/Users/wyz/projects/VoxelCompression/test/shader/raycasting_v.glsl"

    sv::Shader raycastpos_shader(RAYCAST_POS_VERT,RAYCAST_POS_FRAG);
    sv::Shader raycasting_shader(RAYCASTING_VERT,RAYCASTING_FRAG);

    raycasting_shader.use();
    raycasting_shader.setInt("transfer_func",0);
    raycasting_shader.setInt("preInt_transferfunc",1);
    raycasting_shader.setInt("volume_data",2);

    raycasting_shader.setFloat("ka",0.5f);
    raycasting_shader.setFloat("kd",0.8f);
    raycasting_shader.setFloat("shininess",100.0f);
    raycasting_shader.setFloat("ks",1.0f);
    raycasting_shader.setVec3("light_direction",glm::normalize(glm::vec3(-1.0f,-1.0f,-1.0f)));

    raycasting_shader.setFloat("step",1.0f/256*0.3f);

//    while(!glfwWindowShouldClose(window)){
//
//        float current_frame=glfwGetTime();
//        delta_time=current_frame-last_frame;
//        last_frame=current_frame;
//        std::cout<<"fps: "<<1.0f/delta_time<<std::endl;
//
//        glfwPollEvents();
//        processInput(window);
//
//        glm::mat4 model=glm::mat4(1.0f);
//        glm::mat4 view=camera.getViewMatrix();
//
//        glm::mat4 projection=glm::perspective(glm::radians(camera.getZoom()),
//                                              (float)1200/(float)900,
//                                              0.1f,50.0f);
//        glm::mat4 mvp=projection*view*model;
//
//        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        raycastpos_shader.use();
//        raycastpos_shader.setMat4("MVPMatrix",mvp);
//        glBindFramebuffer(GL_FRAMEBUFFER,raycastpos_fbo);
//        glBindVertexArray(proxy_cube_vao);
//        glDrawBuffer(GL_COLOR_ATTACHMENT0);
//        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
//        glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0);
//
//        glEnable(GL_CULL_FACE);
//        glFrontFace(GL_CCW);//逆时针
//        glDrawBuffer(GL_COLOR_ATTACHMENT1);
//        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
//        glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0);
//        glDisable(GL_CULL_FACE);
//        glBindFramebuffer(GL_FRAMEBUFFER,0);
//
//        raycasting_shader.use();
//
//        glBindVertexArray(screen_quad_vao);
//        glDrawArrays(GL_TRIANGLES,0,6);
//
//        glfwSwapBuffers(window);
//    }
    return 0;
}
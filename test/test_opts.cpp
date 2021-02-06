//
// Created by wyz on 2021/2/5.
//
#include <thread>
#include <iostream>
#include <vector>
void func(int id)
{
    std::cout<<"id: "<<id<<std::endl;
}
int main(int argc,char** argv)
{
    int thread_num=2000;
    std::vector<std::thread> jobs;
    for(int i=0;i<thread_num;i++){
        jobs.emplace_back(func,i);
    }
    for(int i=0;i<thread_num;i++)
        jobs[i].join();
    return 0;
}


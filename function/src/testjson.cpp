#include "json/json.h"
#include <string>
#include <iostream>
#include <fstream>
#include"kfile.hpp"
#include"klog.hpp"
#include"kipc.hpp"

using namespace std;
static Json::Value gCfg;
LOG_TYPE _gLogLevel = TEST;
Log *glog;

int main(){
    glog = new Log("../log/mount-service");
#if 0
    string test ="{\"id\":1,\"name\":\"kurama\"}";
    Json::Reader reader;
    Json::Value value;
    if(reader.parse(test,value))
    {
        if(!value["id"].isNull())
        {
            cout<<value["id"].asInt()<<endl;
            cout<<value["name"].asString()<<endl;
        }
    }
#else
    /*
    ifstream readfile("../etc/config.json");
    if ( !readfile.is_open() ){
        return false;
    }

    string content = "";
    readfile.seekg (0, ios::end);
    int size = (int)readfile.tellg();
    readfile.seekg (0, ios::beg);
    unsigned char* buffer = new unsigned char[size];
    readfile.read ((char*)buffer, size);

    content = string( (char*)buffer, size );
    
    delete[] buffer;
    readfile.close();
*/
    Files fd;
    fd.Open("../etc/config.json", O_RDONLY);
    string content = string(fd.Data());
    cout<<content<<endl;
    
    Json::Reader reader;
    cout<<"正在解析"<<endl;
    if(!reader.parse(content, gCfg)){
        cout<<"读取配置文件 %s 失败，或该文件不存在"<<endl;
        return false;
    }
    cout<<"解析完毕"<<endl;

#endif
    return 0;
}

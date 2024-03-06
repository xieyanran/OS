#include <unistd.h>
#include <string>
#include <iostream>
#include <vector>
#include <wait.h>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>


using namespace std;
#define INSTRUCTION_MAX_SIZE 255;
#define BUFFER_MAX (1024*1024)  // 1MB
int builtin_command_num = 5;

vector<string> instruction;
vector<string> history_command_list;

struct builtin_command{
    string name;
    int (*address)(char **);
};

int my_cd(char **argv){
     // 检查命令是否为 "cd"
    if(strcmp(argv[0], "cd")){
        cout << "error: the instruction is not cd\n";
        return 0;
    }
    // 检查是否提供了目标目录参数
    if(argv[1]==NULL){
        cout << "parameter error\n";
        return 0;
    }
     // 使用 chdir 函数改变当前工作目录为指定目录
    chdir(argv[1]);
}

int my_history(char **argv){
     // 检查命令是否为 "history"
    if(strcmp(argv[0], "history")){
        cout << "error: the instruction is not history\n";
        return 0;
    }
     // 检查命令历史记录是否为空
    if(history_command_list.empty()){
        cout << "the command history list is empty. you have never used command\n";
        return 0;
    }
     // 遍历并输出命令历史记录
    for(auto & i : history_command_list){
        cout << i << "\t";
    }
    cout << "\n";
}

int my_mkdir(char **argv){
    // 检查命令是否为 "mkdir"
    if(strcmp(argv[0], "mkdir")){
        cout << "the instruction is not mkdir\n";
        return 0;
    }
    // 检查是否提供了目标目录参数
    if(argv[1]==nullptr){
        cout << "parameter error\n";
        return -1;
    }
    // 检查目录是否已存在
    if(access(argv[1], F_OK) >=0){
        return 0;
    }
    // 使用 mkdir 函数创建新目录，权限设置为 0777
    return mkdir(argv[1], 0777);
}

int my_cp(char **argv){
    // 检查命令是否为 "cp"
    if(strcmp(argv[0], "cp")){
        cout << "the instruction is not cp\n";
        return -1;
    }
    // 检查是否提供了源文件和目标文件参数
    if(argv[1]==nullptr || argv[2]==nullptr){
        cout << "param error\n";
        return -1;
    }
    // 检查源文件是否存在
    if(access(argv[1], F_OK) < 0){
        return 0;
    }
    // 打开源文件和目标文件
    FILE *p_src = fopen(argv[1], "rb");
    FILE *p_des = fopen(argv[2], "wb");
    // 获取源文件的大小
    struct stat stat1;
    stat(argv[1], &stat1);
    int size_dynamic = stat1.st_size;
    // 如果源文件大小超过缓冲区大小限制，将其限制为缓冲区大小
    if(size_dynamic>BUFFER_MAX){
        size_dynamic = BUFFER_MAX;
    }
    // 创建动态缓冲区并复制文件内容
    char *buffer = new char[size_dynamic];
    int res_fread=0;
    while(!feof(p_src)){
        res_fread = fread(buffer, sizeof(char), size_dynamic, p_src);
        fwrite(buffer, sizeof(char), res_fread, p_des);
    }
    // 释放动态缓冲区并关闭文件
    delete buffer;
    fclose(p_src);
    fclose(p_des);

    cout << "copy " << argv[1] << " to " << argv[2] << " successfully \n";
}

int my_date(char **argv){
    // 检查命令是否为 "date"
    if(strcmp(argv[0],"date")){
        cout << "the instruction is not date\n";
        return -1;
    }
    // 获取当前时间戳
    time_t time_raw = time(nullptr);
    // 使用 ctime 函数将时间戳转换为本地时间字符串
    char *time_c_local = ctime(&time_raw);
    // 输出本地时间字符串
    cout << "the local time is " << time_c_local;
}

builtin_command builtin_command_list[]={
        "cd", my_cd,
        "history", my_history,
        "mkdir", my_mkdir,
        "cp", my_cp,
        "date", my_date,
};

int promt_show() {
    // 打印用户名和主机名部分的静态提示信息
    cout << "[xieyanran@21281113]:\t";
    // 用于存储当前工作目录的路径
    char string[255];
    // 获取当前工作目录的路径，并打印到屏幕上
    cout << getcwd(string, 255);
    // 打印命令提示符，表示用户可以开始输入命令
    cout << "/$ ";
}

int get_instruction() {
    int max_size = INSTRUCTION_MAX_SIZE;// // 定义最大命令长度的上限
    string instruct_input;
    char instruction_input_char[255];
    int count =0;
    //用户输入的命令过长
    while(count < max_size && cin >> instruct_input){
        if(instruct_input.size() >= max_size){
            cout << "the instruction is too long\n";
            return -1;
        }
         // 将用户输入的命令添加到 instruction 向量中
        instruction.emplace_back(instruct_input);

        int flag_cin_get;
        flag_cin_get = cin.get();
        if(flag_cin_get == '\n') break;
    }
    return 0;
}

char** transfer2char(vector<string> &src){
     char **des = new char*[src.size()+1];
     // 为每个字符串分配一个字符数组
     for(int i=0; i<src.size(); ++i){
        des[i] = new char[255];
        strcpy(des[i], src[i].c_str());
    }
    return des;
}
int execute(){
    // 匹配内置命令
//    如果用户输入的命令是"builtin"，则列出内置命令列表的数量和名称
    if(instruction[0]=="builtin"){
        cout << "the builtin command archived right now has " << builtin_command_num << "\n";
        for(int i=0; i<builtin_command_num; ++i){
            cout << builtin_command_list[i].name << "\n";
        }
        return 0;
    }

    // 创建一个字符指针数组用于存储命令参数
    char ** argv = transfer2char(instruction);
    int status;
    
    // 检查用户输入的命令是否是已知的内置命令
    for(int i=0; i< builtin_command_num;++i){
        if(instruction[0]==builtin_command_list[i].name){
            // 如果是内置命令之一，将命令添加到历史命令列表，并调用与之关联的函数
            history_command_list.emplace_back(instruction[0]);
            return builtin_command_list[i].address(argv);
        }
    }

// 创建一个子进程以执行外部命令
    pid_t pid = fork();
    if(pid == 0){
        // 在子进程中执行用户输入的外部命令
        execvp(instruction[0].c_str(), argv);
    }
    else if(pid < 0){
        // 如果fork失败，输出错误消息
        perror("error:\t");
    }
    else{
        // 在父进程中等待子进程完成
        waitpid(pid, &status, WUNTRACED);
    }

    // 将执行的命令添加到历史命令列表
    history_command_list.emplace_back(instruction[0]);
    
    // 清理动态分配的内存
    for(int i=0; i<instruction.size(); ++i){
        delete argv[i];
    }
    delete argv;
    return 0;
}

int free_vector(){//清空存储用户输入命令的 instruction 向量
    instruction.erase(instruction.begin(), instruction.end());
    return 0;
}

int runs(){
    cout << "welcome to Xie Yanran's shell" << "\n\n";
    while(1){
        promt_show();//显示提示符
        get_instruction();//获取用户输入
        execute();//执行命令
        free_vector();//清理资源
    }
}

int main() {

    runs();
//    execvp("ls", NULL);
}
//基于TCP的HTTP服务器，1.0版本
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX 1024
#define HOME_PAGE "index.html"
//#define Debug 1

static int startup(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        printf("sock error\n");
        exit(2);
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);//监听主机所有IP

    if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0)
    {
        printf("bind error\n");
        exit(3);
    }

    if(listen(sock, 5) < 0)
    {
        printf("listen error\n");
        exit(4);
    }
    return sock;
}

int get_line(int sock, char line[], int size)//按行获取文本->行结束标记可能不同
{
    int c = 'a';//仅初始化，只要不是'\n'即可
    int i = 0;
    while(c!='\n' && i<size-1)//size-1是因为之后要加'\0'在末尾
    {
        ssize_t s = recv(sock, &c, 1, 0);//一次从sock拿一个字符
        if(s > 0)//因为协议等不同，可能的换行标志也不同，可能是'\r'、'\r\n'、'\n'
        {
            if(c == '\r')
            {
                //如果直接去拿sock中的下一个数据，若并不是换行标志，就得放回去，比较麻烦
                recv(sock, &c, 1, MSG_PEEK);//参数flag的MSG_PEEK取值表示看一眼数据，并不拿走
                if(c != '\n')//只有一个'\r'作为换行
                    c = '\n';
                else//'\r\n'作为换行
                    recv(sock, &c, 1, 0);
            }
            //以'\n'作为结尾标志的我们不用管
            line[i++] = c;
        }
        else//读失败 
        {
            break;
            //return 5;
        }
    }
    line[i] = '\0';
    return i;
}

void echo_error(int code)
{
    switch(code)
    {
        case 404:
            break;
        case 502:
            break;
        default:
            break;
    }
}

void clear_header(int sock)
{
    char line[MAX];
    do
    {
        get_line(sock, line, sizeof(line));
    }while(strcmp(line, "\n") != 0);//读到空行循环结束
}

void echo_www(int sock, char* path, int size, int* err)//不带参数的GET请求(请求普通文件)的返回->返回一个页面
{//此时并无请求正文
    clear_header(sock);//将请求处理完
    
    //打开请求的文件
    int fd = open(path, O_RDONLY);
    if(fd < 0)
    {
        *err = 404;
        return;
    }
    char line[MAX];
    sprintf(line, "HTTP/1.0 200 OK\r\n");
    send(sock, line, strlen(line), 0);//先返回响应报头,不用带/0，那是C认为的字符串结尾
    sprintf(line, "Content-Type: text/html\r\n");
    send(sock, line, strlen(line), 0);
    //sprintf(line, "Content-Type:text/html;charset=ISO-8859-1\r\n");//请求报头返回一个content-type
    //send(sock, line, strlen(line), 0);
    sprintf(line, "\r\n");
    send(sock, line, strlen(line), 0);//给响应报文返回一个空行
    sendfile(sock, fd, NULL, size);//将请求文件返回,写到sock里
    close(fd);
}

int exe_cgi(int sock, char path[], char method[], char* query_string)//以cgi模式处理
{//处理带参的GET与POST请求
    char line[MAX];
    int content_length = -1;
    char method_env[MAX/32];
    char query_string_env[MAX];
    char content_length_env[MAX/16];
    
    if(strcasecmp(method, "GET") == 0)//GET
    {
        clear_header(sock);//就算已经拿到了参数，也要把报头读完，即浏览器传来的请求读完 
    }
    else//POST
    {
        //因为POST的参数在正文中，所以要先读到空行
        do
        {
            get_line(sock, line, sizeof(line));
            //http报文的报头中的Content-Length字段标明了正文的长度
            if(strncmp(line, "Content-Length: ", 16) == 0)
            {
                content_length = atoi(line+16);
            }
        }while(strcmp(line, "\n") != 0);
        if(content_length == -1)//不知道正文的长度，无法继续处理
        {
            return 404;
        }
    }
    //先响应正文前的内容  
    sprintf(line, "HTTP/1.0 200 OK\r\n");
    send(sock, line, strlen(line), 0);//先返回响应报头,不用带/0，那是C认为的字符串结尾
    sprintf(line, "Content-Type: text/html\r\n");
    send(sock, line, strlen(line), 0);
    sprintf(line, "\r\n");
    send(sock, line, strlen(line), 0);//给响应报文返回一个空行
    
    //开始处理正文部分
    //走到这里，不论是什么方法，都有机会拿到参数
    int input[2];
    int output[2];
    //浏览器将请求发给父进程,父进程将数据传给子进程，子进程执行之后将最终的结果给父进程，让父进程再返回给浏览器
    //创建两个管道，实现父子间通信，因为进程具有独立性，不能直接看到对方的消息
    //父进程将数据方法等发给子进程，让子进程去执行程序
    //子进程将执行完结果传给父进程，让父进程响应给浏览器
    pipe(input);
    pipe(output);
    pid_t id = fork();
    if(id < 0)
    {
        return 404;
    }
    else if(id == 0)//child->进程替换->执行新的程序代码
    {//method, GET[query_string],POST[content_length]
        close(input[1]);//通过input读
        close(output[0]);//通过output写
        //因为进程替换只会替换内存中的代码和数据，并不会替换文件描述符表等信息
        //所以只要子进程能够拿到要读、写的fd，就可以进行读写了
        dup2(input[0], 0);//dup2->新的是旧的一份拷贝
        dup2(output[1], 1);
       
        //将所需数据等导为环境变量，且写到环境变量列表里
        sprintf(method_env, "METHOD=%s", method);
        putenv(method_env);//写到系统环境变量中去
        if(strcasecmp(method, "GET") == 0)//GET方法，因为query_string放着参数，所以导出该环境变量
        {
            sprintf(query_string_env, "QUERY_STRING=%s", query_string);
            putenv(query_string_env);
        }
        else//POST方法,要通过content-length来确定正文中的参数
        {
            sprintf(content_length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(content_length_env);
        }

        //程序替换
        execl(path, path, NULL);//第二个参数，因为在命令行上带路径运行命令也可以执行
        exit(1);//进程替换只要返回就说明替换失败
    }
    else//father->wait
    {
        close(input[0]);//通过input写
        close(output[1]);//通过output读
        
        //将子进程需要的方法等数据通过环境变量传过去 
        char c;
        if(strcasecmp(method, "POST") == 0)//POST方法要从正文中读content-length个数据
        {//前面将首行及报头处理过了，这里直接读的就是正文
            int i = 0;
            for(; i < content_length; ++i)
            {
                read(sock, &c, 1);//读正文中参数
                write(input[1], &c, 1);//写给子进程
            }
        }

        //将子进程返回的数据依次返回给浏览器
        while(read(output[0], &c, 1) > 0)
        {
            send(sock, &c, 1, 0);
        }
        waitpid(id, NULL, 0);//这里是线程在等，不会使服务器阻塞
        close(input[1]);
        close(output[0]);
    }
    return 200;
}

static void* handler_request(void* arg)
{
    int sock = (int)arg;
    char line[MAX];
    char method[MAX/32];
    char url[MAX];
    int errcode = 200;
    int cgi = 0;//以cgi模式处理要提交的数据的请求信息
    char path[MAX];
    char* query_string = NULL;//参数信息(?后面的部分)
#ifdef Debug
    do
    {
        get_line(sock, line, sizeof(line));//将按行获取得到的消息放到line中
        printf("%s", line);
    }while(strcmp(line, "\n") != 0);//读到空行就不读了
#else
    if(get_line(sock, line, sizeof(line)) < 0)//获得行出错
    {
        errcode = 404;
        goto end;
    }
    
//GET /index.php?a=10 HTTP/1.1
    //get method->从line里拿方法到method中去
    int i = 0;
    int j = 0;
    while(i<sizeof(method)-1 && j<sizeof(line) && !isspace(line[j]))
    {
        method[i] = line[j];
        i++;
        j++;
    }
    method[i] = '\0';
    //只处理GET与POST方法
    if(strcasecmp(method, "GET") == 0)//strcasecmp可以兼容比较大小写
    {

    }
    else if(strcasecmp(method, "POST") == 0)//在我们编写的服务器中认为POST方法一定带参数
    {
        cgi = 1;//想要向网页上提交数据就要以cgi模式处理请求
    }
    else//既不是get也不是post方法
    {
        errcode = 404;
        goto end;
    }
    //get url->将请求行中间位置的url拿出来
    while(j<sizeof(line) && isspace(line[j]))
    {
        j++;
    }
    i=0;
    while(i<sizeof(url)-1 && j<sizeof(line) && !isspace(line[j]))
    {
        url[i] = line[j];
        i++;
        j++;
    }
    url[i] = '\0';

    //处理GET方法中url,带参(url中会有?号)将参数拿出来
    if(strcasecmp(method, "GET") == 0)//strcasecmp支持大小写同时比较
    {
        query_string = url;//url的前半部分一定是要请求的资源
        while(*query_string)
        {
            if(*query_string == '?')//?后面的才是参数信息
            {
                *query_string = '\0';//url字符串由一个变成两个
                query_string++;
                cgi = 1;
                break;
            }
            query_string++;
        }
    }

//method[GET|POST],cgi[0|1],url[],query_string[NULL|arg]
    //判断请求的资源存不存在
    //不论存不存在，请求的资源一定会从当前目录下的wwwroot下开始
    //url中放的是wwwroot/a/b/c.html,请求某文件 | url->wwwroot/,请求目录->返回首页
    sprintf(path, "wwwroot%s", url);//将url前拼接上第二个参数输出到path
    if(path[strlen(path)-1] == '/')//将路径拼接上
    {
        strcat(path, HOME_PAGE);
    }
    
    //printf("method:%s, path:%s\n", method, path);
    //判断文件是否存在
    struct stat st;
    if(stat(path, &st) < 0)//该函数成功返回0，失败返回-1
    {//请求的文件不存在
        errcode = 404;
        goto end;
    }
    else
    {
        if(S_ISDIR(st.st_mode))//请求的是目录->给一个目录的首页
        {
            strcat(path, HOME_PAGE);
        }
        else
        {
            //判断请求的资源是不是可执行程序
            if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))//用户/所在组/其他有可执行权限
            {
                cgi = 1;
            }
        }
        if(cgi)//若以cgi方式运行
        {
            errcode = exe_cgi(sock, path, method, query_string);
        }
        else
        {
            printf("method:%s, path:%s\n", method, path);
            //method[GET|POST], cgi[0|1], url[], query_string[NULL|arg]
            echo_www(sock, path, st.st_size, &errcode);//不带参的GET请求，且请求的是一普通文件
        }
    }

#endif
end:
    if(errcode != 200)
        echo_error(errcode);
    close(sock);
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Usage: %s [port]\n", argv[0]);
        return 1;
    }

    //daemon(0, 0);

    int listen_sock = startup(atoi(argv[1]));//参数将字符串转化为整型
    
    for(; ;)
    {
       struct sockaddr_in client;
       socklen_t len = sizeof(client);

       int sock = accept(listen_sock, (struct sockaddr*)&client, &len);
       if(sock < 0)
       {
           perror("accept");
           continue;
       }
       
       pthread_t id;
       pthread_create(&id, NULL, handler_request, (void*)sock);
       pthread_detach(id);
    }

    return 0;
}

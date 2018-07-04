#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

void insert_data(char* name, char* sex, char* tel)
{
    MYSQL* mysql_fd = mysql_init(NULL);//初始化mysql

    if(mysql_real_connect(mysql_fd, "127.0.0.1", "root", "t020", "httpd", 3306, NULL, 0) == NULL)//连接mysql
    {
        printf("connect faild\n");
        return;
    }
    printf("connect mysql success\n");
    
    //可以对连接上的数据库进行操作了
    char mysql[1024];
    sprintf(mysql, "insert into stu_info(name, sex, tel) values(\"%s\", \"%s\", \"%s\")", name, sex, tel);
    printf("sql: %s\n", mysql);
    //const char* mysql = "insert into stu_info(name, sex, tel) values(\"youngjack\", \"man\", \"12345678901\")";
    mysql_query(mysql_fd, mysql);//执行mysql中的命令

    mysql_close(mysql_fd);//关闭mysql
}

int main()
{
    char data[1024];
    if(getenv("METHOD"))
    {
        if(strcasecmp("GET", getenv("METHOD")) == 0)//GET
        {
            strcpy(data, getenv("QUERY_STRING"));
        }
        else//POST
        {
            int content_length = atoi(getenv("CONTENT_LENGTH"));
            int i = 0;
            for(; i<content_length; i++)
            {
                read(0, data+i, 1);
            }
            data[i] = '\0';
        }
    }
    printf("arg: %s\n", data);

    char* name;
    char* sex;
    char* tel;
    strtok(data, "=&");
    name = strtok(NULL, "=&");
    strtok(NULL, "=&");
    sex = strtok(NULL, "=&");
    strtok(NULL, "=&");
    tel = strtok(NULL, "=&");

    //sscanf(data, "name=%s&sex=%s&tel=%s\n", name, sex, tel);

    //printf("client version: %s\n", mysql_get_client_info());//打印mysql版本
    insert_data(name, sex, tel);
    return 0;
}

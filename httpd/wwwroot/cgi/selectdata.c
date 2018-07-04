#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

void select_data()
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
    sprintf(mysql, "select* from stu_info");
    mysql_query(mysql_fd, mysql);//执行mysql中的命令

    MYSQL_RES* res = mysql_store_result(mysql_fd);
    int row = mysql_num_rows(res);
    int col = mysql_num_fields(res);
    MYSQL_FIELD* field = mysql_fetch_fields(res);
    int i = 0;
    for(; i<col; i++)
    {
        printf("%s\t", field[i].name);
    }
    printf("\n");

    printf("<table border=\"1\">");
    for(i=0; i<row; i++)
    {
        MYSQL_ROW rowdata = mysql_fetch_row(res);//返回值当作一个二维数组去看待
        int j = 0;
        printf("<tr>");
        for(; j<col; j++)
        {
            printf("<td>%s</td>", rowdata[j]);
        }
        printf("</tr>");
    }
    printf("</table>");

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
//    printf("arg: %s\n", data);
//
//    char* name;
//    char* sex;
//    char* tel;
//    strtok(data, "=&");
//    name = strtok(NULL, "=&");
//    strtok(NULL, "=&");
//    sex = strtok(NULL, "=&");
//    strtok(NULL, "=&");
//    tel = strtok(NULL, "=&");

    select_data();
    return 0;
}

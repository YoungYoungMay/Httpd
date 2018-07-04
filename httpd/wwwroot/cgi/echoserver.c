#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

#define MAX 1024

//firstdata=12121&lastdata=212
//strtok
void mycal(char* buf)
{
   int x,y;
   sscanf(buf, "firstdata=%d&lastdata=%d", &x, &y);

   printf("<html>\n");
   printf("<body>\n");

   printf("<h3>%d + %d = %d</h3>\n", x, y, x+y);
   printf("<h3>%d - %d = %d</h3>\n", x, y, x-y);
   printf("<h3>%d * %d = %d</h3>\n", x, y, x*y);
   if(y == 0)
   {
        printf("<h3>%d / %d = %d</h3>, %s\n", x, y, -1, "(zero)");
        printf("<h3>%d %% %d = %d</h3>, %s\n", x, y, -1, "(zero)");
   }
   else
   {
        printf("<h3>%d / %d = %d</h3>\n", x, y, x/y);
        printf("<h3>%d %% %d = %d</h3>\n", x, y, x%y);
   }
   printf("</body>\n");
   printf("</html>\n");

}

int main()
{
    //写任何cgi,都用到下面这部分代码
    char buf[MAX] = {0};
    if(getenv("METHOD"))
    {
        if(strcasecmp(getenv("METHOD"), "GET") == 0)//GET方法->拿query_string
        {
            strcpy(buf, getenv("QUERY_STRING"));
        }
        else//POST方法->拿content-length
        {
            int content_length = atoi(getenv("CONTENT_LENGTH"));
            int i = 0;
            char c;
            for(; i<content_length; ++i)
            {
                read(0, &c, 1);
                buf[i] = c;
            }
            buf[i] = '\0';
        }
    }
   
    //printf("%s\n", buf);
    mycal(buf);

    return 0;
}

#include <fcgi_stdio.h>
 
int main(int argc, char *argv[])
{
    while( FCGI_Accept() >=0 )
    {
        FCGI_printf("Status[200 ok]\n");
        FCGI_printf("Content-Type: text/html\n");
        FCGI_printf("hello world! [from fcgi, in C Lan]\n");
    }
 
    return 0;
}

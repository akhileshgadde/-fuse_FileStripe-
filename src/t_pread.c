/*  C file to read from specific offset 
*   Specify the starting and ending offset in the command line
*   Compiling: gcc -Wall -Werror t_pread.c -o t_pread
*   Running: ./t_pread <file-name> <file-name> <file-name> <file-name> <file-name> <file-name> <file-name> <file-name> <file-name> <start-offset> <end-offset>
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define PATH_MAX        4096
typedef long long int offset_t;

int main(int argc, char **argv)
{
    int ret;
    int fd;
    char buf[7000000];
    char path[PATH_MAX];
    offset_t st_off_t, end_off_t;
    
    if (argc < 3)
    {
        printf("./t_pread <file-name> <st-offset> <end-offset>\n");
        exit(EXIT_FAILURE);   
    } 
    //strcpy(path, "./mntdir/file1_dir/file1.0");
    strcpy(path, argv[1]);
    printf("path: %s\n", path);
    #if 1
    if ((fd = open(argv[1], O_RDONLY)) < 0)
    {
        perror("Open() error\n");
        exit(EXIT_FAILURE);
    }
    #endif
    st_off_t = strtoll(argv[2], NULL, 10);
    end_off_t = strtoll(argv[3], NULL, 10);
    printf("%lld, %lld\n", st_off_t, end_off_t);
    if (end_off_t < st_off_t)
    {
        perror("End offset less than start offset\n");
        close (fd);
        exit(EXIT_FAILURE);
    }
    ret = pread(fd, buf, end_off_t - st_off_t, st_off_t);
    buf[ret] = 0x00;
    printf("Bytes read: %d:\n", ret);
    printf("Read buf:\n%s\n", buf);    
    close (fd);
    return 1;
}

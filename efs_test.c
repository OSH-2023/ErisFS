/* 
 write a test program for a virtual file system.
*/
#include <headers.h>

/*
int init_sd() 
{
    // 1. efs init
    efs_init();

    // 2. filesystem init 
    efs_elmfat_init();

    // 3. storage device init 
    splash_init() // Storage Device

    // 4. storage device mount 
    if (efs_mount( "sd0", "/", "elmfat", 0, 0) != 0)
    {
        printf("File System on SD ('sd0') initialization failed!\n");
        printf("Trying format and remount...\n");
        if (efs_mkfs("elmfat", "sd0") == 0) //format the specified storage device and create a file system)
        {
            if (efs_mount( "sd0", "/", "elmfat", 0, 0) == 0)
            {
                printf("File System on SD ('sd0') initialized!\n");
                return 0;
            }
        }
        return -1;
    }
    else
    {
        printf("File System on SD ('sd0') initialized!\n");
        return 0;
    } 
}
*/


int init() {
    /* 1. efs init */
    efs_init();

    /* 2. filesystem init */
    efs_ramfs_init();

    /* 3. storage device init */


    /* 4. storage device mount */
    if (efs_mount( NULL, "/", "ramfs", 0, 0) != 0) //efs_ramfs_create(rampool, 1024)))== 0
    {
        printf("File System on ram initialization failed!\n");
        return -1;
    }
    else
    {
        printf("File System on ram initialized!\n");
        return 0;
    } 
}

int main()
{
    init();
    int fd;

    // write to file
    fd = efs_open("test.txt", O_RDWR, 0);
    write(fd, "Hello World!", 12);
    close(fd);

    // read from file
    fd = efs_open("test.txt", O_RDWR, 0);
    char buf[12];
    read(fd, buf, 12);
    printf("%s\n", buf);
    close(fd);
    return 0;
}
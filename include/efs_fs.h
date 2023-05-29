#include <efs_device.h>

#define ERIS_EOK 0
#define EFS_FILESYSTEM_TYPES_MAX 4
#define EFS_FILESYSTEMS_MAX 4
#define ERIS_ERROR 1
//来自efs_file 中efs_file_open函数，暂定为0x200000，留待后续对接
#define O_DIRECTORY 0x200000
#define ERIS_DEVICE_OFLAG_RDWR 0x003    /* open the device in read-and_write mode */

struct statfs
{
    size_t f_bsize;   /* block size */
    size_t f_blocks;  /* total data blocks in file system */
    size_t f_bfree;   /* free blocks in file system */
    size_t f_bavail;  /* free blocks available to unprivileged user*/
};

/* Pre-declaration */
struct efs_filesystem;
struct efs_file;

/* File system operations */
struct efs_filesystem_ops
{
    char *name;
    uint32_t flags;      /* flags for file system operations */

    /* operations for file */
    const struct efs_file_ops *fops;

    /* mount and unmount file system */
    int (*mount)    (struct efs_filesystem *fs, unsigned long rwflag, const void *data);
    int (*unmount)  (struct efs_filesystem *fs);

    /* make a file system */
    int (*mkfs)     (eris_device_t dev_id, const char *fs_name);
    int (*statfs)   (struct efs_filesystem *fs, struct statfs *buf);

    int (*unlink)   (struct efs_filesystem *fs, const char *pathname);
    int (*stat)     (struct efs_filesystem *fs, const char *filename, struct stat *buf);
    int (*rename)   (struct efs_filesystem *fs, const char *oldpath, const char *newpath);
};

/* Mounted file system */
struct efs_filesystem
{
    eris_device_t dev_id;     /* Attached device */

    char *path;             /* File system mount point */
    const struct efs_filesystem_ops *ops; /* Operations for file system type */

    void *data;             /* Specific file system data */
};

/*文件系统分区表 */
struct efs_partition
{
    uint8_t type;        /* 文件系统类型 */
    off_t  offset;       /* partition start offset */
    size_t size;         /* 分区大小 */
    eris_sem_t lock;
};

/*挂载表*/
struct efs_mount_tbl
{   
    /*设备名*/
    const char   *device_name;
    /*挂载路径*/
    const char   *path;
    /*文件系统类型*/
    const char   *filesystemtype;
    /*读写标志位*/
    unsigned long rwflag;
    /*用户数据*/
    const void   *data;
};

/* 注册文件系统 将文件系统注册到DFS框架中
** 参数：ops 文件系统操作方法
** 返回值：0 注册成功，-1 失败。
*/ 
int efs_register(const struct efs_filesystem_ops *ops);

/* 查找指定路径上的文件系统
** 参数：path 路径字符串
** 返回值：成功返回挂载的文件系统，失败则返回NULL。
*/
struct efs_filesystem *efs_filesystem_lookup(const char *path);

/* 返回指定设备挂载路径
** 参数：device	已挂载的设备对象。
** 返回值 成功返回已挂载路径；失败则返回NULL。
*/
const char *efs_filesystem_get_mounted_path(struct eris_device *device);

/* 获取分区表
** 参数：
**  part	返回的分区表结构。
**  buf	存储分区表的缓冲区。
**  pindex	要获取的分区表的索引。
** 返回值：ERIS_EOK 成功；-ERIS_ERROR 失败
*/
int efs_filesystem_get_partition(struct efs_partition *part, uint8_t *buf, uint32_t pindex);

/* 挂载文件系统，将文件系统和具体的存储设备关联起来
** 参数：
**  device_name	一个包含了文件系统的设备名
**  path	挂载文件系统的路径，即挂载点
**  filesystemtype	需要挂载的文件系统类型
**  rwflag	读写标志位
**  data	文件系统的私有数据
** 返回值：0 挂载成功，-1 失败。
*/
int efs_mount(const char *device_name, const char *path, const char *filesystemtype, unsigned long rwflag, const void *data);

/* 卸载文件系统
** 参数：specialfile	挂载了文件系统的指定路径
** 返回值：0 卸载成功，-1 失败。
*/
int efs_unmount(const char *specialfile);

/* 格式化文件系统
** 参数：
**  fs_name	文件系统名
**  device_name	需要格式化文件系统的设备名
** 返回值：0 格式化成功，-1 失败。
*/
int efs_mkfs(const char *fs_name, const char *device_name);

/* 获取已挂载的文件系统信息
** 参数：
**  path 已挂载的指定文件系统路径
**  buffer 信息存储缓冲区
** 返回值：0 成功，否则失败
*/
int efs_statfs(const char *path, struct statfs *buffer);

/* 挂载设备
** 参数：dev 需挂载的设备
** 返回值：RT_EOK 成功；-RT_ERROR 失败
*/
int efs_mount_device(eris_device_t dev);

/* 卸载设备
** 参数：dev 需挂载的设备
** 返回值：0 成功，-1 失败。
*/
int efs_unmount_device(eris_device_t dev);


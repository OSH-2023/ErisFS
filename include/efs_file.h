/*
 * @Description: 
 * @Version: 
 * @Author: Tyrion Huu
 * @Date: 2023-05-18 15:14:16
 * @LastEditors: Tyrion Huu
 * @LastEditTime: 2023-05-19 17:19:17
 */
#ifndef __EFS_FILE_H__
#define __EFS_FILE_H__

#include <efs.h>
#include <efs_fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* file descriptor */
#define EFS_FD_MAGIC     0xfdfd

/**
 * Double List structure
 */
struct list_node
{
    struct list_node *next;                          /**< point to next node. */
    struct list_node *prev;                          /**< point to prev node. */
};
typedef struct list_node list_t;                  /**< Type for lists. */

// struct rt_pollreq;

struct efs_file_ops
{
    int (* open)     (struct efs_file * fd);
    int (* close)    (struct efs_file * fd);
    int (* ioctl)    (struct efs_file * fd, int cmd, void * args);
    int (* read)     (struct efs_file * fd, void * buf, size_t count);
    int (* write)    (struct efs_file * fd, const void * buf, size_t count);
    int (* flush)    (struct efs_file * fd);
    int (* lseek)    (struct efs_file * fd, off_t offset);
    // int (*getdents) (struct efs_file * fd, struct dirent * dirp, uint32_t count);

    // int (*poll)     (struct efs_file *fd, struct rt_pollreq *req);
};

struct efs_vnode
{
    uint16_t type;                      /* Type (regular or socket) */

    char * path;                        /* Name (below mount point) */
    char * fullpath;                    /* Full path is hash key */
    int ref_count;                      /* Descriptor reference count */
    list_t list;                        /* The node of vnode hash table */

    struct efs_filesystem * fs;         /* File system */
    const struct efs_file_ops * fops;   /* File operations */
    uint32_t flags;                     /* self flags, is dir etc.. */

    size_t size;                        /* Size in bytes */
    void * data;                        /* Specific file system data */
};

struct efs_file
{
    // uint16_t magic;              /* file descriptor magic number */
    uint32_t flags;                 /* Descriptor flags */
    int ref_count;                  /* Descriptor reference count */
    off_t pos;                      /* Current file position */
    struct efs_vnode * vnode;       /* file node struct */
    void * data;                    /* Specific fd data */
};

/*
    * @Description: This struct is likely used in a program that needs to map a file into memory. 
                    The fields in this struct provide information about how the file should be mapped.
    * @Version:
*/
struct efs_mmap2_args
{
    void * addr;        /* User address */
    size_t length;      /* Length of mapping */
    int prot;           /* Mapping protection */
    int flags;          /* Mapping flags */
    off_t pgoffset;     /* Offset to page boundary */
    void * ret;         /* Return value */
};

/*
    * @Description: This function is likely used to initialize the vnode manager in a file system. 
                    The vnode manager is responsible for managing the virtual node (vnode) objects that represent files and directories in the file system. 
                    By calling this function, the vnode manager is initialized and ready to manage the vnodes in the file system.
    * @Version:
*/
void efs_vnode_mgr_init(void);

// int efs_file_is_open(const char *pathname);

/*
    * @Description: This function is likely used to open a file in a file system. 
                    The file is specified by the path argument. 
                    The flags argument specifies how the file should be opened. 
                    The return value is a file descriptor that can be used to access the file.
    * @Version:
*/
int efs_file_open(struct efs_file * fd, const char * path, int flags);

/*
    * @Description: This function is likely used to close a file in a file system. 
                    The file is specified by the fd argument. 
                    The return value is zero if the file is closed successfully. 
                    Otherwise, a negative value is returned.
    * @Version:
*/
int efs_file_close(struct efs_file * fd);

// int efs_file_ioctl(struct efs_file *fd, int cmd, void *args);

// int efs_file_read(struct efs_file *fd, void *buf, size_t len);

// int efs_file_getdents(struct efs_file *fd, struct dirent *dirp, size_t nbytes);

// int efs_file_unlink(const char *path);

/* 
    * @Description: This function is likely used to write data to a file in a file system. 
                    The file is specified by the fd argument. 
                    The data to be written is specified by the buf argument. 
                    The number of bytes to write is specified by the len argument. 
                    The return value is the number of bytes written. 
                    A negative value is returned if an error occurs.
    * @Version:
*/
int efs_file_write(struct efs_file * fd, const void * buf, size_t len);

// int efs_file_flush(struct efs_file *fd);

// int efs_file_lseek(struct efs_file *fd, off_t offset);

// int efs_file_stat(const char *path, struct stat *buf);

// int efs_file_rename(const char *oldpath, const char *newpath);

// int efs_file_ftruncate(struct efs_file *fd, off_t length);

// int efs_file_mmap2(struct efs_file *fd, struct efs_mmap2_args *mmap2);

// /* 0x5254 is just a magic number to make these relatively unique ("RT") */
// #define RT_FIOFTRUNCATE  0x52540000U
// #define RT_FIOGETADDR    0x52540001U
// #define RT_FIOMMAP2      0x52540002U

#ifdef __cplusplus
}
#endif

#endif

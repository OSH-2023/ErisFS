/*
 * @Description: 
 * @Version: 
 * @Author: Tyrion Huu
 * @Date: 2023-06-06 13:53:58
 * @LastEditors: Tyrion Huu
 * @LastEditTime: 2023-07-04 11:29:38
 */
#ifndef __EFS_FILE_H__
#define __EFS_FILE_H__

#include "headers.h"



/* file descriptor */
#define EFS_FD_MAGIC     0xfdfd

/**
 * rt_container_of - return the start address of struct type, while ptr is the
 * member of struct type.
 */
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - (unsigned long) (&((type *)0)->member)))
    

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
    int (* getdents) (struct efs_file * fd, struct dirent * dirp, uint32_t count);
};

struct efs_vnode
{
    uint16_t type;                      /* Type (regular or socket) */

    char * path;                        /* Name (below mount point) */
    char * fullpath;                    /* Full path is hash key */
    int ref_count;                      /* Descriptor reference count */
    eris_list_t list;                        /* The node of vnode hash table */

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

void efs_vnode_mgr_init(void);

// int efs_file_is_open(const char *pathname);

int efs_file_open(struct efs_file * fd, const char * path, int flags);

int efs_file_close(struct efs_file * fd);

// int efs_file_ioctl(struct efs_file *fd, int cmd, void *args);

int efs_file_read(struct efs_file * fd, void * buf, size_t len);

int efs_file_getdents(struct efs_file *fd, struct dirent *dirp, size_t nbytes);

int efs_file_unlink(const char *path);

int efs_file_write(struct efs_file * fd, const void * buf, size_t len);

// int efs_file_flush(struct efs_file *fd);

off_t efs_file_lseek(struct efs_file *fd, off_t offset);

int efs_file_stat(const char *path, struct stat_efs *buf);

int efs_file_rename(const char *old_path, const char *new_path);

int efs_file_ftruncate(struct efs_file *fd, off_t length);

// int efs_file_mmap2(struct efs_file *fd, struct efs_mmap2_args *mmap2);

// /* 0x5254 is just a magic number to make these relatively unique ("RT") */
// #define RT_FIOFTRUNCATE  0x52540000U
// #define RT_FIOGETADDR    0x52540001U
// #define RT_FIOMMAP2      0x52540002U

#endif

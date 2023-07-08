
#ifndef __EFS_H__
#define __EFS_H__
#include "headers.h"
#define EFS_PATH_MAX 256
#define EFS_MAX_FD 16

#define EFS_FS_FLAG_DEFAULT     0x00    /* default flag */
#define EFS_FS_FLAG_FULLPATH    0x01    /* set full path to underlaying file system */

/* File types */
#define FT_REGULAR               0   /* regular file */
#define FT_SOCKET                1   /* socket file  */
#define FT_DIRECTORY             2   /* directory    */
#define FT_USER                  3   /* user defined */
#define FT_DEVICE                4   /* device */

/* File flags */
#define EFS_F_OPEN              0x01000000
#define EFS_F_DIRECTORY         0x02000000
#define EFS_F_EOF               0x00000000
#define EFS_F_ERR               0x08000000

/* open-only flags */
#define O_RDONLY        0x0000          /* open for reading only */
#define O_WRONLY        0x0001          /* open for writing only */
#define O_RDWR          0x0002          /* open for reading and writing */
#define O_ACCMODE       0x0003          /* mask for above modes */

/*
 * Kernel encoding of open mode; separate read and write bits that are
 * independently testable: 1 greater than the above.
 *
 * XXX
 * FREAD and FWRITE are excluded from the #ifdef KERNEL so that TIOCFLUSH,
 * which was documented to use FREAD/FWRITE, continues to work.
 */
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define FREAD           0x00000001
#define FWRITE          0x00000002
#endif
#define O_NONBLOCK      0x00000004      /* no delay */
#define O_APPEND        0x00000008      /* set append mode */

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define O_SHLOCK        0x00000010      /* open with shared file lock */
#define O_EXLOCK        0x00000020      /* open with exclusive file lock */
#define O_ASYNC         0x00000040      /* signal pgrp when data ready */
#define O_FSYNC         O_SYNC          /* source compatibility: do not use */
#define O_NOFOLLOW      0x00000100      /* don't follow symlinks */
#endif /* (_POSIX_C_SOURCE && !_DARWIN_C_SOURCE) */
#define O_CREAT         0x00000200      /* create if nonexistant */
#define O_TRUNC         0x00000400      /* truncate to zero length */
#define O_EXCL          0x00000800      /* error if already exists */

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define O_EVTONLY       0x00008000      /* descriptor requested for event notifications only */
#endif


#define O_NOCTTY        0x00020000      /* don't assign controlling terminal */


#if __DARWIN_C_LEVEL >= 200809L
#define O_DIRECTORY     0x00100000
#endif

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define O_SYMLINK       0x00200000      /* allow open of a symlink */
#endif

//      O_DSYNC         0x00400000      /* synch I/O data integrity */


#if __DARWIN_C_LEVEL >= 200809L
#define O_CLOEXEC       0x01000000      /* implicitly set FD_CLOEXEC */
#endif


#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define O_NOFOLLOW_ANY  0x20000000      /* no symlinks allowed in path */
#endif

#if __DARWIN_C_LEVEL >= 200809L
#define O_EXEC          0x40000000               /* open file for execute only */
#define O_SEARCH        (O_EXEC | O_DIRECTORY)   /* open directory for search only */
#endif

typedef unsigned short mode_t;
typedef int dev_t;
typedef unsigned long long ino_t;
typedef unsigned short nlink_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long long blkcnt_t;
typedef int blksize_t;
typedef signed long off_t;
typedef long time_t;
struct stat_efs {
	dev_t           st_dev;         /* [XSI] ID of device containing file */
	ino_t           st_ino;         /* [XSI] File serial number */
	mode_t          st_mode;        /* [XSI] Mode of file (see below) */
	nlink_t         st_nlink;       /* [XSI] Number of hard links */
	uid_t           st_uid;         /* [XSI] User ID of the file */
	gid_t           st_gid;         /* [XSI] Group ID of the file */
	dev_t           st_rdev;        /* [XSI] Device ID */
    time_t          st_atime;       /* [XSI] Time of last access */
	long            st_atimensec;   /* nsec of last access */
	time_t          st_mtime;       /* [XSI] Last data modification time */
	long            st_mtimensec;   /* last data modification nsec */
	time_t          st_ctime;       /* [XSI] Time of last status change */
	long            st_ctimensec;   /* nsec of last status change */

	off_t           st_size;        /* [XSI] file size, in bytes */
	blkcnt_t        st_blocks;      /* [XSI] blocks allocated for file */
	blksize_t       st_blksize;     /* [XSI] optimal blocksize for I/O */
	unsigned int      st_flags;       /* user defined flags for file */
	unsigned int      st_gen;         /* file generation number */
	int              st_lspare;      /* RESERVED: DO NOT USE! */
	long long       st_qspare[2];   /* RESERVED: DO NOT USE! */
};


#define EPERM 1
#define ENOENT 2
#define ENOFILE ENOENT
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define ENXIO 6
#define E2BIG 7
#define ENOEXEC 8
#define EBADF 9
#define ECHILD 10
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define ENFILE 23
#define EMFILE 24
#define ENOTTY 25
#define EFBIG 27
#define ENOSPC 28
#define ESPIPE 29
#define EROFS 30
#define EMLINK 31
#define EPIPE 32
#define EDOM 33
#define EDEADLK 36
#define ENAMETOOLONG 38
#define ENOLCK 39
#define ENOSYS 40
#define ENOTEMPTY 41

#ifndef RC_INVOKED
#if !defined(_SECURECRT_ERRCODE_VALUES_DEFINED)
#define _SECURECRT_ERRCODE_VALUES_DEFINED
#define STRUNCATE 80
#endif
#endif
/*
 * [XSI] The symbolic names for file modes for use as values of mode_t
 * shall be defined as described in <sys/stat.h>
 */
#ifndef S_IFMT
/* File type */
#define S_IFMT          0170000         /* [XSI] type of file mask */
#define S_IFIFO         0010000         /* [XSI] named pipe (fifo) */
#define S_IFCHR         0020000         /* [XSI] character special */
#define S_IFDIR         0040000         /* [XSI] directory */
#define S_IFBLK         0060000         /* [XSI] block special */
#define S_IFREG         0100000         /* [XSI] regular */
#define S_IFLNK         0120000         /* [XSI] symbolic link */
#define S_IFSOCK        0140000         /* [XSI] socket */
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define S_IFWHT         0160000         /* OBSOLETE: whiteout */
#endif

/* File mode */
/* Read, write, execute/search by owner */
#define S_IRWXU         0000700         /* [XSI] RWX mask for owner */
#define S_IRUSR         0000400         /* [XSI] R for owner */
#define S_IWUSR         0000200         /* [XSI] W for owner */
#define S_IXUSR         0000100         /* [XSI] X for owner */
/* Read, write, execute/search by group */
#define S_IRWXG         0000070         /* [XSI] RWX mask for group */
#define S_IRGRP         0000040         /* [XSI] R for group */
#define S_IWGRP         0000020         /* [XSI] W for group */
#define S_IXGRP         0000010         /* [XSI] X for group */
/* Read, write, execute/search by others */
#define S_IRWXO         0000007         /* [XSI] RWX mask for other */
#define S_IROTH         0000004         /* [XSI] R for other */
#define S_IWOTH         0000002         /* [XSI] W for other */
#define S_IXOTH         0000001         /* [XSI] X for other */

#define S_ISUID         0004000         /* [XSI] set user id on execution */
#define S_ISGID         0002000         /* [XSI] set group id on execution */
#define S_ISVTX         0001000         /* [XSI] directory restrcted delete */

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define S_ISTXT         S_ISVTX         /* sticky bit: not supported */
#define S_IREAD         S_IRUSR         /* backward compatability */
#define S_IWRITE        S_IWUSR         /* backward compatability */
#define S_IEXEC         S_IXUSR         /* backward compatability */
#endif
#endif  /* !S_IFMT */


/* */
#define EFS_FIOFTRUNCATE  0x70820000U //FR


struct efs_fdtable
{
    unsigned int maxfd;
    struct efs_file **fds;    /* efs_file.h/efs_fd */
    int used;               /* NEW! for check if full*/
};

//#ifndef HAVE_DIR_STRUCTURE
//#define HAVE_DIR_STRUCTURE
//typedef struct
//{
//    int fd;                         /* directory file */
//    char buf[512];
//    int num;
//    int cur;
//}DIR;
//#endif

/* Initialization of efs */
int efs_init(void);

char *efs_normalize_path(const char *directory, const char *filename);

const char *efs_subdir(const char *directory, const char *filepath);

void efs_fd_lock(void);

void efs_fd_unlock(void);

void efs_lock(void);

void efs_unlock(void);


/* FD APIs */
/* fd_num/fd(int) -> fd(efs_file) -> vnode */
// int fd_slot_expand(struct efs_fdtable *fdt, int fd);
// int fd_slot_alloc(struct efs_fdtable *fdt, int startfd);
int fd_new(void);

int fdt_fd_new(struct efs_fdtable *fdt);

int fd_creat(struct efs_fdtable *fdt, int startfd);

int fd_expand(struct efs_fdtable *fdt, int fd);

struct efs_file *fd_get(int fd_num);

struct efs_file *fdt_fd_get(struct efs_fdtable* fdt, int fd);

int get_fd_index(struct efs_file *fd);

int fdt_get_fd_index(struct efs_fdtable *fdt, struct efs_file *fd);

void fd_release(int fd);

void fdt_fd_release(struct efs_fdtable* fdt, int fd);

void fd_init(struct efs_file *fd);

struct efs_fdtable *efs_fdtable_get(void);

//
int dup_efs(int oldfd, int startfd);

#endif

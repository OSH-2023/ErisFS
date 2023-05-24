#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

struct efs_fdtable
{
    uint32_t maxfd;
    struct efs_fd **fds;    /* efs_file.h/efs_fd */
    int used;               /* NEW! for check if full*/
};

/* Initialization of efs */
int efs_init(void);

char *efs_normalize_path(const char *directory, const char *filename);

void efs_fd_lock(void);
void efs_fd_unlock(void);
void efs_fm_lock(void);
void efs_fm_unlock(void);

/* FD APIs */
// int fd_slot_expand(struct efs_fdtable *fdt, int fd);
// int fd_slot_alloc(struct efs_fdtable *fdt, int startfd);
// int fd_alloc(struct efs_fdtable *fdt, int startfd);
int fd_new(void);
//int fdt_fd_new(struct efs_fdtable *fdt);
struct efs_fd *fd_get(int fd_num);
// struct efs_fd *fdt_fd_get(struct efs_fdtable* fdt, int fd);
int fd_get_fd_index(struct efs_fd *file);
// int fd_get_fd_index(struct efs_fd *file);
void fd_release(int fd);
// void fdt_fd_release(struct efs_fdtable* fdt, int fd)
void fd_init(struct efs_fd *fd);

struct efs_fdtable *efs_fdtable_get(void);
#include "FreeRTOS.h"
#include "task.h"

#include <efs.h>
#include <efs_file.h>
#include <efs_fs.h>
#include <efs_posix.h>
static struct efs_fdtable _fdtab;

/* get a new fd_num */
int fd_new(void) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_new(fdt);
}

/* get a new fd_num from specific fdt */
int fdt_fd_new(struct efs_fdtable *fdt) {
    /* fdt: the efs file descriptor table */
    /* fd_num: the number of fd */
    efs_fd_lock();

    int fd_num;
    fd_num = fd_alloc(fdt, 0);;
    if(fd_num < 0) {
        printf("[efs.c] fdt_fd_new is failed! Couldn't found an empty fd entry!\n");
    }

    efs_fd_unlock();
    return fd_num;
    
}

/* create a new fd and return its fd_num */
int fd_creat(struct efs_fdtable *fdt, int start_fd_num) {
    /* fdt: the efs file descriptor table */
    /* fd_num: the number of fd */
    /* fd: struct efs_fd */

    int fd_num;

    /* find the first empty fd_num larger than 0 */
    for (fd_num = 0; fd_num < (int)fdt->maxfd; fd_num++) {
        if(fdt->fds[fd_num] == NULL) {
            break;
        }
    }
    struct efs_file *fd;
    if (fd_num == fdt->maxfd) {
        // If fdt full, expand fdt
        /*
        if (fd_slot_expand(fdt, fd_num) < 0) {
            return -1;
        }
        */
       printf("[efs.c] fd_creat is failed! fdt is full!\n");
       return -1;
    }
    else {
        fd = (struct efs_fd *)pvPortMalloc(sizeof(struct efs_file));
        if (fd == NULL) {
            printf("[efs.c] fd_creat is failed! fd entry allocate failed!\n");
            return -1;
        }
        fd -> ref_count = 1;
        fd -> vnode = NULL;
        fdt->fds[fd_num] = fd;
        return fd_num;
    }
    
}

struct efs_file *fd_get(int fd) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_get(fdt, fd);
}

struct efs_file *fdt_fd_get(struct efs_fdtable* fdt, int fd) {
    if(fd < 0 || fd >= (int)fdt->maxfd) {
        printf("[efs.c] fdt_fd_get is failed! fd_num is illegal!\n");
        return NULL;
    }

    struct efs_file *fd;
    efs_file_lock();
    fd = fdt->fds[fd];
    if (fd == NULL) {
        efs_file_unlock();
        printf("[efs.c] fdt_fd_get is failed! fd entry is NULL!\n");
        return NULL;
    }
    else {
        //fd->ref_count++;
        efs_file_unlock();
        return fd;
    }
    

}


int get_fd_index(struct efs_file *fd) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_get_fd_index(fdt, fd);
}

int fdt_get_fd_index(struct efs_fdtable *fdt, struct efs_file *fd) {
    if(fd == NULL) {
        printf("[efs.c] fdt_get_fd_index is failed! fd is NULL!\n");
        return -1;
    }

    int fd_num = -1;
    efs_file_lock();
    for(int i = 0; i < (int)fdt->maxfd; i++) {
        if(fdt->fds[i] == fd) {
            fd_num = i;
            dfs_file_unlock();
            return fd_num;
        }
    }

    dfs_file_unlock();
    printf("[efs.c] fdt_get_fd_index is failed! fd is not in fdt!\n");
    return -1;
}

void fd_release(int fd) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_release(fdt, fd);
}

void fdt_fd_release(struct efs_fdtable* fdt, int fd) {
    if(fd < 0 || fd >= (int)fdt->maxfd) {
        printf("[efs.c] fdt_fd_release is failed! fd_num is illegal!\n");
        return ;
    }

    struct efs_file *fd_entry;
    efs_file_lock();
    fd_entry = fdt->fds[fd];
    if (fd_entry == NULL) {
        efs_file_unlock();
        printf("[efs.c] fd entry is already NULL!\n");
        return ;
    }
    else {
        fdt->fds[fd] = NULL;
        fd_entry->ref_count--;
        if (fd_entry -> ref_count == 0) {
            struct efs_vnode *vnode = fd_entry->vnode;
            vnode->ref_count--;
            if (vnode->ref_count == 0) {
                vPortFree(vnode);
            }
            fd_entry->vnode = NULL;
            vPortFree(fd_entry);
        }
        efs_file_unlock();
        return ;
    }

}

void fd_init(struct efs_file *fd) {
    if(fd) {
        fd->flags = 0;
        fd->ref_count = 1;
        fd->pos = 0;
        fd->vnode = NULL;
        fd->data = NULL;

    }
}

struct efs_fdtable *efs_fdtable_get(void) {
    struct dfs_fdtable *fdt = &_fdtab;
    return fdt;
}



int efs_init(void);
char *efs_normalize_path(const char *directory, const char *filename);
void efs_fd_lock(void);
void efs_fd_unlock(void);
void efs_fm_lock(void);
void efs_fm_unlock(void);
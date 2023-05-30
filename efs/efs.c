#include <FreeRTOS.h>
#include <semphr.h>

#include <efs.h>
#include <efs_file.h>
#include <efs_fs.h>
#include <efs_posix.h>

struct efs_filesystem_ops *filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX];
struct efs_filesystem filesystem_table[EFS_FILESYSTEMS_MAX];
static struct efs_fdtable _fdtab;
char working_directory[EFS_PATH_MAX] = {"/"};


static SemaphoreHandle_t xEfsMutex;
static SemaphoreHandle_t xEfsFileMutex;

/* get a new fd_num */
int fd_new(void) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_new(fdt);
}

/* get a new fd_num from specific fdt */
/* fdt: the efs file descriptor table */
/* fd_num: the number of fd */
int fdt_fd_new(struct efs_fdtable *fdt) {

    efs_file_lock();

    int fd_num;
    fd_num = fd_alloc(fdt, 0);;
    if(fd_num < 0) {
        printf("[efs.c] fdt_fd_new is failed! Couldn't found an empty fd entry!\n");
    }

    efs_file_unlock();
    return fd_num;
    
}

/* create a new fd and return its fd_num */
/* fdt: the efs file descriptor table */
/* fd_num: the number of fd */
/* fd: struct efs_fd */
int fd_creat(struct efs_fdtable *fdt, int start_fd_num) {

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

/* get efs_file corresponding to fd */
struct efs_file *fd_get(int fd) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_get(fdt, fd);
}

/* get efs_file corresponding to fd in specific fdt */
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

/* get efs_file's fd */
int get_fd_index(struct efs_file *fd) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_get_fd_index(fdt, fd);
}

/* get efs_file's fd in specific fdt */
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

/* release fd */
void fd_release(int fd) {
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_release(fdt, fd);
}

/* release fd in specific fdt */
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

/* init efs_file */
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


/* init efs */
int efs_init(void) {
    static BaseType_t init_ready = pdFALSE;

    if (init_ready) {
        printf("efs already init!\n");
        return 0;
        
    }
    /* init vnode hash table */
    efs_vnode_mgr_init();
    /* init filesystem operation table */
    memset((void *)filesystem_operation_table, 0, sizeof(filesystem_operation_table));
    /* init filesystem table */
    memset(filesystem_table, 0, sizeof(filesystem_table));
    /* init fd table */
    memset(&_fdtab, 0, sizeof(_fdtab));

    /* init filesystem lock */
    xEfsMutex = xSemaphoreCreateMutex();
    if (xEfsMutex == NULL) {
        printf("[efs.c] efs_init is failed! xEfsMutex create failed!\n");
        return -1;
    }
    /* init fd lock */
    xEfsFileMutex = xSemaphoreCreateMutex();
    if (xEfsFileMutex == NULL) {
        printf("[efs.c] efs_init is failed! xEfsFileMutex create failed!\n");
        return -1;
    }
    init_ready = pdTRUE;
    printf("efs inited.\n");
    return 0;
}

// TODO
char *efs_normalize_path(const char *directory, const char *filename) {
    return "TODO";
}

void efs_lock(void) {
    xSemaphoreTake( xEfsMutex, portMAX_DELAY );
}

void efs_unlock(void) {
    xSemaphoreGive( xEfsMutex );
}

void efs_file_lock(void) {
    xSemaphoreTake( xEfsFileMutex, portMAX_DELAY );
}

void efs_file_unlock(void) {
    xSemaphoreGive( xEfsFileMutex );
}
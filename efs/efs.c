#include "headers.h"

struct efs_filesystem_ops *filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX];
struct efs_filesystem filesystem_table[EFS_FILESYSTEMS_MAX];
static struct efs_fdtable _fdtab;


static SemaphoreHandle_t xEfsMutex;
static SemaphoreHandle_t xEfsFileMutex;

/* normalize path by removing . and .. in  the path */
/* normalize the combination of directory and file name if filename[0] != '/' */
/* normalize only the filename if filename[0] == '/' */
/* "/home", ".././././/data///.././/data/.//test.c" -> "/data.c" */
char *efs_normalize_path(const char *directory, const char *filename)
{   
    char *fullpath;

    if ((directory == NULL) && (filename[0] != '/'))
    {
        printf("[efs.c] efs_normalize_path is failed! directory is NULL and filename is not absolute path!\n");
        return NULL;
    }
    if (filename[0] != '/') /* relative path, join it with directory */
    {
        fullpath = (char *)pvPortMalloc(strlen_efs(directory) + strlen_efs(filename) + 2);
        if (fullpath == NULL){
            printf("[efs.c] efs_normalize_path is failed! fullpath is NULL!\n");
            return NULL;
        }
        /* join path and file name */
        strcpy(fullpath, directory);
        strcat(fullpath, "/");
        strcat(fullpath, filename);
        strcat(fullpath, "\0");
    }
    else /* absolute path, use it directly */
    {
        fullpath = (char *)pvPortMalloc(strlen_efs(filename) + 1);
        if (fullpath == NULL){
            printf("[efs.c] efs_normalize_path is failed! fullpath is NULL!\n");
            return NULL;
        }            
        strcpy(fullpath, filename); 
    }
    int len = strlen_efs(fullpath);
    char* normalized = (char *)pvPortMalloc((len + 2) * sizeof(char));

    int i = 0; // current position in the path
    int j = 0; // current position in the normalized path

    while (fullpath[i] != '\0') 
    {
        if (fullpath[i] == '/') 
        {
            // handle consecutive '/'
            while (fullpath[i + 1] == '/') 
            {
                i++;
            }

            if (fullpath[i + 1] == '.' && fullpath[i + 2] == '.' && (fullpath[i + 3] == '/' || fullpath[i + 3] == '\0')) 
            {
                // handle '..'
                while (normalized[j - 1] != '/') 
                {
                    j--; // go back to the previous '/'
                    if(j == 0) 
                    {
                        printf("[efs.c] efs_normalize_path is failed! invalid path!\n");
                        return NULL;
                    }
                }
                if (j > 0) 
                {
                    j--;
                }
                i += 3; //jump to next '/'
            }
            else if (fullpath[i + 1] == '.' && (fullpath[i + 2] == '/' || fullpath[i + 2] == '\0')) 
            {
                // handle '.' 
                i += 2; //jump to next '/'
            }
            else 
            {
                normalized[j++] = '/';
                i++;
            }
        }
        else 
        {
            normalized[j++] = fullpath[i++];
        }
        //printf("normalized: %s\n", normalized);
    }

    // terminate the normalized path
    normalized[j] = '\0';
    //printf("length of normalized: %d\n", strlen_efs(normalized));
    vPortFree(fullpath);
    return normalized;
}

/* return subdir based only on length */
/* "/home", "/home/1.txt" -> "/1.txt" */
/* "/abcd", "/home/1.txt" -> "/1.txt" */
const char *efs_subdir(const char *directory, const char *filepath)
{
    const char *dir;
    char * subdir = pvPortMalloc(strlen_efs(filepath) + 1);

    if (strlen_efs(directory) >= strlen_efs(filepath)) /* same length */
        return NULL;

    dir = filepath + strlen_efs(directory);
    

    if ((*dir != '/') && (dir != filepath))
    {
        dir --;
    }

    strcpy(subdir, dir);
    return subdir;
}

void efs_lock() 
{
    xSemaphoreTake( xEfsMutex, portMAX_DELAY );
}

void efs_unlock() 
{
    xSemaphoreGive( xEfsMutex );
}

void efs_file_lock() 
{
    xSemaphoreTake( xEfsFileMutex, portMAX_DELAY );
}

void efs_file_unlock() 
{
    xSemaphoreGive( xEfsFileMutex );
} 

int fd_expand(struct efs_fdtable *fdt, int fd)
{
    if (fd < (int)fdt->maxfd) 
    {
        return fd;
    }
    if (fd > EFS_MAX_FD)
    {
        printf("[efs.c] fd_expand is failed! fd is larger than EFS_MAX_FD!\n");
        return -1;
    }

    int nd = fd + 4;
    if (nd > EFS_MAX_FD)
    {
        nd = EFS_MAX_FD;
    }
    struct efs_file **fds = (struct efs_fd **)pvPortRealloc(fdt->fds, nd * sizeof(struct efs_fd *));
    if (!fds)
    {
        return -1;
    }

    /* clean the new allocated fds */
    for (int i = fdt->maxfd; i < nd; i ++)
    {
        fds[i] = NULL;
    }
    fdt->fds   = fds;
    fdt->maxfd = nd;
    return fd;

}

/* create a new fd and return its fd_num */
/* fdt: the efs file descriptor table */
/* fd_num: the number of fd */
/* fd: struct efs_fd */
int fd_creat(struct efs_fdtable *fdt, int start_fd_num) 
{

    int fd_num;
    /*
    if (start_fd_num > fdt->maxfd) 
    {
        printf("[efs.c] fd_creat is failed! start_fd_num is larger than maxfd!\n");
        return -1;
    }
    */

    /* find the first empty fd_num larger than 0 */
    for (fd_num = start_fd_num; fd_num < (int)fdt->maxfd; fd_num++) 
    {
        if(fdt->fds[fd_num] == NULL) 
        {
            break;
        }
    }
    struct efs_file *fd;
    if (fd_num >= (int)fdt->maxfd) 
    {
        if (fd_expand(fdt, fd_num) < 0) 
        {
            printf("[efs.c] fd_creat is failed! fdt is full!\n");
            return -1;
        }
    }
    fd = (struct efs_file *)pvPortMalloc(sizeof(struct efs_file));
    if (fd == NULL) 
    {
        printf("[efs.c] fd_creat is failed! fd entry allocate failed!\n");
        return -1;
    }
    fd -> ref_count = 1;
    fd -> vnode = NULL;
    fdt -> fds[fd_num] = fd;
    fdt -> used ++;
    return fd_num;
    
}

/* get a new fd_num from specific fdt */
/* fdt: the efs file descriptor table */
/* fd_num: the number of fd */
int fdt_fd_new(struct efs_fdtable *fdt) 
{

    efs_file_lock();

    int fd_num;
    fd_num = fd_creat(fdt, 0);
    if(fd_num < 0) 
    {
        //printf("[efs.c] fdt_fd_new is failed! Couldn't found an empty fd entry!\n");
        efs_file_unlock();
        return -1;
    }

    efs_file_unlock();
    return fd_num;
    
}

/* get efs_file corresponding to fd */
struct efs_file *fd_get(int fd) 
{
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_get(fdt, fd);
}

/* get efs_file corresponding to fd in specific fdt */
struct efs_file *fdt_fd_get(struct efs_fdtable* fdt, int fd) 
{
    printf("fd: %d\n", fd);
    if(fd < 0 || fd >= (int)fdt->maxfd) 
    {
        printf("[efs.c] fdt_fd_get is failed! fd_num is illegal!\n");
        return NULL;
    }

    struct efs_file *fd_entry;
    efs_file_lock();
    fd_entry = fdt->fds[fd];
    if (fd_entry == NULL) 
    {
        efs_file_unlock();
        printf("[efs.c] fdt_fd_get is failed! fd entry is NULL!\n");
        return NULL;
    }
    else 
    {
        //fd_entry->ref_count++;
        efs_file_unlock();
        return fd_entry;
    }
    

}

/* get a new fd_num */
int fd_new(void) 
{
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_new(fdt);
}

/* get efs_file's fd in specific fdt */
int fdt_get_fd_index(struct efs_fdtable *fdt, struct efs_file *fd) 
{
    if(fd == NULL) 
    {
        printf("[efs.c] fdt_get_fd_index is failed! fd is NULL!\n");
        return -1;
    }

    int fd_num = -1;
    efs_file_lock();
    for(int i = 0; i < (int)fdt->maxfd; i++) 
    {
        if(fdt->fds[i] == fd) 
        {
            fd_num = i;
            efs_file_unlock();
            return fd_num;
        }
    }

    efs_file_unlock();
    printf("[efs.c] fdt_get_fd_index is failed! fd is not in fdt!\n");
    return -1;
}

/* get efs_file's fd */
int get_fd_index(struct efs_file *fd) 
{
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_get_fd_index(fdt, fd);
}

/* release fd in specific fdt */
void fdt_fd_release(struct efs_fdtable* fdt, int fd) 
{
    if(fd < 0 || fd >= (int)fdt->maxfd) 
    {
        printf("[efs.c] fdt_fd_release is failed! fd_num is illegal!\n");
        return ;
    }

    struct efs_file *fd_entry;
    efs_file_lock();
    fd_entry = fdt->fds[fd];
    if (fd_entry == NULL) 
    {
        efs_file_unlock();
        printf("[efs.c] fd entry is already NULL!\n");
        return ;
    }
    else 
    {
        fdt->fds[fd] = NULL;
        fd_entry->ref_count--;
        if (fd_entry -> ref_count == 0) 
        {
            struct efs_vnode *vnode = fd_entry->vnode;
            vnode->ref_count--;
            if (vnode->ref_count == 0) 
            {
                vPortFree(vnode);
            }
            fd_entry->vnode = NULL;
            vPortFree(fd_entry);
        }
        fdt->used--;
        efs_file_unlock();
        return ;
    }

}

/* release fd */
void fd_release(int fd) 
{
    struct efs_fdtable *fdt = efs_fdtable_get();
    return fdt_fd_release(fdt, fd);
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

struct efs_fdtable *efs_fdtable_get(void) 
{
    struct efs_fdtable *fdt = &_fdtab;
    return fdt;
}


/* init efs */
int efs_init(void) 
{
    static BaseType_t init_ready = pdFALSE;

    if (init_ready) 
    {
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
    if (xEfsMutex == NULL) 
    {
        printf("[efs.c] efs_init is failed! xEfsMutex create failed!\n");
        return -1;
    }
    /* init fd lock */
    xEfsFileMutex = xSemaphoreCreateMutex();
    if (xEfsFileMutex == NULL) 
    {
        printf("[efs.c] efs_init is failed! xEfsFileMutex create failed!\n");
        return -1;
    }
    init_ready = pdTRUE;
    printf("efs inited.\n");
    return 0;
}


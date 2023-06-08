#ifndef __EFS_UTIL_H__
#define __EFS_UTIL_H__

#include "headers.h"

typedef signed   char                   eris_int8_t;      /**<  8bit integer type */
typedef signed   short                  eris_int16_t;     /**< 16bit integer type */
typedef signed   int                    eris_int32_t;     /**< 32bit integer type */
typedef unsigned char                   eris_uint8_t;     /**<  8bit unsigned integer type */
typedef unsigned short                  eris_uint16_t;    /**< 16bit unsigned integer type */
typedef unsigned int                    eris_uint32_t;    /**< 32bit unsigned integer type */
//#endif

typedef int                             eris_bool_t;      /**< boolean type */
typedef signed long                     eris_base_t;      /**< Nbit CPU related date type */
typedef unsigned long                   eris_ubase_t;     /**< Nbit unsigned CPU related data type */


#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
    #define configHEAP_CLEAR_MEMORY_ON_FREE    0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* Max value that fits in a size_t type. */
#define heapSIZE_MAX              ( ~( ( size_t ) 0 ) )

/* Check if multiplying a and b will result in overflow. */
#define heapMULTIPLY_WILL_OVERFLOW( a, b )    ( ( ( a ) > 0 ) && ( ( b ) > ( heapSIZE_MAX / ( a ) ) ) )

/* Check if adding a and b will result in overflow. */
#define heapADD_WILL_OVERFLOW( a, b )         ( ( a ) > ( heapSIZE_MAX - ( b ) ) )

/* MSB of the xBlockSize member of an BlockLink_t structure is used to track
 * the allocation status of a block.  When MSB of the xBlockSize member of
 * an BlockLink_t structure is set then the block belongs to the application.
 * When the bit is free the block is still part of the free heap space. */
#define heapBLOCK_ALLOCATED_BITMASK    ( ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 ) )
#define heapBLOCK_SIZE_IS_VALID( xBlockSize )    ( ( ( xBlockSize ) & heapBLOCK_ALLOCATED_BITMASK ) == 0 )
#define heapBLOCK_IS_ALLOCATED( pxBlock )        ( ( ( pxBlock->xBlockSize ) & heapBLOCK_ALLOCATED_BITMASK ) != 0 )
#define heapALLOCATE_BLOCK( pxBlock )            ( ( pxBlock->xBlockSize ) |= heapBLOCK_ALLOCATED_BITMASK )
#define heapFREE_BLOCK( pxBlock )                ( ( pxBlock->xBlockSize ) &= ~heapBLOCK_ALLOCATED_BITMASK )


/* boolean type definitions */
#define Eris_TRUE                         1               /**< boolean true  */
#define Eris_FALSE                        0               /**< boolean fails */

/* null pointer definition */
#define Eris_NULL                         0


#define Eris_EOK                          0               /**< There is no error */
#define Eris_ERROR                        1               /**< A generic error happens */
#define Eris_ETIMEOUT                     2               /**< Timed out */
#define Eris_EFULL                        3               /**< The resource is full */
#define Eris_EEMPTY                       4               /**< The resource is empty */
#define Eris_ENOMEM                       5               /**< No memory */
#define Eris_ENOSYS                       6               /**< No system */
#define Eris_EBUSY                        7               /**< Busy */
#define Eris_EIO                          8               /**< IO error */
#define Eris_EINTR                        9               /**< Interrupted system call */
#define Eris_EINVAL                       10              /**< Invalid argument */
#define Eris_ETRAP                        11              /**< Trap event */
#define Eris_ENOENT                       12              /**< No entry */
#define Eris_ENOSPC                       13              /**< No space left */

/**
 * memory item on the heap
 */

// struct eris_memheap_item
// {
//     eris_uint32_t             magic;                      /**< magic number for memheap */
//     struct eris_memheap      *pool_ptr;                   /**< point of pool */
// 
//     struct eris_memheap_item *next;                       /**< next memheap item */
//     struct eris_memheap_item *prev;                       /**< prev memheap item */
// 
//     struct eris_memheap_item *next_free;                  /**< next free memheap item */
//     struct eris_memheap_item *prev_free;                  /**< prev free memheap item */
// #ifdef Eris_USING_MEMTRACE
//     eris_uint8_t              owner_thread_name[4];       /**< owner thread name */
// #endif /* Eris_USING_MEMTRACE */
// };
// 
// /**
//  * Base structure of memory heap object
//  */
// struct eris_memheap
// {
//     struct eris_object        parent;                     /**< inherit from eris_object */
// 
//     void                      *start_addr;                 /**< pool start address and size */
// 
//     eris_size_t               pool_size;                  /**< pool size */
//     eris_size_t               available_size;             /**< available size */
//     eris_size_t               max_used_size;              /**< maximum allocated size */
// 
//     struct eris_memheap_item *block_list;                 /**< used block list */
// 
//     struct eris_memheap_item *free_list;                  /**< free block list */
//     struct eris_memheap_item  free_header;                /**< free block list header */
// 
//     struct eris_semaphore     lock;                       /**< semaphore lock */
//     eris_bool_t               locked;                     /**< External lock mark */
// };

typedef struct A_BLOCK_LINK
{
    eris_uint32_t             magic;
    struct eris_memheap      *pool_ptr;

    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
    size_t xUsedBlockSize;

    #ifdef Eris_USING_MEMTRACE
        eris_uint8_t              owner_thread_name[4];       /**< owner thread name */
    #endif /* Eris_USING_MEMTRACE */
} BlockLink_t;

struct eris_memheap
{
    struct                    eris_object parent;

    void                      *start_addr;
/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
    eris_size_t               pool_size;                  /**< pool size */
    eris_size_t               xMinimumEverFreeBytesRemaining;
    eris_size_t               xFreeBytesRemaining;        /*available_size*/
    eris_size_t               max_used_size;              /*maximum allocated size*/

    eris_size_t               xNumberOfSuccessfulAllocations;
    eris_size_t               xNumberOfSuccessfulFrees;

/* Create a couple of list links to mark the start and end of the list. */
    BlockLink_t               xStart;
    BlockLink_t              *free_list;
    BlockLink_t              *pxEnd;
    
    SemaphoreHandle_t         lock;                       /**< semaphore lock */
    eris_bool_t               locked;                     /**< External lock mark */
};

#define portBYTE_ALIGNMENT 8
#if portBYTE_ALIGNMENT == 32
    #define portBYTE_ALIGNMENT_MASK    ( 0x001f )
#elif portBYTE_ALIGNMENT == 16
    #define portBYTE_ALIGNMENT_MASK    ( 0x000f )
#elif portBYTE_ALIGNMENT == 8
    #define portBYTE_ALIGNMENT_MASK    ( 0x0007 )
#elif portBYTE_ALIGNMENT == 4
    #define portBYTE_ALIGNMENT_MASK    ( 0x0003 )
#elif portBYTE_ALIGNMENT == 2
    #define portBYTE_ALIGNMENT_MASK    ( 0x0001 )
#elif portBYTE_ALIGNMENT == 1
    #define portBYTE_ALIGNMENT_MASK    ( 0x0000 )
#else /* if portBYTE_ALIGNMENT == 32 */
    #error "Invalid portBYTE_ALIGNMENT definition"
#endif /* if portBYTE_ALIGNMENT == 32 */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/**
 * Single List structure
 */
struct eris_slist_node
{
    struct eris_slist_node *next;                         /**< point to next node. */
};
typedef struct eris_slist_node eris_slist_t;                /**< Type for single list. */



#ifndef O_DIRECTORY
#define O_DIRECTORY 0x200000
#endif


#ifndef _INC_ERRNO
#define _INC_ERRNO
/*
    freertos中见FreeRTOS/Source/include/projdefs.h
    /Applications/ArmGNUToolchain/12.2.mpacbti-rel1/arm-none-eabi/arm-none-eabi/include/sys/errno.h already have these errnos
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
*/


#ifndef RC_INVOKED
#if !defined(_SECURECEris_ERRCODE_VALUES_DEFINED)
#define _SECURECEris_ERRCODE_VALUES_DEFINED
#define STRUNCATE 80
#endif
#endif

#endif

#define Eris_NAME_MAX 8
#define Eris_ALIGN_SIZE 8
#define Eris_THREAD_PRIORITY_32
#define Eris_THREAD_PRIORITY_MAX 32
#define Eris_TICK_PER_SECOND 100
#define Eris_USING_OVERFLOW_CHECK
#define Eris_USING_HOOK
#define Eris_HOOK_USING_FUNC_PTR
#define Eris_USING_IDLE_HOOK
#define Eris_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 256
#define Eris_USING_TIMER_SOFT
#define Eris_TIMER_THREAD_PRIO 4
#define Eris_TIMER_THREAD_STACK_SIZE 512

enum eris_object_class_type
{
    Eris_Object_Class_Null          = 0x00,      /**< The object is not used. */
    Eris_Object_Class_Thread        = 0x01,      /**< The object is a thread. */
    Eris_Object_Class_Semaphore     = 0x02,      /**< The object is a semaphore. */
    Eris_Object_Class_Mutex         = 0x03,      /**< The object is a mutex. */
    Eris_Object_Class_Event         = 0x04,      /**< The object is a event. */
    Eris_Object_Class_MailBox       = 0x05,      /**< The object is a mail box. */
    Eris_Object_Class_MessageQueue  = 0x06,      /**< The object is a message queue. */
    Eris_Object_Class_MemHeap       = 0x07,      /**< The object is a memory heap. */
    Eris_Object_Class_MemPool       = 0x08,      /**< The object is a memory pool. */
    Eris_Object_Class_Device        = 0x09,      /**< The object is a device. */
    Eris_Object_Class_Timer         = 0x0a,      /**< The object is a timer. */
    Eris_Object_Class_Module        = 0x0b,      /**< The object is a module. */
    Eris_Object_Class_Memory        = 0x0c,      /**< The object is a memory. */
    Eris_Object_Class_Channel       = 0x0d,      /**< The object is a channel */
    Eris_Object_Class_Custom        = 0x0e,      /**< The object is a custom object */
    Eris_Object_Class_Unknown       = 0x0f,      /**< The object is unknown. */
    Eris_Object_Class_Static        = 0x80       /**< The object is a static object. */
};


#define Eris_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup KernelService
 */

/**@{*/

/**
 * eris_container_of - return the start address of struct type, while ptr is the
 * member of struct type.
 */
#define eris_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))


/**
 * @brief initialize a list object
 */
#define Eris_LIST_OBJECT_INIT(object) { &(object), &(object) }


void eris_list_init(eris_list_t *l);

void eris_list_insert_after(eris_list_t *l, eris_list_t *n);

void eris_list_insert_before(eris_list_t *l, eris_list_t *n);

void eris_list_remove(eris_list_t *n);

int eris_list_isempty(const eris_list_t *l);

unsigned int eris_list_len(const eris_list_t *l);

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define eris_list_entry(node, type, member) \
    eris_container_of(node, type, member)

/**
 * eris_list_for_each - iterate over a list
 * @param pos the eris_list_t * to use as a loop cursor.
 * @param head the head for your list.
 */
#define eris_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * eris_list_for_each_safe - iterate over a list safe against removal of list entry
 * @param pos the eris_list_t * to use as a loop cursor.
 * @param n another eris_list_t * to use as temporary storage
 * @param head the head for your list.
 */
#define eris_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

/**
 * eris_list_for_each_entry  -   iterate over list of given type
 * @param pos the type * to use as a loop cursor.
 * @param head the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define eris_list_for_each_entry(pos, head, member) \
    for (pos = eris_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = eris_list_entry(pos->member.next, typeof(*pos), member))

/**
 * eris_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @param pos the type * to use as a loop cursor.
 * @param n another type * to use as temporary storage
 * @param head the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define eris_list_for_each_entry_safe(pos, n, head, member) \
    for (pos = eris_list_entry((head)->next, typeof(*pos), member), \
         n = eris_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = eris_list_entry(n->member.next, typeof(*n), member))

/**
 * eris_list_first_entry - get the first element from a list
 * @param ptr the list head to take the element from.
 * @param type the type of the struct this is embedded in.
 * @param member the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define eris_list_first_entry(ptr, type, member) \
    eris_list_entry((ptr)->next, type, member)

#define Eris_SLIST_OBJECT_INIT(object) { Eris_NULL }

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
void eris_slist_init(eris_slist_t *l);

void eris_slist_append(eris_slist_t *l, eris_slist_t *n);

void eris_slist_insert(eris_slist_t *l, eris_slist_t *n);

unsigned int eris_slist_len(const eris_slist_t *l);

eris_slist_t *eris_slist_remove(eris_slist_t *l, eris_slist_t *n);

eris_slist_t *eris_slist_first(eris_slist_t *l);

eris_slist_t *eris_slist_tail(eris_slist_t *l);

eris_slist_t *eris_slist_next(eris_slist_t *n);

int eris_slist_isempty(eris_slist_t *l);

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define eris_slist_entry(node, type, member) \
    eris_container_of(node, type, member)

/**
 * eris_slist_for_each - iterate over a single list
 * @param pos the eris_slist_t * to use as a loop cursor.
 * @param head the head for your single list.
 */
#define eris_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != Eris_NULL; pos = pos->next)

/**
 * eris_slist_for_each_entry  -   iterate over single list of given type
 * @param pos the type * to use as a loop cursor.
 * @param head the head for your single list.
 * @param member the name of the list_struct within the struct.
 */
#define eris_slist_for_each_entry(pos, head, member) \
    for (pos = eris_slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (Eris_NULL); \
         pos = eris_slist_entry(pos->member.next, typeof(*pos), member))

/**
 * eris_slist_first_entry - get the first element from a slist
 * @param ptr the slist head to take the element from.
 * @param type the type of the struct this is embedded in.
 * @param member the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define eris_slist_first_entry(ptr, type, member) \
    eris_slist_entry((ptr)->next, type, member)

/**
 * eris_slist_tail_entry - get the tail element from a slist
 * @param ptr the slist head to take the element from.
 * @param type the type of the struct this is embedded in.
 * @param member the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define eris_slist_tail_entry(ptr, type, member) \
    eris_slist_entry(eris_slist_tail(ptr), type, member)

/**@}*/

#ifdef __cplusplus
}
#endif

struct statfs
{
    size_t f_bsize;   /* block size */
    size_t f_blocks;  /* total data blocks in file system */
    size_t f_bfree;   /* free blocks in file system */
    size_t f_bavail;  /* free blocks available to unprivileged user*/
};
#define Eris_MEMHEAP_MAGIC        0x1ea01ea0
#define Eris_MEMHEAP_MASK         0xFFFFFFFE
#define Eris_MEMHEAP_USED         0x01
#define Eris_MEMHEAP_FREED        0x00

#define Eris_IPC_FLAG_FIFO                0x00            /**< FIFOed IPC. @ref IPC. */
#define Eris_IPC_FLAG_PRIO                0x01            /**< PRIOed IPC. @ref IPC. */
#define Eris_DEBUG_LOG(type, message) printf(message);

#ifndef Eris_DEBUG_MEMHEAP
#define Eris_DEBUG_MEMHEAP               0
#endif

#define Eris_ASSERT(EX)                                                         \
if (!(EX))                                                                    \
{                                                                             \
    exit(-1);                           \
}

#define __attribute__(...)
#define eris_weak                     __attribute__((weak))

#ifndef __DIRENT_H__
#define __DIRENT_H__


#ifdef __cplusplus
extern "C" {
#endif

/*
* dirent.h - format of directory entries
 * Ref: http://www.opengroup.org/onlinepubs/009695399/basedefs/dirent.h.html
 */

/* File types */
#define FT_REGULAR      0   /* regular file */
#define FT_SOCKET       1   /* socket file  */
#define FT_DIRECTORY    2   /* directory    */
#define FT_USER         3   /* user defined */

#define DT_UNKNOWN      0x00
#define DT_REG          0x01
#define DT_DIR          0x02

#ifndef HAVE_DIR_STRUCTURE
#define HAVE_DIR_STRUCTURE
typedef struct
{
    int fd;                         /* directory file */
    char buf[512];
    int num;
    int cur;
}DIR;
#endif

#ifndef HAVE_DIRENT_STRUCTURE
#define HAVE_DIRENT_STRUCTURE

#define DIRENT_NAME_MAX    256

struct dirent
{
    eris_uint8_t  d_type;             /* The type of the file */
    eris_uint8_t  d_namlen;           /* The length of the not including the terminating null file name */
    eris_uint16_t d_reclen;           /* length of this record */
    char d_name[DIRENT_NAME_MAX];   /* The null-terminated file name */
};
#endif

int            closedir(DIR *);
DIR           *opendir(const char *);
struct dirent *readdir(DIR *);
int            readdir_r(DIR *, struct dirent *, struct dirent **);
void           rewinddir(DIR *);
void           seekdir(DIR *, long int);
long           telldir(DIR *);

#ifdef __cplusplus
}
#endif

#endif


eris_int32_t eris_strcmp(const char *cs, const char *ct);

void *eris_memcpy(void *dst, const void *src, eris_ubase_t count);

eris_err_t eris_memheap_init(struct eris_memheap *memheap,
                         const char        *name,
                         void              *start_addr,
                         eris_size_t         size);

eris_weak void *eris_memset(void *s, int c, eris_ubase_t count);

void * pvPortMalloc_efs( size_t xWantedSize , struct eris_memheap * memheap);

void vPortFree_efs( void * pv , struct eris_memheap *memheap);

size_t xPortGetFreeHeapSize_efs( struct eris_memheap *memheap );

size_t xPortGetMinimumEverFreeHeapSize_efs( struct eris_memheap *memheap );

void * pvPortCalloc_efs( size_t xNum,
                     size_t xSize,
                     struct eris_memheap *memheap );

void prvInsertBlockIntoFreeList_efs( BlockLink_t * pxBlockToInsert, struct eris_memheap * memheap);

void *pvPortRealloc_efs(struct eris_memheap *memheap, void *ptr, eris_size_t newsize);

void *pvPortRealloc( void *pv, size_t xWantedSize );

char *strdup_efs(const char *s);

int strlen_efs(const char *s);

#endif
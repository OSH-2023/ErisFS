/* 本头文件记录了dfs中有关设备的一系列结构体
** efs_fs.c 部分未使用信号量，且由于RT-Thread与FreeRTOS的信号量存在差异
** 故并未修改信号量结构体
**
**
*/

#ifndef __EFS_DEVICE_H__
#define __EFS_DEVICE_H__


#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

typedef signed long                       eris_off_t;       /**< Type for offset */
typedef unsigned long                       eris_size_t;       /**< Type for offset */

typedef signed long                         eris_err_t;
typedef signed long                         eris_ssize_t;     /**< Used for a count of bytes or an error indication */


struct eris_list_node
{
    struct eris_list_node *next;                          /**< point to next node. */
    struct eris_list_node *prev;                          /**< point to prev node. */
};
typedef struct eris_list_node eris_list_t;  

/**
 * Base structure of Kernel object
 */
struct eris_object
{
#if eris_NAME_MAX > 0
    char        name[ERIS_NAME_MAX];                       /**< dynamic name of kernel object */
#else
    const char *name;                                    /**< static name of kernel object */
#endif /* ERIS_NAME_MAX > 0 */
    uint8_t  type;                                    /**< type of kernel object */
    uint8_t  flag;                                    /**< flag of kernel object */

    eris_list_t   list;                                    /**< list node of kernel object */
};
 

/**
 * device (I/O) class type设备类型
 */
enum eris_device_class_type
{
    eris_Device_Class_Block,                              /**< block device */
};

/**
 * Device structure
 */
typedef struct eris_device *eris_device_t;

struct eris_device
{
    struct eris_object          parent;                   /**< inherit from eris_object */
    enum eris_device_class_type type;                     /**< device type */
    uint16_t               flag;                     /**< device flag */
    uint16_t               open_flag;                /**< device open flag */

    uint8_t                ref_count;                /**< reference count */
    uint8_t                device_id;                /**< 0 - 255 */

    /* device call back */
    eris_err_t (*rx_indicate)(eris_device_t dev, eris_size_t size);
    eris_err_t (*tx_complete)(eris_device_t dev, void *buffer);

    /* common device interface 通用设备接口*/
    eris_err_t  (*init)   (eris_device_t dev);
    eris_err_t  (*open)   (eris_device_t dev, uint16_t oflag);
    eris_err_t  (*close)  (eris_device_t dev);
    eris_ssize_t (*read)  (eris_device_t dev, eris_off_t pos, void *buffer, eris_size_t size);
    eris_ssize_t (*write) (eris_device_t dev, eris_off_t pos, const void *buffer, eris_size_t size);
    eris_err_t  (*control)(eris_device_t dev, int cmd, void *args);

    void                     *user_data;                /**< device private data */
};


typedef signed long      off_t;

struct eris_semaphore
{
    uint16_t          value;                         /**< value of semaphore. */
    uint16_t          reserved;                      /**< reserved field */
};
typedef struct eris_semaphore *eris_sem_t;

typedef uint16_t                        uint16_t;    /**< 16bit unsigned integer type */
typedef uint8_t                        uint8_t;    /**< 16bit unsigned integer type */

/**
 * Base structure of IPC object
 */
struct eris_ipc_object
{
    struct eris_object parent;                            /**< inherit from eris_object */

    eris_list_t        suspend_thread;                    /**< threads pended on this resource */
};

typedef struct eris_object *eris_object_t;                   /**< Type for kernel objects. */

#endif
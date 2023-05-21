/* 本头文件记录了dfs中有关设备的一系列结构体
** efs_fs.c 部分未使用信号量，且由于RT-Thread与FreeRTOS的信号量存在差异
** 故并未修改信号量结构体
**
**
*/

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

typedef signed long                       eris_off_t;       /**< Type for offset */
typedef unsigned long                       eris_size_t;       /**< Type for offset */

typedef signed long                         eris_err_t;
typedef signed long                         eris_ssize_t;     /**< Used for a count of bytes or an error indication */

/**
 * device (I/O) class type设备类型
 */
enum eris_device_class_type
{
    eris_Device_Class_Char = 0,                           /**< character device */
    eris_Device_Class_Block,                              /**< block device */
    eris_Device_Class_NetIf,                              /**< net interface */
    eris_Device_Class_MTD,                                /**< memory device */
    eris_Device_Class_CAN,                                /**< CAN device */
    eris_Device_Class_erisC,                                /**< erisC device */
    eris_Device_Class_Sound,                              /**< Sound device */
    eris_Device_Class_Graphic,                            /**< Graphic device */
    eris_Device_Class_I2CBUS,                             /**< I2C bus device */
    eris_Device_Class_USBDevice,                          /**< USB slave device */
    eris_Device_Class_USBHost,                            /**< USB host bus */
    eris_Device_Class_USBOTG,                             /**< USB OTG bus */
    eris_Device_Class_SPIBUS,                             /**< SPI bus device */
    eris_Device_Class_SPIDevice,                          /**< SPI device */
    eris_Device_Class_SDIO,                               /**< SDIO bus device */
    eris_Device_Class_PM,                                 /**< PM pseudo device */
    eris_Device_Class_Pipe,                               /**< Pipe device */
    eris_Device_Class_Portal,                             /**< Portal device */
    eris_Device_Class_Timer,                              /**< Timer device */
    eris_Device_Class_Miscellaneous,                      /**< Miscellaneous device */
    eris_Device_Class_Sensor,                             /**< Sensor device */
    eris_Device_Class_Touch,                              /**< Touch device */
    eris_Device_Class_PHY,                                /**< PHY device */
    eris_Device_Class_Security,                           /**< Security device */
    eris_Device_Class_WLAN,                               /**< WLAN device */
    eris_Device_Class_Pin,                                /**< Pin device */
    eris_Device_Class_ADC,                                /**< ADC device */
    eris_Device_Class_DAC,                                /**< DAC device */
    eris_Device_Class_WDT,                                /**< WDT device */
    eris_Device_Class_PWM,                                /**< PWM device */
    eris_Device_Class_Bus,                                /**< Bus device */
    eris_Device_Class_Unknown                             /**< unknown device */
};

/**
 * Device structure
 */
struct eris_device
{
    struct eris_object          parent;                   /**< inherit from eris_object */
    enum eris_device_class_type type;                     /**< device type */
    eris_uint16_t               flag;                     /**< device flag */
    eris_uint16_t               open_flag;                /**< device open flag */

    eris_uint8_t                ref_count;                /**< reference count */
    eris_uint8_t                device_id;                /**< 0 - 255 */

    /* device call back */
    eris_err_t (*rx_indicate)(eris_device_t dev, eris_size_t size);
    eris_err_t (*tx_complete)(eris_device_t dev, void *buffer);

    /* common device interface 通用设备接口*/
    eris_err_t  (*init)   (eris_device_t dev);
    eris_err_t  (*open)   (eris_device_t dev, eris_uint16_t oflag);
    eris_err_t  (*close)  (eris_device_t dev);
    eris_ssize_t (*read)  (eris_device_t dev, eris_off_t pos, void *buffer, eris_size_t size);
    eris_ssize_t (*write) (eris_device_t dev, eris_off_t pos, const void *buffer, eris_size_t size);
    eris_err_t  (*control)(eris_device_t dev, int cmd, void *args);

    void                     *user_data;                /**< device private data */
};
typedef struct eris_device *eris_device_t;

typedef signed long      off_t;

struct eris_semaphore
{
    struct eris_ipc_object parent;                        /**< inherit from ipc_object */

    eris_uint16_t          value;                         /**< value of semaphore. */
    eris_uint16_t          reserved;                      /**< reserved field */
};
typedef struct eris_semaphore *eris_sem_t;

typedef uint16_t                        eris_uint16_t;    /**< 16bit unsigned integer type */
typedef uint8_t                        eris_uint8_t;    /**< 16bit unsigned integer type */

/**
 * Base structure of IPC object
 */
struct eris_ipc_object
{
    struct eris_object parent;                            /**< inherit from eris_object */

    eris_list_t        suspend_thread;                    /**< threads pended on this resource */
};


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
    eris_uint8_t  type;                                    /**< type of kernel object */
    eris_uint8_t  flag;                                    /**< flag of kernel object */

    eris_list_t   list;                                    /**< list node of kernel object */
};
typedef struct eris_object *eris_object_t;                   /**< Type for kernel objects. */
struct eris_list_node
{
    struct eris_list_node *next;                          /**< point to next node. */
    struct eris_list_node *prev;                          /**< point to prev node. */
};
typedef struct eris_list_node eris_list_t;   
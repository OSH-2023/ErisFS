#include "headers.h"

#define device_init     (dev->init)
#define device_open     (dev->open)
#define device_close    (dev->close)
#define device_read     (dev->read)
#define device_write    (dev->write)

eris_device_t eris_device_find(const char *name){
    if (eris_strcmp(name, "Apollo")) return NULL;
    else return NULL;
}

eris_err_t eris_device_open (eris_device_t dev, uint16_t oflag){
    eris_err_t result = ERIS_EOK;

    /* parameter check */
    if (dev == NULL) {
        printf("[efs_device.c] no this device\n");
        return -1;
    }

    /* if device is not initialized, initialize it. */
    if (!(dev->flag & ERIS_DEVICE_FLAG_ACTIVATED))
    {
        if (device_init != NULL)
        {
            result = device_init(dev);
            if (result != ERIS_EOK)
            {
                printf("To initialize device:%s failed. The error code is %d",
                        dev->parent.name, result);

                return result;
            }
        }

        dev->flag |= ERIS_DEVICE_FLAG_ACTIVATED;
    }

    /* device is a stand alone device and opened */
    if ((dev->flag & ERIS_DEVICE_FLAG_STANDALONE) &&
        (dev->open_flag & ERIS_DEVICE_OFLAG_OPEN))
    {
        return -ERIS_EBUSY;
    }

    /* device is not opened or opened by other oflag, call device_open interface */
    if (!(dev->open_flag & ERIS_DEVICE_OFLAG_OPEN) ||
        ((dev->open_flag & ERIS_DEVICE_OFLAG_MASK) != (oflag & ERIS_DEVICE_OFLAG_MASK)))
    {
        if (device_open != NULL)
        {
            result = device_open(dev, oflag);
        }
        else
        {
            /* set open flag */
            dev->open_flag = (oflag & ERIS_DEVICE_OFLAG_MASK);
        }
    }

    /* set open flag */
    if (result == ERIS_EOK || result == -ERIS_ENOSYS)
    {
        dev->open_flag |= ERIS_DEVICE_OFLAG_OPEN;

        dev->ref_count++;
        /* don't let bad things happen silently. If you are bitten by this asseeris,
        * please set the ref_count to a bigger type. */
        if(dev->ref_count == 0) {
            printf("[efs_device.c] reference failed\n");
            return -1;
        }
    }

    return result;
}

eris_err_t eris_device_close(eris_device_t dev){
    eris_err_t result = ERIS_EOK;

    /* parameter check */
        if (dev == NULL) {
        printf("[efs_device.c] no this device\n");
        return -1;
    }

    if (dev->ref_count == 0)
        return -ERIS_ERROR;

    dev->ref_count--;

    if (dev->ref_count != 0)
        return ERIS_EOK;

    /* call device_close interface */
    if (device_close != NULL)
    {
        result = device_close(dev);
    }

    /* set open flag */
    if (result == ERIS_EOK || result == -ERIS_ENOSYS)
        dev->open_flag = ERIS_DEVICE_OFLAG_CLOSE;

    return result;
}

eris_ssize_t eris_device_read(eris_device_t dev,
                            eris_off_t    pos,
                            void       *buffer,
                            eris_size_t   size)
{
    /* parameter check */
    if(dev == NULL) {
        printf("[efs_device.c] no this dev");
        return 0;
    }
    if (dev->ref_count == 0)
    {
        printf("[efs_device.c] device hasn't been referenced");
        return 0;
    }

    /* call device_read interface */
    if (device_read != NULL)
    {
        return device_read(dev, pos, buffer, size);
    }

    /* set error code */
    printf("[efs_device.c] need read function of device");

    return 0;
}

eris_ssize_t eris_device_write(eris_device_t dev,
                            eris_off_t    pos,
                            const void *buffer,
                            eris_size_t   size)
{
    /* parameter check */
    if(dev == NULL) {
        printf("[efs_device.c] no this dev");
        return 0;
    }
    if (dev->ref_count == 0)
    {
        printf("[efs_device.c] device hasn't been referenced");
        return 0;
    }

    /* call device_write interface */
    if (device_write != NULL)
    {
        return device_write(dev, pos, buffer, size);
    }

    /* set error code */
    printf("[efs_device.c] need write function of device");

    return 0;
}
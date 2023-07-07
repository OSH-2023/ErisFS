/*
 * @Description: 
 * @Version: 
 * @Author: Tyrion Huu
 * @Date: 2023-06-09 08:16:17
 * @LastEditors: Tyrion Huu
 * @LastEditTime: 2023-07-07 16:02:07
 */
#ifndef PLUS_FAT_H
    #define PLUS_FAT_H

    #include <stdio.h>
    #include <string.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <fcntl.h>
    #include <errno.h>

    #include <sys/stat.h>
    #include <sys/time.h>

    #include "FreeRTOS.h"
    #include "semphr.h"
    #include "task.h"
    #include "list.h"
    #include "queue.h"

    #include "efs.h"
    #include "efs_device.h"
    #include "efs_util.h"
    #include "efs_fs.h"
    #include "efs_posix.h"
    #include "efs_private.h"
    #include "efs_file.h" 
    #include "efs_aes.h"

    #include "efs_ramfs.h"
#endif /* ifndef PLUS_FAT_H */
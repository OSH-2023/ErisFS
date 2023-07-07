#include <headers.h>
/* results:
 *  -1, no space to install fatfs driver
 *  >= 0, there is an space to install fatfs driver
 */
static int elm_result_to_efs(FRESULT result)
{
    int status = 0;

    switch (result)
    {
    case FR_OK:
        break;

    case FR_NO_FILE:
    case FR_NO_PATH:
    case FR_NO_FILESYSTEM:
        status = -2;
        break;

    case FR_INVALID_NAME:
        status = -3;
        break;

    case FR_EXIST:
    case FR_INVALID_OBJECT:
        status = -4;
        break;

    case FR_DISK_ERR:
    case FR_NOT_READY:
    case FR_INT_ERR:
        status = -5;
        break;

    case FR_WRITE_PROTECTED:
    case FR_DENIED:
        status = -6;
        break;

    case FR_MKFS_ABORTED:
        status = -7;
        break;

    default:
        status = -1;
        break;
    }

    return status;
}
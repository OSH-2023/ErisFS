#ifndef __EFS_PRIVATE_H__
#define __EFS_PRIVATE_H__
#include "headers.h"

extern struct efs_filesystem_ops *filesystem_operation_table[];

extern struct efs_filesystem filesystem_table[];

extern const struct efs_mount_tbl mount_table[];

#endif
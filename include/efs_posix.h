/*
 * @Description: 
 * @Version: 
 * @Author: Tyrion Huu
 * @Date: 2023-07-03 20:04:31
 * @LastEditors: Tyrion Huu
 * @LastEditTime: 2023-07-04 11:43:20
 */
#ifndef __EFS_POSIX_H__
#define __EFS_POSIX_H__


int efs_open(const char *file, int flags, ...);

int close(int fd);

typedef int ssize_t;

ssize_t read(int fd, void *buf, size_t len);

ssize_t write(int fd, const void *buf, size_t len);

int fstat_(int fd, struct stat *buf);


#endif
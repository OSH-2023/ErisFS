# ErisFS
A comprehensive virtual filesystem for FreeRTOS, build by x-Eris group.

## Sturcture
```
ErisFS
├── include
│   ├── efs_posix.h
│   ├── efs.h
│   ├── efs_file.h
│   ├── efs_fs.h
│   ├── efs_posix.h
│   ├── efs_utils.h
│ 
├── efs
│   ├── efs_posix.c
│   ├── efs.c
│   ├── efs_file.c
│   ├── efs_fs.c
│   ├── efs_posix.c
│   ├── efs_es.c
│   └── efs_utils.c
│ 
└── filesystem
    ├─── fatfs
    └─── ramfs
```

## Usage
Please check the template file in ```replace```, which shows some basic usage of ErisFS (almost same with POSIX API).

Also, you can check ```include/efs_posix.h``` and ```include/efs_fs.h``` to find every functions' declaration.

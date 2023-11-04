////////////////////////////////////////////////////////////////////////////////////////
//
//      2021 Georgi Angelov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////////////

#ifndef _VFS_H_
#define _VFS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <wizio.h>
#include <vfs_config.h>

#ifdef USE_LFS
#include <lfs.h>
#endif

#ifdef USE_FATFS
#include <ff.h>
#endif

#define FILES_FD_BASE 3

    struct vfs_s;
    struct vfs_file_s;

    typedef struct vfs_oper
    {
        int (*init)(struct vfs_s *);
        int (*mount)(struct vfs_s *);
        int (*unmount)(struct vfs_s *);

        /* FILE */
        void *(*open)(struct vfs_s *, const char *, int, int);
        int (*close)(struct vfs_file_s *);
        size_t (*write)(struct vfs_file_s *, const char *, size_t);
        size_t (*read)(struct vfs_file_s *, char *, size_t);
        _off_t (*seek)(struct vfs_file_s *, _off_t, int);

        /* DIR */
        int (*mkdir)(struct vfs_file_s *, const char *, mode_t);
    } vfs_oper;

    typedef struct vfs_s
    {
        void *ctx;    // file_system_context -> vfs_oper *op;
        char name[3]; // "A:"
    } vfs_t;          // list

    typedef struct vfs_file_s
    {
        void *file;
        vfs_t *fs;         // link to file_system_context
        int fd;            // real
        unsigned int hash; // path
    } vfs_file_t;          // array of open files

    int vfs_init(void);
    int vfs_mount(const char *path, const void *file_system_context);
    int vfs_unmount(const char *path);
    int vfs_open(const char *path, int flags, int mode);
    int vfs_close(int fd);
    size_t vfs_write(int fd, const char *buf, size_t size);
    size_t vfs_read(int fd, char *buf, size_t size);
    _off_t vfs_seek(int fd, _off_t where, int whence);

    extern unsigned int strhash(const void *p);

#define PRE_INIT_FUNC(F) static __attribute__((section(".preinit_array"))) void (*__##F)(void) = F

#ifdef __cplusplus
}
#endif
#endif // _VFS_H_
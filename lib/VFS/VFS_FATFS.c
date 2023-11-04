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

#include "VFS.h"
#ifdef USE_FATFS
#include "VFS_FATFS.h"

#pragma message "FATFS is not tested with SD card..."

#define FFS_LOG(ERR, TXT)
//printf("[FFS] %s( %d ) %s\n", __func__, (int)ERR, (char *)TXT);

#define FFS_ERR(ERR, TXT)
//printf("[ERROR] at line %d, %s( %d ) %s\n", __LINE__, __func__, (int)ERR, (char *)TXT);

typedef struct fatfs_context_s
{
    vfs_oper *op; // must be first
    FATFS *fs;
    struct fatfs_config *cfg;
    void *pMutex;
} fatfs_context_t;

#define FATFS_FS_CTX ((fatfs_context_t *)Fs->ctx)
#define FATFS_FS_FFS (FATFS *)FATFS_FS_CTX->fs
#define FATFS_FS_MUTEX FATFS_FS_CTX->pMutex

#define FATFS_CTX ((fatfs_context_t *)File->fs->ctx)
#define FATFS_FFS ((FATFS *)FATFS_CTX->lfs)
#define FATFS_FILE ((FIL *)File->file)
#define FATFS_MUTEX FATFS_CTX->pMutex

static int ffs_toerror(FRESULT fr)
{
    switch (fr)
    {
    case FR_DISK_ERR:
        return -EIO;
    case FR_INT_ERR:
        return -EIO;
    case FR_NOT_READY:
        return -ENODEV;
    case FR_NO_FILE:
        return -ENOENT;
    case FR_NO_PATH:
        return -ENOENT;
    case FR_INVALID_NAME:
        return -EINVAL;
    case FR_DENIED:
        return -EACCES;
    case FR_EXIST:
        return -EEXIST;
    case FR_INVALID_OBJECT:
        return -EBADF;
    case FR_WRITE_PROTECTED:
        return -EACCES;
    case FR_INVALID_DRIVE:
        return -ENXIO;
    case FR_NOT_ENABLED:
        return -ENODEV;
    case FR_NO_FILESYSTEM:
        return -ENODEV;
    case FR_MKFS_ABORTED:
        return -EINTR;
    case FR_TIMEOUT:
        return -ETIMEDOUT;
    case FR_LOCKED:
        return -EACCES;
    case FR_NOT_ENOUGH_CORE:
        return -ENOMEM;
    case FR_TOO_MANY_OPEN_FILES:
        return -ENFILE;
    case FR_INVALID_PARAMETER:
        return -EINVAL;
    case FR_OK:
        return 0;
    }
    //assert(0 && "unhandled FRESULT");
    return -ENOTSUP;
}

static int ffs_tomode(int m)
{
    int res = 0;
    int acc_mode = m & O_ACCMODE;
    if (acc_mode == O_RDONLY)
    {
        res |= FA_READ;
    }
    else if (acc_mode == O_WRONLY)
    {
        res |= FA_WRITE;
    }
    else if (acc_mode == O_RDWR)
    {
        res |= FA_READ | FA_WRITE;
    }
    if ((m & O_CREAT) && (m & O_EXCL))
    {
        res |= FA_CREATE_NEW;
    }
    else if ((m & O_CREAT) && (m & O_TRUNC))
    {
        res |= FA_CREATE_ALWAYS;
    }
    else if (m & O_APPEND)
    {
        res |= FA_OPEN_ALWAYS;
    }
    else
    {
        res |= FA_OPEN_EXISTING;
    }
    return res;
}

static int s_fatfs_mount(vfs_t *Fs)
{
    //FFS_LOG(0," ");
    MUTEX_LOCK(FATFS_FS_MUTEX);
    FRESULT err = f_mount(FATFS_FS_FFS, Fs->name, 0);
    MUTEX_UNLOCK(FATFS_FS_MUTEX);
    return ffs_toerror(err);
}

static int s_fatfs_unmount(vfs_t *Fs)
{
    //FFS_LOG(0, " ");
    MUTEX_LOCK(FATFS_FS_MUTEX);
    FRESULT err = f_unmount(Fs->name);
    MUTEX_UNLOCK(FATFS_FS_MUTEX);
    return ffs_toerror(err);
}

static void *s_fatfs_open(vfs_t *Fs, const char *path, int flags, int mode)
{
    FFS_LOG(0, " ");
    FIL *file = (FIL *)calloc(1, sizeof(FIL));
    if (file)
    {
        MUTEX_LOCK(FATFS_FS_MUTEX);
        if ((errno = ffs_toerror(f_open(file, path, ffs_tomode(flags)))))
        {
            FFS_ERR(errno, path);
            free(file);
            file = NULL;
        } // else errno = 0;
        MUTEX_UNLOCK(FATFS_FS_MUTEX);
    }
    else
    {
        FFS_ERR(errno, "no mem");
        errno = ENOMEM;
    }
    return file;
}

static int s_fatfs_close(vfs_file_t *File)
{
    FFS_LOG(0, " ");
    MUTEX_LOCK(FATFS_MUTEX);
    FRESULT err = f_close(FATFS_FILE);
    free(FATFS_FILE);
    MUTEX_UNLOCK(FATFS_MUTEX);
    return ffs_toerror(err);
}

static size_t s_fatfs_write(vfs_file_t *File, const char *buf, size_t size)
{
    FFS_LOG(0, " ");
    MUTEX_LOCK(FATFS_MUTEX);
    unsigned wr = 0;
    FRESULT err = f_write(FATFS_FILE, buf, size, &wr);
    MUTEX_UNLOCK(FATFS_MUTEX);
    if (wr)
        return wr;
    return ffs_toerror(err);
}

static size_t s_fatfs_read(vfs_file_t *File, char *buf, size_t size)
{
    FFS_LOG(0, " ");
    MUTEX_LOCK(FATFS_MUTEX);
    unsigned rd = 0;
    FRESULT err = f_read(FATFS_FILE, buf, size, &rd);
    MUTEX_UNLOCK(FATFS_MUTEX);
    if (rd)
        return rd;
    return ffs_toerror(err);
}

static _off_t s_fatfs_seek(vfs_file_t *File, _off_t offset, int mode)
{
    FFS_LOG(0, " ");
    off_t new_pos;
    if (mode == SEEK_SET)
    {
        new_pos = offset;
    }
    else if (mode == SEEK_CUR)
    {
        off_t cur_pos = f_tell(FATFS_FILE);
        new_pos = cur_pos + offset;
    }
    else if (mode == SEEK_END)
    {
        off_t size = f_size(FATFS_FILE);
        new_pos = size + offset;
    }
    else
    {
        return -1;
    }
    MUTEX_LOCK(FATFS_MUTEX);
    FRESULT err = f_lseek(FATFS_FILE, new_pos);
    MUTEX_UNLOCK(FATFS_MUTEX);
    if (err != FR_OK)
        return ffs_toerror(err);
    return new_pos;
}

vfs_oper fatfs_oper = {
    .mount = s_fatfs_mount,
    .unmount = s_fatfs_unmount,
    .open = s_fatfs_open,
    .close = s_fatfs_close,
    .write = s_fatfs_write,
    .read = s_fatfs_read,
    .seek = s_fatfs_seek,
    //.mkdir = s_fatfs_mkdir,
};

static FATFS fatfs;
static fatfs_context_t fatfs_ctx = {
    .op = &fatfs_oper,
    .fs = &fatfs,
    .pMutex = NULL,
};

#endif // USE_FATFS

int fatfs_init(void)
{
    int err = -1;
#ifdef USE_FATFS
    MUTEX_INIT(fatfs_ctx.pMutex);
    err = vfs_mount(FATFS_LETTER, &fatfs_ctx);
#endif
    return err;
}
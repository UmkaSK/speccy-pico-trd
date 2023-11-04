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

#include "VFS_LFS.h"

#ifdef USE_LFS

#define LFS_PRINTF

typedef struct lfs_context_s
{
    vfs_oper *op; // must be first
    lfs_t *lfs;
    struct lfs_config *cfg;
    void *pMutex;
} lfs_context_t;

#define LFS_FS_CTX ((lfs_context_t *)Fs->ctx)
#define LFS_FS_LFS (lfs_t *)LFS_FS_CTX->lfs
#define LFS_FS_CFG ((struct lfs_config *)LFS_FS_CTX->cfg)
#define LFS_FS_MUTEX LFS_FS_CTX->pMutex

#define LFS_CTX ((lfs_context_t *)File->fs->ctx)
#define LFS_LFS ((lfs_t *)LFS_CTX->lfs)
#define LFS_FILE ((lfs_file_t *)File->file)
#define LFS_MUTEX LFS_CTX->pMutex

static int lfs_tomode(int m)
{
    int flags = 0;
    if ((m & 3) == O_RDONLY)
        flags |= LFS_O_RDONLY;
    if ((m & 3) == O_WRONLY)
        flags |= LFS_O_WRONLY;
    if ((m & 3) == O_RDWR)
        flags |= LFS_O_RDWR;
    if (m & O_CREAT)
        flags |= LFS_O_CREAT;
    if (m & O_EXCL)
        flags |= LFS_O_EXCL;
    if (m & O_TRUNC)
        flags |= LFS_O_TRUNC;
    if (m & O_APPEND)
        flags |= LFS_O_APPEND;
    return flags;
}

static int lfs_toerror(int err)
{
    switch (err)
    {
    case LFS_ERR_OK:
        return 0;
    case LFS_ERR_IO:
        return -EIO;
    case LFS_ERR_NOENT:
        return -ENOENT;
    case LFS_ERR_EXIST:
        return -EEXIST;
    case LFS_ERR_NOTDIR:
        return -ENOTDIR;
    case LFS_ERR_ISDIR:
        return -EISDIR;
    case LFS_ERR_INVAL:
        return -EINVAL;
    case LFS_ERR_NOSPC:
        return -ENOSPC;
    case LFS_ERR_NOMEM:
        return -ENOMEM;
    case LFS_ERR_CORRUPT:
        return -EILSEQ;
    default:
        return err;
    }
}

static int s_lfs_mount(vfs_t *Fs)
{
    LFS_PRINTF("[LFS] %s( + ) [%s] \n", __func__, Fs->name);
    int err;
    MUTEX_LOCK(LFS_FS_MUTEX);

#ifdef LFS_ROM_PRE_FORMAT
#warning LFS_ROM_PRE_FORMAT
    lfs_format(LFS_FS_LFS, LFS_FS_CFG); // for flash test
#endif

    if ((err = lfs_mount(LFS_FS_LFS, LFS_FS_CFG)))
    {
        LFS_PRINTF("[ERROR] lfs_mount() [%s] res = %d\n", Fs->name, err);
        if (0 == (err = lfs_format(LFS_FS_LFS, LFS_FS_CFG)))
        {
            LFS_PRINTF("[ERROR] lfs_format() [%s] res = %d\n", Fs->name, err);
            err = lfs_mount(LFS_FS_LFS, LFS_FS_CFG);
        }
    }

    MUTEX_UNLOCK(LFS_FS_MUTEX);
    LFS_PRINTF("[VFS] %s( - ) [%s] res = %d\n", __func__, Fs->name, err);
    return lfs_toerror(err);
}

static int s_lfs_unmount(vfs_t *fs)
{
    LFS_PRINTF("    [ %s ] fs_mount()\n", fs->name);
    return 0;
}

static void *s_lfs_open(vfs_t *Fs, const char *path, int flags, int mode)
{
    lfs_file_t *file = (lfs_file_t *)calloc(1, sizeof(lfs_file_t));
    if (file)
    {
        MUTEX_LOCK(LFS_FS_MUTEX);
        if ((errno = lfs_toerror(lfs_file_open(LFS_FS_LFS, file, path + 3, lfs_tomode(flags)))))
        {
            LFS_PRINTF("[ERROR] %s( %d ) '%s'\n", __func__, errno, path + 3);
            free(file);
            file = NULL;
        }
        else
            errno = 0;
        MUTEX_UNLOCK(LFS_FS_MUTEX);
    }
    else
    {
        LFS_PRINTF("[ERROR] %s( no mem )\n", __func__);
        errno = ENOMEM;
    }
    return file;
}

static int s_lfs_close(vfs_file_t *File)
{
    MUTEX_LOCK(LFS_MUTEX);
    int err = lfs_toerror(lfs_file_close(LFS_LFS, LFS_FILE));
    free(LFS_FILE);
    MUTEX_UNLOCK(LFS_MUTEX);
    return err;
}

static size_t s_lfs_write(vfs_file_t *File, const char *buf, size_t size)
{
    MUTEX_LOCK(LFS_MUTEX);
    int err = lfs_toerror(lfs_file_write(LFS_LFS, LFS_FILE, buf, size));
    MUTEX_UNLOCK(LFS_MUTEX);
    return err;
}

static size_t s_lfs_read(vfs_file_t *File, char *buf, size_t size)
{
    MUTEX_LOCK(LFS_MUTEX);
    int err = lfs_toerror(lfs_file_read(LFS_LFS, LFS_FILE, buf, size));
    MUTEX_UNLOCK(LFS_MUTEX);
    return err;
}

static _off_t s_lfs_seek(vfs_file_t *File, _off_t where, int whence)
{
    MUTEX_LOCK(LFS_MUTEX);
    int err = lfs_toerror(lfs_file_seek(LFS_LFS, LFS_FILE, where, whence));
    MUTEX_UNLOCK(LFS_MUTEX);
    return err;
}

static int s_lfs_mkdir(vfs_file_t *File, const char *name, mode_t mode)
{
    MUTEX_LOCK(LFS_MUTEX);
    int err = lfs_toerror(lfs_mkdir(LFS_LFS, name));
    MUTEX_UNLOCK(LFS_MUTEX);
    return err;
}

vfs_oper lfs_oper = {
    //.init = NULL,
    .mount = s_lfs_mount,
    .unmount = s_lfs_unmount,
    /* FILE */
    .open = s_lfs_open,
    .close = s_lfs_close,
    .write = s_lfs_write,
    .read = s_lfs_read,
    .seek = s_lfs_seek,
    /* DIR */
    .mkdir = s_lfs_mkdir
    //
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_LFS_RAM

/*

    RAM Device IO

*/

static uint8_t *lfs_ram_memory[LFS_RAM_BLOCK_SIZE * LFS_RAM_BLOCK_COUNT] __attribute__((aligned(16)));

static int lfs_ram_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    memcpy(buffer, lfs_ram_memory + (c->block_size * block) + off, size);
    return LFS_ERR_OK;
}

static int lfs_ram_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    memcpy(lfs_ram_memory + (c->block_size * block) + off, buffer, size);
    return LFS_ERR_OK;
}

static int lfs_ram_erase(const struct lfs_config *c, lfs_block_t block)
{
    memset(lfs_ram_memory + (c->block_size * block), 0, c->block_size);
    return LFS_ERR_OK;
}

static int lfs_ram_sync(const struct lfs_config *c) { return LFS_ERR_OK; }

/*

    RAM Device Context

*/

static lfs_t lfs_ram;
static struct lfs_context_s lfs_ram_ctx;
static const struct lfs_config lfs_ram_cfg = {
    .context = &lfs_ram_ctx,
    .read = lfs_ram_read,
    .prog = lfs_ram_write,
    .erase = lfs_ram_erase,
    .sync = lfs_ram_sync,
    .read_size = LFS_RAM_READ_SIZE,
    .prog_size = LFS_RAM_PROG_SIZE,
    .block_size = LFS_RAM_BLOCK_SIZE,
    .block_count = LFS_RAM_BLOCK_COUNT,
    .cache_size = LFS_RAM_CACHE_SIZE,
    .lookahead_size = LFS_RAM_LOOKAHEAD_SIZE,
    .block_cycles = LFS_RAM_BLOCK_CYCLES,
};
static lfs_context_t lfs_ram_ctx = {
    .op = &lfs_oper,
    .lfs = &lfs_ram,
    .cfg = (struct lfs_config *)&lfs_ram_cfg,
    .pMutex = NULL,
};

#pragma GCC push_options
#pragma GCC optimize("-O0")
static void lfs_ram_pre_init(void)
{
    MUTEX_INIT(lfs_ram_ctx.pMutex);
}
PRE_INIT_FUNC(lfs_ram_pre_init);
#pragma GCC pop_options

#endif // USE_LFS_RAM_FS /////////////////////////////////////////////////////////////////////////////

#ifdef USE_LFS_ROM

/*

    Internal Flash Device IO

*/

extern char __flash_binary_end;

static inline uint32_t lfs_rom_memory(void)
{
    return ((uint32_t)&__flash_binary_end + FLASH_SECTOR_SIZE - 1) & ~(FLASH_SECTOR_SIZE - 1);
}

static int lfs_rom_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    memcpy(buffer, (const void *)lfs_rom_memory() + (c->block_size * block) + off, size);
    return LFS_ERR_OK;
}

static int lfs_rom_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    if (c->block_size != FLASH_SECTOR_SIZE)
        return LFS_ERR_CORRUPT;
    uint8_t *buf = (uint8_t *)buffer;
    uint32_t flash_offs = lfs_rom_memory() + (c->block_size * block) - XIP_BASE;
    LFS_PRINTF("[WRITE] ADDR = %08X\n", (int)flash_offs);
    uint32_t saved_irq = save_and_disable_interrupts();
    {
        if (off) // ?
        {
            uint32_t len = c->block_size - off;
            flash_range_program(flash_offs + off, buf, len);
            flash_offs += len;
            buf += len;
            size -= len;
        }
        while ((int)size > 0)
        {
            LFS_PRINTF("[WRITE] ADDR = %08X  size = %d\n", (int)flash_offs, (int)size);
            flash_range_program(flash_offs, buf, size);
            flash_offs += c->block_size;
            buf += c->block_size;
            size -= c->block_size;
        }
    }
    restore_interrupts(saved_irq);
    LFS_PRINTF("[WRITE] OK\n");
    return LFS_ERR_OK;
}

static int lfs_rom_erase(const struct lfs_config *c, lfs_block_t block)
{
    if (c->block_size != FLASH_SECTOR_SIZE)
        return LFS_ERR_CORRUPT;
    uint32_t flash_offs = lfs_rom_memory() + (c->block_size * block) - XIP_BASE;
    LFS_PRINTF("[ERASE] PAGE ADDR = 0x%08X  size = %d\n", (int)flash_offs, (int)c->block_size);
    uint32_t saved_irq = save_and_disable_interrupts();
    {
        flash_range_erase(flash_offs, c->block_size /*4096*/);
    }
    restore_interrupts(saved_irq);
    LFS_PRINTF("[ERASE] OK\n");
    return LFS_ERR_OK; // PICO_FLASH_SIZE_BYTES = 200000
}

static int lfs_rom_sync(const struct lfs_config *c) { return LFS_ERR_OK; }

/*

    Internal Flash Device Context

*/

static lfs_t lfs_rom;
static struct lfs_context_s lfs_rom_ctx;
static const struct lfs_config lfs_rom_cfg = {
    .context = &lfs_rom_ctx,
    .read = lfs_rom_read,
    .prog = lfs_rom_write,
    .erase = lfs_rom_erase,
    .sync = lfs_rom_sync,
    .read_size = LFS_ROM_READ_SIZE,
    .prog_size = LFS_ROM_PROG_SIZE,
    .block_size = LFS_ROM_BLOCK_SIZE,
    .block_count = LFS_ROM_BLOCK_COUNT,
    .cache_size = LFS_ROM_CACHE_SIZE,
    .lookahead_size = LFS_ROM_LOOKAHEAD_SIZE,
    .block_cycles = LFS_ROM_BLOCK_CYCLES,
};
static lfs_context_t lfs_rom_ctx = {
    .op = &lfs_oper,
    .lfs = &lfs_rom,
    .cfg = (struct lfs_config *)&lfs_rom_cfg,
    .pMutex = NULL,
};

#pragma GCC push_options
#pragma GCC optimize("-O0")
static void lfs_rom_pre_init(void)
{
    MUTEX_INIT(lfs_rom_ctx.pMutex);
}
PRE_INIT_FUNC(lfs_rom_pre_init);
#pragma GCC pop_options

#endif // USE_LFS_ROM

/*



*/

int lfs_init(void)
{
    int err = 0;

#ifdef USE_LFS_RAM
    err = vfs_mount(LFS_RAM_LETTER, &lfs_ram_ctx);
    LFS_PRINTF("[LFS] %s( %d ) disk %s\n", __func__, err, LFS_RAM_LETTER);
    if (err)
        return err;
#endif

#ifdef USE_LFS_ROM
    err = vfs_mount(LFS_ROM_LETTER, &lfs_rom_ctx);
    LFS_PRINTF("[LFS] %s( %d ) disk %s\n", __func__, err, LFS_ROM_LETTER);
    if (err)
        return err;
#endif

    return err;
}

#endif // USE_LFS
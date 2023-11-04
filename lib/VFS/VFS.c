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

#include <VFS.h>
#ifdef USE_VFS

static vfs_file_t vfs_open_files[MAX_OPEN_FILES] = {0};

typedef struct vfs_node
{
    vfs_t *fs;
    struct vfs_node *prev;
    struct vfs_node *next;
} vfs_node;

typedef struct vfs_list_s
{
    vfs_node *head;
    vfs_node *tail;
    void (*add)(struct vfs_list_s *_this, const vfs_t *fs);
    vfs_t *(*get)(struct vfs_list_s *_this, const char *name);
    void (*remove)(struct vfs_list_s *_this, const vfs_t *fs);
    vfs_node *(*create_node)(const vfs_t *);
} vfs_list_t;

static vfs_list_t vfs_list;

static inline bool is_valid_dev(const char *name)
{
    return (name && strlen(name) >= 2 && name[1] == ':');
}

static vfs_node *create_node(const vfs_t *fs)
{
    vfs_node *node = (vfs_node *)malloc(sizeof(vfs_node));
    node->fs = (vfs_t *)fs;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

static void list_add(vfs_list_t *_this, const vfs_t *fs)
{
    vfs_node *newNode = _this->create_node(fs);
    vfs_node *head = _this->head;
    vfs_node *tail = _this->tail;
    if (head == NULL)
        _this->head = newNode;
    else
    {
        vfs_node *lastNode = tail;
        if (tail == NULL)
            lastNode = head;
        lastNode->next = newNode;
        newNode->prev = lastNode;
        _this->tail = newNode;
    }
}

static vfs_t *list_get(vfs_list_t *_this, const char *name)
{
    if (is_valid_dev(name))
    {
        for (vfs_node *drive = _this->head; drive; drive = drive->next)
            if (drive->fs->name[0] == name[0])
                return drive->fs;
    }
    return NULL;
}

void list_remove(vfs_list_t *_this, const vfs_t *item)
{
    vfs_node *prev;
    vfs_node *next;
    for (vfs_node *node = _this->head; node; node = node->next)
        if (node->fs == item)
        {
            prev = node->prev;
            next = node->next;
            prev->next = next;
            next->prev = prev;
            free(node);
            break;
        }
}

void vfs_create_list(vfs_list_t *_this)
{
    if (_this)
    {
        _this->head = NULL;
        _this->tail = NULL;
        _this->add = &list_add;
        _this->get = &list_get;
        _this->remove = &list_remove;
        _this->create_node = &create_node;
    }
}

/*

*/

static vfs_t *vfs_add_fs(const char *name, const void *file_system_context)
{
    if (vfs_list.get(&vfs_list, name))
    {
        goto EXIT;
    }
    vfs_t *fs = (vfs_t *)calloc(1, sizeof(vfs_t));
    if (fs)
    {
        vfs_list.add(&vfs_list, fs);
        fs->ctx = (void*)file_system_context;
        fs->name[0] = name[0];
        fs->name[1] = ':';
        fs->name[2] = 0;
        return fs;
    }
EXIT:
    return NULL;
}

static inline void vfs_clear_file(vfs_file_t *File)
{
    if (File)
        memset(File, 0, sizeof(vfs_file_t));
}

static bool vfs_file_is_open(const char *path)
{
    if (path)
    {
        int hash = strhash(path);
        for (int i = 0; i < MAX_OPEN_FILES; i++)
        {
            if (vfs_open_files[i].fd && vfs_open_files[i].file && vfs_open_files[i].hash && hash == vfs_open_files[i].hash)
                return true;
        }
    }
    return false;
}

static vfs_t *vfs_get_fs(const char *path, bool only_dev)
{
    if (is_valid_dev(path))
    {
        if (only_dev)
        {
        OK:
            return vfs_list.get(&vfs_list, path);
        }
        if (strlen(path) > 3 && '/' == path[2])
            goto OK;
    }
    return NULL;
}

static vfs_file_t *vfs_file_get_free_index(int *index)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (0 == vfs_open_files[i].fd)
        {
            *index = i;
            return &vfs_open_files[i];
        }
    }
    return NULL;
}

static int vfs_file_get_index(int fd)
{
    if (fd >= FILES_FD_BASE && fd < FILES_FD_BASE + MAX_OPEN_FILES)
    {
        for (int index = 0; index < MAX_OPEN_FILES; index++)
            if (vfs_open_files[index].fd == fd)
                return index;
    }
    return -1;
}

static vfs_file_t *vfs_get_file(int fd)
{
    int index = vfs_file_get_index(fd);
    if (index < 0)
    {
        return NULL;
    }
    return &vfs_open_files[index];
}

static bool vfs_is_file(vfs_file_t *File)
{
    if (File)
        return (File->fd >= FILES_FD_BASE && File->fd < FILES_FD_BASE + MAX_OPEN_FILES);
    return false;
}

/*

*/

int vfs_mount(const char *path, const void *file_system_context)
{
    int err = -ENOENT;
    if (path && file_system_context)
    {
        vfs_t *Fs = vfs_add_fs(path, file_system_context);
        if (Fs && Fs->ctx)
        {
            vfs_oper *op = (vfs_oper *)*(uint32_t *)Fs->ctx;
            if (op && op->init)
                op->init(Fs);
            if (op && op->mount)
                err = op->mount(Fs);
            else
                err = -EACCES; //13
        }
        else
            err = -ENOMEM; //12
    }
    return err;
}

int vfs_unmount(const char *path)
{
    if (path)
    {
        vfs_t *Fs = vfs_get_fs(path, 1);
        if (Fs)
        {
            for (int index = 0; index < MAX_OPEN_FILES; index++)
                if (vfs_open_files[index].fs == Fs)
                    vfs_close(vfs_open_files[index].fd);
            if (Fs->ctx)
            {
                vfs_oper *op = (vfs_oper *)*(uint32_t *)Fs->ctx;
                if (op->unmount)
                    op->unmount(Fs);
            }
            vfs_list.remove(&vfs_list, Fs);
        }
    }
    return -1;
}

int vfs_open(const char *path, int flags, int mode)
{
    int index, err = -1;
    if (false == vfs_file_is_open(path))
    {
        vfs_t *Fs;
        if ((Fs = vfs_get_fs(path, 0)))
        {
            vfs_file_t *File;
            if ((File = vfs_file_get_free_index(&index)))
            {
                vfs_oper *op = (vfs_oper *)*(uint32_t *)Fs->ctx;
                if (op && (File->file = op->open(Fs, path, flags, mode)))
                {
                    File->fs = Fs;
                    File->hash = strhash(path);
                    File->fd = index + 3;
                    return File->fd;
                }
            }
            else
            {
                vfs_clear_file(File);
            }
        }
    }
    return err;
}

#define IF_IS_VFS_FILE(OPER)                               \
    int err = -EBADF;                                      \
    vfs_file_t *File = vfs_get_file(fd);                   \
    if (NULL == File || !vfs_is_file(File))                \
        goto end;                                          \
    vfs_oper *op = (vfs_oper *)*(uint32_t *)File->fs->ctx; \
    if (op && op->OPER)

#define OPER_END() \
    end:           \
    return err;

int vfs_close(int fd)
{
    IF_IS_VFS_FILE(close)
    {
        err = op->close(File);
        vfs_clear_file(File);
    }
    OPER_END();
}

size_t vfs_write(int fd, const char *buf, size_t size)
{
    IF_IS_VFS_FILE(write)
    {
        err = op->write(File, buf, size);
    }
    OPER_END();
}

size_t vfs_read(int fd, char *buf, size_t size)
{
    IF_IS_VFS_FILE(read)
    {
        err = op->read(File, buf, size);
    }
    OPER_END();
}

_off_t vfs_seek(int fd, _off_t where, int whence)
{
    IF_IS_VFS_FILE(seek)
    {
        err = op->seek(File, where, whence);
    }
    OPER_END();
}

#pragma GCC push_options
#pragma GCC optimize("-O0")
static void pre_vfs_init(void)
{
    vfs_create_list(&vfs_list);
}
PRE_INIT_FUNC(pre_vfs_init);
#pragma GCC pop_options

int vfs_init(void)
{
    int err = 0;

#ifdef USE_LFS
    extern int lfs_init(void);
    if ((err = lfs_init()))
    {
        return err;
    }
#endif

#ifdef USE_FATFS
    extern int fatfs_init(void);
    if ((err = fatfs_init()))
    {
        return err;
    }
#endif
    return err;
}

#endif
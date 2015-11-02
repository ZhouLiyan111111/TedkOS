#include <inc/fs/filesystem.h>
#include <inc/fs/kiss.h>
#include <inc/fs/dev.h>

namespace filesystem {

const size_t MaxMountableFilesystems = 4;

/* Our singleton objects here */
Dispatcher dispatcher;
DevFS devFS;
KissFS kissFS;

Dispatcher::Dispatcher()
{
    for (auto &x : fileOfFd) x = nullptr;
    this->_devFS = &devFS;
    this->_kissFS = &kissFS;
    numFds = 0;
}

void Dispatcher::mountAll()
{
    _kissFS->init();
    _devFS->init();

    mount(_kissFS, "/");
    mount(_devFS, "/dev/");
}

void Dispatcher::mount(AbstractFS *fs, const char *path)
{
    lookup.insert(path, fs);
}

int32_t Dispatcher::read(int32_t fd, void *buf, int32_t nbytes)
{
    if (isInvalidFd(fd))
    {
        return -1;
    }
    auto fdData = fileOfFd[fd];

    int32_t result = fdData->fs->read(&fdData->fsSpecificData, fdData->offset, static_cast<uint8_t*>(buf), nbytes);
    if (result == -1) return -1;
    fdData->offset += result;
    return result;
}

int32_t Dispatcher::write(int32_t fd, const void *buf, int32_t nbytes)
{
    if (isInvalidFd(fd))
    {
        return -1;
    }
    auto fdData = fileOfFd[fd];

    int32_t result = fdData->fs->write(&fdData->fsSpecificData, fdData->offset, static_cast<const uint8_t*>(buf), nbytes);
    if (result == -1) return -1;
    fdData->offset += result;
    return result;
}

int32_t Dispatcher::open(const char *filename)
{
    const char *fn = filename;
    // Patch: support accessing rtc without fs root
    if (strncmp(fn, "rtc", 4) == 0) fn = "/dev/rtc";
    // Patch #2: append slash in front of raw path
    char goodName[70] = { '/', '\0' };
    if (fn[0] != '/')
    {
        strncpy(&goodName[1], fn, 69);
        fn = goodName;
    }

    auto x = lookup.search(fn);
    if (x.val == nullptr)
    {
        return -1;
    }
    else
    {
        AbstractFS *_fs = x.val;
        int32_t fd = numFds;
        auto fdData = pool.get();
        fileOfFd[fd] = fdData;
        *fdData = { .offset = 0, .fs = _fs };
        // Skip over x.len characters, trimming the mount point from fs
        bool result = _fs->open(fn + x.len, &fdData->fsSpecificData);
        if (!result)
        {
            pool.release(fdData);
            return -1;
        }
        numFds++;
        return fd;
    }
}

int32_t Dispatcher::close(int32_t fd)
{
    /* check invalid fd here */
    if (isInvalidFd(fd))
    {
        return -1;
    }
    auto fdData = fileOfFd[fd];
    bool result = fdData->fs->close(&fdData->fsSpecificData);
    if (!result)
    {
        return -1;
    }
    pool.release(fdData);
    numFds--;
    fileOfFd[fd] = nullptr;
    return 0;
}

bool Dispatcher::isInvalidFd(int32_t fd)
{
    auto fdData = fileOfFd[fd];
    if (fdData == nullptr) return true;
    return false;
}

void Dispatcher::register_devfs(const char* path, const FOpsTable& jtable)
{
   static_cast<DevFS *>(_devFS)->registerDevice(path, jtable);
}

}

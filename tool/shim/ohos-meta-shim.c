/*
 * ohos-meta-shim.c
 *
 * LD_PRELOAD shim for building on virtiofs shared storage.
 *
 * virtiofs (the shared disk) rejects fchmod/fchown/utimensat with EPERM for
 * non-root users. Tools in the hvigor process tree (Node fs.copyFileSync
 * copies the source file mode, CMake/ninja set metadata on outputs) treat
 * that EPERM as fatal. This shim reports success for those metadata ops when
 * the only failure is EPERM, so builds can write build dirs directly on the
 * shared disk (no bind-mount needed; DevEco Studio and build.sh share the
 * same output dirs). On local file systems the real calls still take effect.
 *
 * copy_file_range: virtiofs also rejects it with EPERM, and Rust's fs::copy
 * (used by cargo's link_or_copy fallback) treats EPERM as fatal while ENOSYS
 * triggers a read/write fallback. So on EPERM we return ENOSYS to force the
 * fallback; read/write works fine on virtiofs.
 *
 * Build: gcc -shared -fPIC -o ohos-meta-shim.so ohos-meta-shim.c -ldl
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

/* Return 0 (success) when the real call failed only because of EPERM. */
static int ignore_eperm(int ret) {
    return (ret == -1 && errno == EPERM) ? 0 : ret;
}

int fchmod(int fd, mode_t mode) {
    static int (*real)(int, mode_t);
    if (!real) real = (int (*)(int, mode_t))dlsym(RTLD_NEXT, "fchmod");
    return ignore_eperm(real(fd, mode));
}

int fchmodat(int dirfd, const char *path, mode_t mode, int flags) {
    static int (*real)(int, const char *, mode_t, int);
    if (!real) real = (int (*)(int, const char *, mode_t, int))dlsym(RTLD_NEXT, "fchmodat");
    return ignore_eperm(real(dirfd, path, mode, flags));
}

int chmod(const char *path, mode_t mode) {
    static int (*real)(const char *, mode_t);
    if (!real) real = (int (*)(const char *, mode_t))dlsym(RTLD_NEXT, "chmod");
    return ignore_eperm(real(path, mode));
}

int fchown(int fd, uid_t owner, gid_t group) {
    static int (*real)(int, uid_t, gid_t);
    if (!real) real = (int (*)(int, uid_t, gid_t))dlsym(RTLD_NEXT, "fchown");
    return ignore_eperm(real(fd, owner, group));
}

int fchownat(int dirfd, const char *path, uid_t owner, gid_t group, int flags) {
    static int (*real)(int, const char *, uid_t, gid_t, int);
    if (!real) real = (int (*)(int, const char *, uid_t, gid_t, int))dlsym(RTLD_NEXT, "fchownat");
    return ignore_eperm(real(dirfd, path, owner, group, flags));
}

int chown(const char *path, uid_t owner, gid_t group) {
    static int (*real)(const char *, uid_t, gid_t);
    if (!real) real = (int (*)(const char *, uid_t, gid_t))dlsym(RTLD_NEXT, "chown");
    return ignore_eperm(real(path, owner, group));
}

int lchown(const char *path, uid_t owner, gid_t group) {
    static int (*real)(const char *, uid_t, gid_t);
    if (!real) real = (int (*)(const char *, uid_t, gid_t))dlsym(RTLD_NEXT, "lchown");
    return ignore_eperm(real(path, owner, group));
}

int utimensat(int dirfd, const char *path, const struct timespec times[2], int flags) {
    static int (*real)(int, const char *, const struct timespec *, int);
    if (!real) real = (int (*)(int, const char *, const struct timespec *, int))dlsym(RTLD_NEXT, "utimensat");
    return ignore_eperm(real(dirfd, path, times, flags));
}

int futimens(int fd, const struct timespec times[2]) {
    static int (*real)(int, const struct timespec *);
    if (!real) real = (int (*)(int, const struct timespec *))dlsym(RTLD_NEXT, "futimens");
    return ignore_eperm(real(fd, times));
}

int futimes(int fd, const struct timeval tv[2]) {
    static int (*real)(int, const struct timeval *);
    if (!real) real = (int (*)(int, const struct timeval *))dlsym(RTLD_NEXT, "futimes");
    return ignore_eperm(real(fd, tv));
}

int utimes(const char *path, const struct timeval tv[2]) {
    static int (*real)(const char *, const struct timeval *);
    if (!real) real = (int (*)(const char *, const struct timeval *))dlsym(RTLD_NEXT, "utimes");
    return ignore_eperm(real(path, tv));
}

int utime(const char *path, const struct utimbuf *times) {
    static int (*real)(const char *, const struct utimbuf *);
    if (!real) real = (int (*)(const char *, const struct utimbuf *))dlsym(RTLD_NEXT, "utime");
    return ignore_eperm(real(path, times));
}

int lutimes(const char *path, const struct timeval tv[2]) {
    static int (*real)(const char *, const struct timeval *);
    if (!real) real = (int (*)(const char *, const struct timeval *))dlsym(RTLD_NEXT, "lutimes");
    return ignore_eperm(real(path, tv));
}

/* virtiofs rejects copy_file_range with EPERM; Rust's fs::copy treats EPERM as
 * fatal but falls back to read/write on ENOSYS. Convert EPERM -> ENOSYS. */
ssize_t copy_file_range(int fd_in, off64_t *off_in, int fd_out, off64_t *off_out,
                        size_t len, unsigned int flags) {
    static ssize_t (*real)(int, off64_t *, int, off64_t *, size_t, unsigned int);
    if (!real) {
        real = (ssize_t (*)(int, off64_t *, int, off64_t *, size_t, unsigned int))
            dlsym(RTLD_NEXT, "copy_file_range");
    }
    errno = 0;
    ssize_t ret = real(fd_in, off_in, fd_out, off_out, len, flags);
    if (ret == -1 && errno == EPERM) {
        errno = ENOSYS;
        return -1;
    }
    return ret;
}

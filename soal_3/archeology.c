#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <syslog.h>

// Global variables
#define MAX_BUFFER 1024
#define MAX_SPLIT  10000
static const char *root_path = "/home/agnesgriselda/modul4_soal3/relics";

// Helper function to build full path
static void build_full_path(char *fpath, const char *path) {
    snprintf(fpath, MAX_BUFFER, "%s%s", root_path, path);
}

// Helper function to read the size of parts
static size_t get_total_size(const char *fpath) {
    size_t total_size = 0;
    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int i = 0;

    while (true) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, i++);
        if ((fd = fopen(ppath, "rb")) == NULL) break;

        fseek(fd, 0L, SEEK_END);
        total_size += ftell(fd);
        fclose(fd);
    }
    return i == 1 ? 0 : total_size;
}

// Function to get file attributes
static int arch_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        syslog(LOG_DEBUG, "getattr: root directory");
        return 0;
    }

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = get_total_size(fpath);

    syslog(LOG_DEBUG, "getattr: path=%s size=%ld", path, stbuf->st_size);

    return stbuf->st_size == 0 ? -ENOENT : 0;
}

// Function to read directory
static int arch_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    DIR *dp = opendir(fpath);
    if (!dp) {
        syslog(LOG_ERR, "readdir: opendir failed for %s", fpath);
        return -errno;
    }

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (!strstr(de->d_name, ".000")) continue;

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char filename[MAX_BUFFER];
        strncpy(filename, de->d_name, strlen(de->d_name) - 4);
        filename[strlen(de->d_name) - 4] = '\0';

        if (filler(buf, filename, &st, 0)) break;
    }

    closedir(dp);
    syslog(LOG_DEBUG, "readdir: path=%s", path);
    return 0;
}

// Function to read data from file
static int arch_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int i = 0;
    size_t total_read = 0;

    while (size > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, i++);
        if ((fd = fopen(ppath, "rb")) == NULL) break;

        fseek(fd, 0L, SEEK_END);
        size_t part_size = ftell(fd);
        fseek(fd, 0L, SEEK_SET);

        if (offset >= part_size) {
            offset -= part_size;
            fclose(fd);
            continue;
        }

        fseek(fd, offset, SEEK_SET);
        size_t read_size = fread(buf, 1, size, fd);
        fclose(fd);

        buf += read_size;
        size -= read_size;
        total_read += read_size;
        offset = 0;
    }
    syslog(LOG_DEBUG, "read: path=%s size=%ld offset=%ld", path, total_read, offset);
    return total_read;
}

// Function to write data into file
static int arch_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int pcurrent = offset / MAX_SPLIT;
    size_t poffset = offset % MAX_SPLIT;
    size_t total_write = 0;

    while (size > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        fd = fopen(ppath, "r+b");
        if (!fd) {
            fd = fopen(ppath, "wb");
            if (!fd) {
                syslog(LOG_ERR, "write: open failed for %s", ppath);
                return -errno;
            }
        }

        fseek(fd, poffset, SEEK_SET);
        size_t write_size = size > (MAX_SPLIT - poffset) ? (MAX_SPLIT - poffset) : size;

        fwrite(buf, 1, write_size, fd);
        fclose(fd);

        buf += write_size;
        size -= write_size;
        total_write += write_size;
        poffset = 0;
    }
    syslog(LOG_DEBUG, "write: path=%s size=%ld offset=%ld", path, total_write, offset);
    return total_write;
}

// Function to unlink item
static int arch_unlink(const char *path) {
    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    int pcurrent = 0;

    while (true) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        int res = unlink(ppath);
        if (res == -1) {
            if (errno == ENOENT) break;
            syslog(LOG_ERR, "unlink: failed for %s", ppath);
            return -errno;
        }
    }
    syslog(LOG_DEBUG, "unlink: path=%s", path);
    return 0;
}

// Function to create item
static int arch_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    snprintf(fpath, sizeof(fpath), "%s%s.000", root_path, path);

    int res = creat(fpath, mode);
    if (res == -1) {
        syslog(LOG_ERR, "create: failed for %s", fpath);
        return -errno;
    }
    close(res);
    syslog(LOG_DEBUG, "create: path=%s", path);
    return 0;
}

// Function to truncate size
static int arch_truncate(const char *path, off_t size) {
    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    int pcurrent = 0;
    off_t size_rmn = size;

    while (size_rmn > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        size_t part_size = size_rmn > MAX_SPLIT ? MAX_SPLIT : size_rmn;
        int res = truncate(ppath, part_size);
        if (res == -1) {
            syslog(LOG_ERR, "truncate: failed for %s", ppath);
            return -errno;
        }
        size_rmn -= part_size;
    }

    while (true) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        int res = unlink(ppath);
        if (res == -1) {
            if (errno == ENOENT) break;
            syslog(LOG_ERR, "truncate: unlink failed for %s", ppath);
            return -errno;
        }
    }

    syslog(LOG_DEBUG, "truncate: path=%s size=%ld", path, size);
    return 0;
}

// Function to create a directory
static int arch_mkdir(const char *path, mode_t mode) {
    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    int res = mkdir(fpath, mode);
    if (res == -1) {
        syslog(LOG_ERR, "mkdir: failed for %s", fpath);
        return -errno;
    }
    syslog(LOG_DEBUG, "mkdir: path=%s", path);
    return 0;
}

// Function to remove a directory
static int arch_rmdir(const char *path) {
    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    int res = rmdir(fpath);
    if (res == -1) {
        syslog(LOG_ERR, "rmdir: failed for %s", fpath);
        return -errno;
    }
    syslog(LOG_DEBUG, "rmdir: path=%s", path);
    return 0;
}

// Function to check if FUSE can run correctly
static int check_fuse_root_path() {
    struct stat st;
    return stat(root_path, &st) == -1 ? -errno : 0;
}

// FUSE operations structure
static struct fuse_operations arch_oper = {
    .getattr    = arch_getattr,
    .readdir    = arch_readdir,
    .read       = arch_read,
    .write      = arch_write,
    .unlink     = arch_unlink,
    .create     = arch_create,
    .truncate   = arch_truncate,
    .mkdir      = arch_mkdir,
    .rmdir      = arch_rmdir,
};

int main(int argc, char *argv[]) {
    openlog("arch_fs", LOG_PID, LOG_USER);
    if (check_fuse_root_path() != 0) {
        syslog(LOG_ERR, "Error: FUSE root path is not accessible.");
        fprintf(stderr, "Error: FUSE root path is not accessible.\n");
        return 1;
    }
    umask(0);
    int ret = fuse_main(argc, argv, &arch_oper, NULL);
    closelog();
    return ret;
}

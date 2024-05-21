#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// Path root dari filesystem
static const char *root_path = "/home/pikaa/soal1-wm/portofolio";

// Prototipe fungsi
int reverse_file_content(const char *path);

// Fungsi untuk mendapatkan seluruh path
static void get_full_path(char *full_path, const char *path) {
    snprintf(full_path, 256, "%s%s", root_path, path);
}
// Fungsi untuk mendapatkan atribut file
static int fs_getattr(const char *path, struct stat *stbuf) {
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    result = lstat(full_path, stbuf);
    if (result == -1)
        return -errno;

    return 0;
}
// Fungsi untuk mengubah permission file
static int fs_chmod(const char *path, mode_t mode) {
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    result = chmod(full_path, mode);
    if (result == -1)
        return -errno;

    return 0;
}
// Fungsi untuk menambahkan watermark
int add_watermark(const char *path) {
    char command[256];
    snprintf(command, sizeof(command), "convert %s -gravity SouthEast -pointsize 36 -draw \"text 10,10 'inikaryakita.id'\" %s", path, path);
    return system(command);
}
// Fungsi untuk memindahkan file dan menambahkan watermark di folder gallery
static int fs_rename(const char *from, const char *to) {
    int result;
    char full_from[256];
    char full_to[256];
    get_full_path(full_from, from);
    get_full_path(full_to, to);

    result = rename(full_from, full_to);
    if (result == -1)
        return -errno;

    if (strncmp(to, "/gallery/", 9) == 0) {
        add_watermark(full_to);
    }

    return 0;
}
// Fungsi untuk menulis ke file dan membalik isinya di folder bahaya dengan prefix text
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    int fd;
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    fd = open(full_path, O_WRONLY);
    if (fd == -1)
        return -errno;

    result = pwrite(fd, buf, size, offset);
    if (result == -1)
        result = -errno;

    close(fd);

    if (strncmp(path, "/bahaya/test", 12) == 0) {
        reverse_file_content(full_path);
    }

    return result;
}
// Fungsi untuk membuat direktori
static int fs_mkdir(const char *path, mode_t mode) {
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    result = mkdir(full_path, mode);
    if (result == -1)
        return -errno;

    return 0;
}
// Fungsi untuk membaca isi direktori
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    char full_path[256];
    get_full_path(full_path, path);

    DIR *dp;
    struct dirent *entry;
    dp = opendir(full_path);
    if (dp == NULL)
        return -errno;

    while ((entry = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = entry->d_ino;
        st.st_mode = entry->d_type << 12;
        if (filler(buf, entry->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}
// Fungsi untuk mereverse isi file
int reverse_file_content(const char *path) {
    FILE *file = fopen(path, "r+");
    if (!file) return -errno;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(length);
    if (!content) {
        fclose(file);
        return -ENOMEM;
    }

    fread(content, 1, length, file);
    
    for (long i = 0; i < length / 2; ++i) {
        char temp = content[i];
        content[i] = content[length - 1 - i];
        content[length - 1 - i] = temp;
    }

    fseek(file, 0, SEEK_SET);
    fwrite(content, 1, length, file);
    fclose(file);
    free(content);

    return 0;
}
// Fungsi untuk membaca isi file
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    int fd;
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    fd = open(full_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    if (strncmp(path, "/bahaya/test", 12) == 0) {
        // Baca seluruh isi file
        lseek(fd, 0, SEEK_END);
        size_t length = lseek(fd, 0, SEEK_CUR);
        lseek(fd, 0, SEEK_SET);

        char *file_buf = (char *)malloc(length);
        if (!file_buf) {
            close(fd);
            return -ENOMEM;
        }

        pread(fd, file_buf, length, 0);

        // Reverse isi file
        for (size_t i = 0; i < length / 2; ++i) {
            char temp = file_buf[i];
            file_buf[i] = file_buf[length - 1 - i];
            file_buf[length - 1 - i] = temp;
        }

        // Copy isi file yang telah di reverse ke buffer
        size_t copy_size = (offset < length) ? (length - offset < size ? length - offset : size) : 0;
        memcpy(buf, file_buf + offset, copy_size);

        free(file_buf);
        close(fd);
        return copy_size;
    } else {
        result = pread(fd, buf, size, offset);
        if (result == -1)
            result = -errno;
        
        close(fd);
        return result;
    }
}
// Operasi filesystem
static struct fuse_operations fs_operations = {
    .getattr = fs_getattr,
    .chmod = fs_chmod,
    .rename = fs_rename,
    .write = fs_write,
    .mkdir = fs_mkdir,
    .readdir = fs_readdir,
    .read = fs_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &fs_operations, NULL);
}

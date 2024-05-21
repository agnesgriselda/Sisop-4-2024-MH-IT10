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

static const char *root_path = "/home/pikaa/soal1-wm/portofolio";

int add_watermark(const char *path) {
    char command[256];
    snprintf(command, sizeof(command), "convert %s -gravity SouthEast -pointsize 36 -draw \"text 10,10 'inikaryakita.id'\" %s", path, path);
    return system(command);
}
// Fungsi untuk membalik isi file
int reverse_file_content(const char *path) {
    FILE *file = fopen(path, "r+");
    if (!file) return -errno;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(length);
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
// Mendapatkan atribut file
static int fs_getattr(const char *path, struct stat *stbuf) {
    int result;
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    result = lstat(full_path, stbuf);
    if (result == -1)
        return -errno;

    return 0;
}
// Mengubah permission file
static int fs_chmod(const char *path, mode_t mode) {
    int result;
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    result = chmod(full_path, mode);
    if (result == -1)
        return -errno;

    return 0;
}
// Memindahkan file dan menambahkan watermark jika di folder gallery
static int fs_rename(const char *from, const char *to) {
    int result;
    char full_from[256];
    char full_to[256];
    snprintf(full_from, sizeof(full_from), "%s%s", root_path, from);
    snprintf(full_to, sizeof(full_to), "%s%s", root_path, to);

    result = rename(full_from, full_to);
    if (result == -1)
        return -errno;

    if (strncmp(to, "/gallery/", 9) == 0) {
        add_watermark(full_to);
    }

    return 0;
}
// Menulis ke file dan membalik isinya jika di folder bahaya dengan prefix test
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    int fd;
    int result;
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

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
// Membuat direktori
static int fs_mkdir(const char *path, mode_t mode) {
    int result;
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    result = mkdir(full_path, mode);
    if (result == -1)
        return -errno;

    return 0;
}
// Membaca isi direktori
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

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
// Membaca isi file
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    int result;
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    FILE *file = fopen(full_path, "r");
    if (!file)
        return -errno;

    if (strncmp(path, "/test", 5) == 0) {
        // Baca file
        fseek(file, 0, SEEK_END);
        size_t length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *file_buf = (char *)malloc(length);
        if (!file_buf) {
            fclose(file);
            return -ENOMEM;
        }

        fread(file_buf, 1, length, file);
        fclose(file);

        // Reverse
        for (size_t i = 0; i < length / 2; ++i) {
            char temp = file_buf[i];
            file_buf[i] = file_buf[length - 1 - i];
            file_buf[length - 1 - i] = temp;
        }

        // Copy reverse file
        size_t copy_size = (offset < length) ? (length - offset < size ? length - offset : size) : 0;
        memcpy(buf, file_buf + offset, copy_size);

        free(file_buf);
        return copy_size;
    } else {
        // Baca file secara normal
        if (fseek(file, offset, SEEK_SET) == -1) {
            fclose(file);
            return -errno;
        }
        result = fread(buf, 1, size, file);
        fclose(file);

        if (result == -1)
            result = -errno;
        
        return result;
    }
}

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

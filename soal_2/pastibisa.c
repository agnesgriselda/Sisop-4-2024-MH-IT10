#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/evp.h>

static const char *root_path = "/home/nafi/sisop4/pesan/";
static const char *password = "inikaryakita";
static const char *log_file = "/home/nafi/sisop4/rahasia/logs-fuse.log";

void log_result(int success, const char *tag, const char *information) {
    FILE *log = fopen(log_file, "a");
    if (log == NULL) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y-%H:%M:%S", t);

    fprintf(log, "[%s]::%s::[%s]::[%s]\n", success ? "SUCCESS" : "FAILED", timestamp, tag, information);
    fclose(log);
}

int decode_base64(const char *src, char *dst, size_t dst_size) {
    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    if (!ctx) return -1;
    int out_len, out_len_final;

    EVP_DecodeInit(ctx);
    if (EVP_DecodeUpdate(ctx, (unsigned char *)dst, &out_len, (const unsigned char *)src, strlen(src)) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecodeFinal(ctx, (unsigned char *)(dst + out_len), &out_len_final) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        return -1;
    }

    EVP_ENCODE_CTX_free(ctx);
    dst[out_len + out_len_final] = '\0';
    return 0;
}

int decode_hex(const char *src, char *dst, size_t dst_size) {
    size_t len = strlen(src);
    if (len % 2 != 0 || len / 2 > dst_size) return -1;

    for (size_t i = 0; i < len; i += 2) {
        unsigned int byte;
        if (sscanf(src + i, "%2x", &byte) != 1) return -1;
        dst[i / 2] = (char)byte;
    }
    dst[len / 2] = '\0';
    return 0;
}

void decode_rot13(const char *src, char *dst) {
    while (*src) {
        if ((*src >= 'A' && *src <= 'Z') || (*src >= 'a' && *src <= 'z')) {
            char offset = (*src >= 'a') ? 'a' : 'A';
            *dst = (*src - offset + 13) % 26 + offset;
        } else {
            *dst = *src;
        }
        src++;
        dst++;
    }
    *dst = '\0';
}

void reverse_text(const char *src, char *dst) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; i++) {
        dst[i] = src[len - i - 1];
    }
    dst[len] = '\0';
}

static int fs_getattr(const char *path, struct stat *stbuf) {
    int res;
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);
    res = lstat(full_path, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);
    dp = opendir(full_path);
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0)) break;
    }
    closedir(dp);
    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    if (strstr(full_path, "rahasia") != NULL) {
        char input_password[256];
        printf("Enter password to access this folder: ");
        scanf("%255s", input_password);
        if (strcmp(input_password, password) != 0) {
            log_result(0, "access", "Unauthorized access attempt to secret folder");
            return -EACCES;
        }
    }

    res = open(full_path, fi->flags);
    if (res == -1) return -errno;

    close(res);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char full_path[1024];
    char decoded_buf[4096];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    fd = open(full_path, O_RDONLY);
    if (fd == -1) return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);

    if (strncmp(buf, "base64_", 7) == 0) {
        decode_base64(buf + 7, decoded_buf, sizeof(decoded_buf));
        strncpy(buf, decoded_buf, size);
    } else if (strncmp(buf, "hex_", 4) == 0) {
        decode_hex(buf + 4, decoded_buf, sizeof(decoded_buf));
        strncpy(buf, decoded_buf, size);
    } else if (strncmp(buf, "rot13_", 6) == 0) {
        decode_rot13(buf + 6, decoded_buf);
        strncpy(buf, decoded_buf, size);
    } else if (strncmp(buf, "rev_", 4) == 0) {
        reverse_text(buf + 4, decoded_buf);
        strncpy(buf, decoded_buf, size);
    }

    return res;
}

static struct fuse_operations fs_oper = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open = fs_open,
    .read = fs_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &fs_oper, NULL);
}


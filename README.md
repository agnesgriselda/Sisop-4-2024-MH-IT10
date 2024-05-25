# Modul 4
## File System, FUSE, dan Samba

- Agnes Zenobia __Griselda__ Petrina (5027231034)
- Muhammad __Nafi__ Firdaus (5027231045)
- __Rafika__ Az Zahra Kusumastuti (5027231050)

## Soal 1
Adfi merupakan seorang CEO agency creative bernama Ini Karya Kita. Ia sedang melakukan inovasi pada manajemen project photography Ini Karya Kita. Salah satu ide yang dia kembangkan adalah tentang pengelolaan foto project dalam sistem arsip Ini Karya Kita. Dalam membangun sistem ini, Adfi tidak bisa melakukannya sendirian, dia perlu bantuan mahasiswa Departemen Teknologi Informasi angkatan 2023 untuk membahas konsep baru yang akan mengubah project fotografinya lebih menarik untuk dilihat. Adfi telah menyiapkan portofolio hasil project fotonya yang bisa didownload dan diakses di www.inikaryakita.id . Silahkan eksplorasi web Ini Karya Kita dan temukan halaman untuk bisa mendownload projectnya. Setelah kalian download terdapat folder gallery dan bahaya.
- Pada folder “gallery”:
      - Membuat folder dengan prefix "wm." Dalam folder ini, setiap gambar yang dipindahkan ke dalamnya akan diberikan watermark bertuliskan inikaryakita.id. 
			  Ex: "mv ikk.jpeg wm-foto/" 
        Output: 
        Before: (tidak ada watermark bertuliskan inikaryakita.id)
        After: (terdapat watermark tulisan inikaryakita.id)
  
  - Pada folder "bahaya," terdapat file bernama "script.sh." Adfi menyadari pentingnya menjaga keamanan dan integritas data dalam folder ini.
      - Mereka harus mengubah permission pada file "script.sh" agar bisa dijalankan, karena jika dijalankan maka dapat menghapus semua dan isi dari  "gallery"
      - Adfi dan timnya juga ingin menambahkan fitur baru dengan membuat file dengan prefix "test" yang ketika disimpan akan mengalami pembalikan (reverse) isi dari file tersebut.

## Penyelesaian
# Soal 1 : inikaryakita.c
Download `https://drive.google.com/file/d/1VP6o84AQfY6QbghFGkw6ghxkBbv7fpum/view` kemudian unzip. Setelah itu masukkan folder `portofolio` dalam folder `soal1-wm`. Kemudian buat script `inikaryakita.c` menggunakan operasi FUSE.

```c
#define FUSE_USE_VERSION 30
```
Menentukan versi FUSE-nya, disini saya menggunakan versi 30.

```c
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
```
Baris - baris di atas merupakan bagian untuk menyertakan header file yang diperlukan dalam program ini, seperti fuse.h untuk mengakses fungsi-fungsi FUSE, stdio.h untuk operasi input/output standar, stdlib.h untuk fungsi-fungsi utilitas umum, string.h untuk operasi string, errno.h untuk kode error, fcntl.h untuk operasi file control, sys/stat.h untuk informasi status file, unistd.h untuk operasi UNIX standar, dan dirent.h untuk operasi direktori.

```c
static const char *root_path = "/home/pikaa/soal1-wm/portofolio";
```
Mendefinisikan variabel `root_path` yang menyimpan path root dari filesystem yang akan diakses oleh program, yaitu `/home/pikaa/soal1-wm/portofolio`.


```c
int reverse_file_content(const char *path);
```
Deklarasi fungsi untuk membalik isi file, yang akan diimplementasikan nanti.


```c
static void get_full_path(char *full_path, const char *path) {
    snprintf(full_path, 256, "%s%s", root_path, path);
}
```
Fungsi `get_full_path` digunakan untuk mendapatkan path lengkap suatu file atau direktori dengan menggabungkan `root_path` dan `path` yang diberikan sebagai parameter. Fungsi ini menggunakan `snprintf` untuk menggabungkan string dengan ukuran max 256.


```c
static int fs_getattr(const char *path, struct stat *stbuf) {
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    result = lstat(full_path, stbuf);
    if (result == -1)
        return -errno;

    return 0;
}
```
Fungsi `fs_getattr` digunakan untuk mendapatkan atribut suatu file atau direktori dengan memanggil fungsi `lstat`. Fungsi ini memanggil `get_full_path` untuk mendapatkan path lengkap, kemudian memanggil `lstat` untuk mengisi struktur stat dengan informasi file atau direktori tersebut. Jika terjadi error, fungsi ini akan mengembalikan nilai negatif dari errno.


```c
static int fs_chmod(const char *path, mode_t mode) {
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    result = chmod(full_path, mode);
    if (result == -1)
        return -errno;

    return 0;
}
```
Fungsi `fs_chmod` digunakan untuk mengubah permission (hak akses) suatu file atau direktori dengan memanggil fungsi `chmod`. Fungsi ini memanggil `get_full_path` untuk mendapatkan path lengkap, kemudian memanggil `chmod` untuk mengubah permission sesuai dengan mode yang diberikan. Jika terjadi error, fungsi ini akan mengembalikan nilai negatif dari errno.


```c
int add_watermark(const char *path) {
    char command[256];
    snprintf(command, sizeof(command), "convert %s -gravity SouthEast -pointsize 36 -draw \"text 10,10 'inikaryakita.id'\" %s", path, path);
    return system(command);
}
```
Fungsi `add_watermark` digunakan untuk menambahkan watermark pada suatu file gambar dengan menggunakan perintah `convert` dari ImageMagick. Fungsi ini membuat string perintah dengan `snprintf` yang berisi perintah untuk menambahkan watermark `"inikaryakita.id"` dengan posisi di sudut kanan bawah dengan ukuran font 36. Kemudian, fungsi ini memanggil system untuk mengeksekusi perintah tersebut.


```c
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
```
Fungsi `fs_rename` digunakan untuk memindahkan atau mengganti nama suatu file atau direktori dengan memanggil fungsi `rename`. Fungsi ini memanggil `get_full_path` untuk mendapatkan path lengkap dari `from` dan `to`, kemudian memanggil `rename` untuk memindahkan atau mengganti nama. Jika `to` diawali dengan `/gallery/`, maka fungsi `add_watermark` akan dipanggil untuk menambahkan watermark pada file tujuan tersebut. Jika terjadi error, fungsi ini akan mengembalikan nilai negatif dari errno.


```c
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
```

Fungsi `fs_write` digunakan untuk menulis data ke suatu file dengan memanggil fungsi `pwrite`. Fungsi ini memanggil `get_full_path` untuk mendapatkan path lengkap, kemudian membuka file dengan mode `O_WRONLY` (write only) dan memanggil `pwrite` untuk menulis data dari `buf` ke file tersebut dengan panjang size dan `offset` offset. Jika path diawali dengan `/bahaya/test`, maka fungsi `reverse_file_content` akan dipanggil untuk membalik isi file tersebut. Jika terjadi error, fungsi ini akan mengembalikan nilai negatif dari errno.


```c
static int fs_mkdir(const char *path, mode_t mode) {
    int result;
    char full_path[256];
    get_full_path(full_path, path);

    result = mkdir(full_path, mode);
    if (result == -1)
        return -errno;

    return 0;
}
```
Fungsi `fs_mkdir` digunakan untuk membuat direktori baru dengan memanggil fungsi `mkdir`. Fungsi ini memanggil `get_full_path` untuk mendapatkan path lengkap


```c
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
```
Fungsi `fs_readdir` digunakan untuk membaca isi dari suatu direktori. Parameter path menyimpan path direktori yang akan dibaca, `buf` adalah buffer untuk menyimpan isi direktori, `filler` adalah fungsi callback dari FUSE untuk mengisi buffer dengan entry direktori, `offset` adalah offset untuk pembacaan direktori, dan `fi` adalah informasi file. Fungsi ini memanggil `get_full_path` untuk mendapatkan path lengkap direktori, kemudian membuka direktori dengan opendir. Setelah itu, fungsi ini membaca setiap entry direktori dengan `readdir` dan memanggil `filler` untuk mengisi buffer dengan informasi entry tersebut. Setelah selesai, direktori ditutup dengan `closedir`.


```c
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
```
Fungsi `reverse_file_content` digunakan untuk membalik isi dari suatu file. Parameter path menyimpan path file yang akan dibalik isinya. Fungsi ini membuka file dengan mode `"r+"` (read and write) menggunakan `fopen`. Kemudian, fungsi ini menggunakan `fseek` dan `ftell` untuk mendapatkan panjang file. Setelah itu, fungsi mengalokasikan memori dinamis dengan `malloc` untuk menyimpan isi file, lalu membaca isi file dengan `fread`. Selanjutnya, fungsi membalik isi file dengan menukar posisi setiap karakter di dalam array content. Terakhir, fungsi menulis kembali isi file yang telah dibalik dengan `fwrite`, menutup file dengan `fclose`, dan membebaskan memori yang dialokasikan dengan free.


```c
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
        lseek(fd, 0, SEEK_END);
        size_t length = lseek(fd, 0, SEEK_CUR);
        lseek(fd, 0, SEEK_SET);

        char *file_buf = (char *)malloc(length);
        if (!file_buf) {
            close(fd);
            return -ENOMEM;
        }

        pread(fd, file_buf, length, 0);

        for (size_t i = 0; i < length / 2; ++i) {
            char temp = file_buf[i];
            file_buf[i] = file_buf[length - 1 - i];
            file_buf[length - 1 - i] = temp;
        }

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
```
Fungsi `fs_read` digunakan untuk membaca isi dari suatu file. Parameter path menyimpan path file yang akan dibaca, `buf` adalah buffer untuk menyimpan isi file, `size` adalah ukuran maksimum data yang akan dibaca, `offset` adalah offset untuk pembacaan file, dan `fi` adalah informasi file. Fungsi ini memanggil `get_full_path` untuk mendapatkan path lengkap file, kemudian membuka file dengan mode `O_RDONLY` (read only) menggunakan `open`. Jika path diawali dengan `/bahaya/test`, maka fungsi akan membaca seluruh isi file, membalik isinya, dan menyalin isi file yang telah dibalik ke buffer `buf`. Jika bukan, fungsi akan membaca isi file secara normal dengan `pread`. Setelah selesai, file ditutup dengan `close`.


```c
static struct fuse_operations fs_operations = {
    .getattr = fs_getattr,
    .chmod = fs_chmod,
    .rename = fs_rename,
    .write = fs_write,
    .mkdir = fs_mkdir,
    .readdir = fs_readdir,
    .read = fs_read,
};
```
Mendefinisikan struktur `fuse_operations` yang berisi pointer ke fungsi-fungsi yang diimplementasikan dalam program ini, seperti `fs_getattr`, `fs_chmod`, `fs_rename`, `fs_write`, `fs_mkdir`, `fs_readdir`, dan `fs_read`. Struktur ini diperlukan oleh FUSE untuk mengakses implementasi operasi filesystem yang disediakan oleh program ini.


```c
int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &fs_operations, NULL);
}
```
Memanggil `fuse_main` dengan argument `argc` dan `argv` yang diterima dari command line, serta menyertakan pointer ke struktur `fs_operations` yang telah didefinisikan sebelumnya. Fungsi `fuse_main` akan menginisialisasi FUSE dan menjalankan operasi filesystem yang telah ditentukan dalam `fs_operations`.

Kemudian kita compile menggunakan `gcc -Wall -o inikaryakita inikaryakita.c `pkg-config fuse --cflags --libs``.
Setelah itu bikin directory baru untuk mountpointnya, disini saya mkdir `mnt`. Setelah itu kita run `./inikaryakita mnt` agar mountpoinnya jalan. Setelah kita run, nanti akan muncul folder bahaya dan gallery.
Setelah itu kita masuk ke folder `/mnt/gallery` dan buat directory baru bernama `wm`. Setelah itu kita masuk ke folder `/mnt/gallery/wm` dan kita coba move fotonya ke folder wm agar dapat watermarknya.
Setelah kita cek folder `gallery` kita pindah ke folder `bahaya`. Disitu kita cek apakah file `test-adfi.txt` nya sudah ke reverse atau belum.


## Dokumentasi Output
![Screenshot from 2024-05-24 22-42-54](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150429708/c566494a-a516-4a9c-ac5c-77d06c844714)
![Screenshot from 2024-05-24 22-43-59](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150429708/b4f50153-0d7d-4667-9a85-6729c370d80a)
![Screenshot from 2024-05-24 22-44-11](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150429708/f3f00957-a971-4ca5-94fa-7a609fe826ef)
![Screenshot from 2024-05-24 22-44-26](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150429708/d6c0c5c6-58df-4d8b-a35f-9cbb66d3b0ba)


## Soal 3
Seorang arkeolog menemukan sebuah gua yang didalamnya tersimpan banyak relik dari zaman praaksara, sayangnya semua barang yang ada pada gua tersebut memiliki bentuk yang terpecah belah akibat bencana yang tidak diketahui. Sang arkeolog ingin menemukan cara cepat agar ia bisa menggabungkan relik-relik yang terpecah itu, namun karena setiap pecahan relik itu masih memiliki nilai tersendiri, ia memutuskan untuk membuat sebuah file system yang mana saat ia mengakses file system tersebut ia dapat melihat semua relik dalam keadaan utuh, sementara relik yang asli tidak berubah sama sekali.

Ketentuan :

a. Buatlah sebuah direktori dengan ketentuan seperti pada tree berikut

```
├── [nama_bebas]
├── relics
│   ├── relic_1.png.000
│   ├── relic_1.png.001
│   ├── dst dst…
│   └── relic_9.png.010
└── report
```
b. Direktori [nama_bebas] adalah direktori FUSE dengan direktori asalnya adalah direktori relics. Ketentuan Direktori [nama_bebas] adalah sebagai berikut :

- Ketika dilakukan listing, isi dari direktori [nama_bebas] adalah semua relic dari relics yang telah tergabung.

- Ketika dilakukan copy (dari direktori [nama_bebas] ke tujuan manapun), file yang disalin adalah file dari direktori relics yang sudah tergabung.
  
- Ketika ada file dibuat, maka pada direktori asal (direktori relics) file tersebut akan dipecah menjadi sejumlah pecahan dengan ukuran maksimum tiap pecahan adalah 10kb.

- File yang dipecah akan memiliki nama [namafile].000 dan seterusnya sesuai dengan jumlah pecahannya.
Ketika dilakukan penghapusan, maka semua pecahannya juga ikut terhapus.

c. Direktori report adalah direktori yang akan dibagikan menggunakan Samba File Server. Setelah kalian berhasil membuat direktori [nama_bebas], jalankan FUSE dan salin semua isi direktori [nama_bebas] pada direktori report.


Catatan:
- pada contoh terdapat 20 relic, namun pada zip file hanya akan ada 10 relic
- [nama_bebas] berarti namanya dibebaskan
- pada soal 3c, cukup salin secara manual. File Server hanya untuk membuktikan bahwa semua file pada direktori [nama_bebas] dapat dibuka dengan baik.

## Penyelesaian 
# Soal 3 : Archeology Fuse

ArchFS adalah sebuah filesystem yang menggunakan FUSE (Filesystem in Userspace) untuk mengimplementasikan sebuah filesystem yang mempartisi file menjadi beberapa bagian.

## Persyaratan

- FUSE library (libfuse)
- GNU Compiler Collection (gcc)

## Instalasi

1. Pastikan FUSE library sudah terpasang:

    ```sh
    sudo apt-get install libfuse*
    ```

2. Kompilasi kode sumber:

    ```sh
    gcc -Wall archeology.c `pkg-config fuse --cflags --libs` -o archeology
    ```

## Penggunaan

1. Buat direktori untuk mount point:

    ```sh
    mkdir /home/agnesgriselda/modul4_soal3/gabungan
    ```

2. Jalankan filesystem:

    ```sh
    ./archeology /home/agnesgriselda/modul4_soal3/gabungan
    ```

## Struktur Direktori

Semua file dan direktori akan dipartisi dan disimpan di direktori root yang didefinisikan dalam kode sebagai `root_path`. Path default adalah:

```
/home/agnesgriselda/modul4_soal3/relics
```

## Fungsi Utama

### Helper Functions

#### `build_full_path`

Membangun path lengkap dari root dan path relatif.

```c
static void build_full_path(char *fpath, const char *path) {
    snprintf(fpath, MAX_BUFFER, "%s%s", root_path, path);
}
```

#### `get_total_size`

Menghitung total ukuran dari file yang dipartisi.

```c
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
```

### FUSE Operations

#### `arch_getattr`

Mengambil atribut dari sebuah file atau direktori.

```c
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
```

#### `arch_readdir`

Membaca isi dari sebuah direktori.

```c
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
```

#### `arch_read`

Membaca data dari sebuah file yang dipartisi.

```c
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
```

#### `arch_write`

Menulis data ke sebuah file yang dipartisi.

```c
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
```

#### `arch_unlink`

Menghapus file yang dipartisi.

```c
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
```

#### `arch_create`

Membuat file baru yang dipartisi.

```c
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
```

#### `arch_truncate`

Memotong ukuran file yang dipartisi.

```c
static int arch_truncate(const char *path, off_t size) {
    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    int pcurrent = 0;
    off_t size_rmn = size;

    while (size_rmn > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        size_t part_size = size_rmn > MAX_SPLIT ? MAX_SPLIT : size_rmn;
        int res = truncate(ppath, part

_size);
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
```

#### `arch_mkdir`

Membuat direktori baru.

```c
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
```

#### `arch_rmdir`

Menghapus direktori.

```c
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
```

### Main Function

```c
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
```

## Dokumentasi Output
![Screenshot 2024-05-25 092917](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150202762/9e07d4a5-7a7e-4a3f-85c3-24e7ac229073)

![Screenshot 2024-05-24 090426](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150202762/6880a560-0ed0-45da-95b9-14274a0b2ffd)


![Screenshot 2024-05-25 093039](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150202762/b16bde5f-95d8-40f8-a73e-0e735d188854)

![Screenshot 2024-05-25 093012](https://github.com/agnesgriselda/Sisop-4-2024-MH-IT10/assets/150202762/29df95a1-c480-4986-9f77-de8be3a3b71c)


## Revisi

Fungsi untuk membaca relics yang akan digabungkan :
```c
static int arch_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    size_t len;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int i = 0;
    char part_path[1100];
    size_t read_size = 0;

    while (size > 0) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, i++);
        FILE *fp = fopen(part_path, "rb");
        if (!fp) break;

        fseek(fp, 0L, SEEK_END);
        size_t part_size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        if (offset >= part_size) {
            offset -= part_size;
            fclose(fp);
            continue;
        }

        fseek(fp, offset, SEEK_SET);
        len = fread(buf, 1, size, fp);
        fclose(fp);

        buf += len;
        size -= len;
        read_size += len;
        offset = 0;
    }
    return read_size;
}
```
Fungsi untuk menghapus relics dan seluruh pecahannya apabila dibuat sample image baru :
```c
static int arch_unlink(const char *path) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int part_num = 0;
    char part_path[1100];
    int res = 0;

    while (1) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, part_num++);
        res = unlink(part_path);
        if (res == -1 && errno == ENOENT) break;
        else if (res == -1) return -errno;
    }
    return 0;
}
```
Fungsi untuk membuat sample image baru :
```c

static int arch_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s.000", root_path, path);

    int res = creat(fpath, mode);
    if (res == -1) return -errno;

    close(res);
    return 0;
}
```
Fungsi untuk adjust relics pecahan maupun relics yang sudah digabungkan :
```c
static int arch_truncate(const char *path, off_t size) {
    char fpath[1000];
    snprintf(fpath, sizeof(fpath), "%s%s", root_path, path);

    int part_num = 0;
    char part_path[1100];
    off_t remaining_size = size;

    while (remaining_size > 0) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, part_num++);
        size_t part_size = remaining_size > 10000 ? 10000 : remaining_size;
        int res = truncate(part_path, part_size);
        if (res == -1) return -errno;
        remaining_size -= part_size;
    }

    while (1) {
        snprintf(part_path, sizeof(part_path), "%s.%03d", fpath, part_num++);
        int res = unlink(part_path);
        if (res == -1 && errno == ENOENT) break;
        else if (res == -1) return -errno;
    }

    return 0;
}
```

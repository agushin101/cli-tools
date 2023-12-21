#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

void del_file(char *file) {
    char path[1024] = "srm_arch/";
    strcat(path, file);
    if (unlink(path) == -1) {
        fprintf(stderr, "[srm] - Error deleting file %s. Not in archive.\n", file);
        exit(1);
    }
}

void mv_file(char *file) {
    char path[1024] = "srm_arch/";
    strcat(path, file);

    struct stat statbuf;
    if (lstat(file, &statbuf) == -1) {
        fprintf(stderr, "[srm] - Error accessing file %s\n", file);
        exit(1);
    }

    if (S_ISREG(statbuf.st_mode)) {
        int fd;
        if ((fd = open(file, O_CREAT | O_WRONLY | O_EXCL, statbuf.st_mode)) == -1) {
            if (errno == EEXIST) {
                fprintf(stderr, "[srm] - File %s already archived. Either recover or delete it.\n", file);
                exit(1);
            }
            else {
                perror("[srm]");
                exit(1);
            }
        }
        int origFile = open(file, 0);
        char *buf = (char*) malloc((statbuf.st_size ) * sizeof(char));
        read(origFile, buf, statbuf.st_size);
        close(origFile);
        unlink(file);
        write(fd, buf, statbuf.st_size);
        free(buf);
        close(fd);
    }
    else {
        fprintf(stderr, "[srm] - File %s must be ordinary\n", file);
        exit(1);
    }
}

void rec_file(char *file) {
    char path[1024] = "srm_arch/";
    strcat(path, file);

    struct stat statbuf;
    if (lstat(path, &statbuf) == -1) {
        fprintf(stderr, "[srm] - Error accessing file %s\n", file);
        exit(1);
    }

    if (S_ISREG(statbuf.st_mode)) {
        int fd;
        if ((fd = open(file, O_CREAT | O_WRONLY | O_EXCL, statbuf.st_mode)) == -1) {
            if (errno == EEXIST) {
                fprintf(stderr, "[srm] - File %s already exists. Either move or delete it.\n", file);
                exit(1);
            }
            else {
                perror("[srm]");
                exit(1);
            }
        }
        int origFile = open(path, 0);
        char *buf = (char*) malloc((statbuf.st_size) * sizeof(char));
        read(origFile, buf, statbuf.st_size);
        close(origFile);
        unlink(path);
        write(fd, buf, statbuf.st_size);
        free(buf);
        close(fd);
    }
    else {
        fprintf(stderr, "[srm] - File %s must be ordinary\n", file);
        exit(1);
    }
}

int main (int argc, char *argv[]) {
    char fullDel = 0, del = 0, list = 0, clear = 0, all = 0, recover = 0;
    int opt = 0;
    opterr = 0;

    while ((opt = getopt(argc, argv, "dDlcr")) != -1) {
        switch (opt)
        {
        case 'd':
            del = 1;
            break;
        case 'D':
            fullDel = 1;
            break;
        case 'l':
            list = 1;
            break;
        case 'c':
            clear = 1;
            break;
        case 'r':
            recover = 1;
            break;
        default:
            fprintf(stderr, "[srm] - Improper command line. Use: srm [dlcrD] args\n");
            return 1;
        }
    }

    if (fullDel + del + list + clear + recover > 1) {
        fprintf(stderr, "[srm] - Improper command line. Select at most one of [dlcrD]\n");
        return 1;
    }

    mkdir("srm_arch", 0755);

    if (!(fullDel | del | list | clear | recover)) {
        for (int i = 1; i < argc; i++) {
            mv_file(argv[i]);
        }
    }

    else if (del) {
        for (int i = optind; i < argc; i++) {
            del_file(argv[i]);
        }
    }

    else if (fullDel) {
        DIR *arch = opendir("srm_arch");
        struct dirent *entry;
        while ((entry = readdir(arch)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                del_file(entry->d_name);
            }
        }
    }

    else if (list) {
        execlp("ls", "ls", "srm_arch", NULL);
    }

    else if (clear) {
        execlp("rm", "rm", "-r", "srm_arch", NULL);
    }
    
    else {
        for (int i = optind; i < argc; i++) {
            rec_file(argv[i]);
        }
    }
    return 0;
}
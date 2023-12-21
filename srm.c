#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

#define PATHSIZE 2048
#define MAGIC "!srm_arch"

int validate(int fd) {
    char buf[10];
    read(fd, buf, 9);
    buf[9] = '\0';
    if (strcmp(buf, MAGIC) != 0) {
        return 1;
    }
    return 0;
}

void del_file(char *file, const char* home) {
    char path[PATHSIZE];
    strcpy(path, home);
    strcat(path, "/.srm_arch/");
    strcat(path, file);
    if (unlink(path) == -1) {
        fprintf(stderr, "[srm] - Error deleting file %s. Not in archive.\n", file);
        exit(1);
    }
}

void mv_file(char *file, const char* home) {
    char path[PATHSIZE];
    char tokdup[PATHSIZE];
    strcpy(tokdup, file);
    char *prev = file;
    char *token = strtok(tokdup, "/");
    while ((token = strtok(NULL, "/")) != NULL) {
        prev = token;
    }
    
    strcpy(path, home);
    strcat(path, "/.srm_arch/");
    strcat(path, prev);

    struct stat statbuf;
    if (lstat(file, &statbuf) == -1) {
        fprintf(stderr, "[srm] - Error accessing file %s\n", file);
        exit(1);
    }

    if (S_ISREG(statbuf.st_mode)) {
        int fd;
        if ((fd = open(path, O_CREAT | O_WRONLY | O_EXCL, statbuf.st_mode)) == -1) {
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
        char magic[10] = "!srm_arch";
        read(origFile, buf, statbuf.st_size);
        close(origFile);
        unlink(file);
        write(fd, magic, 9);
        write(fd, buf, statbuf.st_size);
        free(buf);
        close(fd);
    }
    else {
        fprintf(stderr, "[srm] - File %s must be ordinary\n", file);
        exit(1);
    }
}

void rec_file(char *file, const char* home) {
    char path[PATHSIZE];
    char newpath[PATHSIZE];
    strcpy(path, home);
    strcat(path, "/.srm_arch/");
    strcat(path, file);

    getcwd(newpath, PATHSIZE);
    strcat(newpath, "/");
    strcat(newpath, file);

    struct stat statbuf;
    if (lstat(path, &statbuf) == -1) {
        fprintf(stderr, "[srm] - Error accessing file %s\n", file);
        exit(1);
    }

    if (S_ISREG(statbuf.st_mode)) {
        int origFile = open(path, 0);
        if (validate(origFile) == 1) {
            fprintf(stderr, "[srm] - Failed access on corrupt file %s.\n", file);
            exit(1);
        }
        char *buf = (char*) malloc((statbuf.st_size) * sizeof(char));
        read(origFile, buf, statbuf.st_size - 9);
        close(origFile);
        int fd;
        if ((fd = open(newpath, O_CREAT | O_WRONLY | O_EXCL, statbuf.st_mode)) == -1) {
            if (errno == EEXIST) {
                fprintf(stderr, "[srm] - File %s already exists in CWD. Either move or delete it.\n", file);
                exit(1);
            }
            else {
                perror("[srm]");
                exit(1);
            }
        }
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

void help() {
    printf("srm [dlcr] file1 file2...\n");
    printf("If used with no arguments, safely remove target files from the current directory tree.\n");
    printf("These files are stored in a safety archive until a secondary command is given to permanently delete them.\n");
    printf("-d - Delete the target files from the safety archive.\n");
    printf("-l - List the contents of the safety archive.\n");
    printf("-c - Clears the safety archive, deleting all files.\n");
    printf("-r - Recovers target files from the safety archive.\n");
}

int main (int argc, char *argv[]) {
    char del = 0, list = 0, clear = 0, all = 0, recover = 0;
    int opt = 0;
    opterr = 0;

    while ((opt = getopt(argc, argv, "dlcrh")) != -1) {
        switch (opt)
        {
        case 'd':
            del = 1;
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
        case 'h':
            help();
            return 0;
        default:
            fprintf(stderr, "[srm] - Improper command line. Use: srm [dlcr] args\n");
            return 1;
        }
    }

    if (argc == 1) {
        help();
        return 0;
    }

    if (del + list + clear + recover > 1) {
        fprintf(stderr, "[srm] - Improper command line. Select at most one of [dlcr]\n");
        return 1;
    }

    char *homed= getenv("HOME");
    char path[50];
    strcpy(path, homed);
    strcat(path, "/.srm_arch/");
    

    mkdir(path, 0755);

    if (!(del | list | clear | recover)) {
        for (int i = 1; i < argc; i++) {
            mv_file(argv[i], homed);
        }
    }

    else if (del) {
        for (int i = optind; i < argc; i++) {
            del_file(argv[i], homed);
        }
    }

    else if (list) {
        execlp("ls", "ls", path, NULL);
    }

    else if (clear) {
        execlp("rm", "rm", "-r", path, NULL);
    }
    
    else {
        for (int i = optind; i < argc; i++) {
            rec_file(argv[i], homed);
        }
    }
    return 0;
}
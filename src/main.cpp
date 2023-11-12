#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include <vector>

#define SHARED_MEMORY_SIZE 1024

void checkStr(const char *ptr, char *shared_error, FILE *file, int size) {
    std::string res;
    for (int i = 1; i < size - 1; ++i) {
        if (isupper(ptr[i])) {
            int j = i;
            std::string tmp = "";
            while (ptr[j] != '\0' && ptr[j] != '\n') {
                tmp.push_back(ptr[j]);
                j++;
            }
            fprintf(file, "%s\n",tmp.c_str());
            res += "String start with uppercase\n";
            i = j;
        } else {
            res += "String does not start with uppercase\n";
            int j = i;
            while (ptr[j] != '\0' && ptr[j] != '\n') {
                j++;
            }
            i = j;
        }
    }
    strcpy(shared_error, res.c_str());
}

char* getInput(int& size) {
    char symbol;
    char* in = (char*)malloc(sizeof(char));
    if (in == NULL) {
        perror("malloc");
    }
    size_t i = 0;
    size = 1;
    std::cout << "Enter something strings. If you want to stop, press Ctrl+D" << std::endl;
    while ((symbol = getchar()) != EOF) {
        in[i++] = symbol;
        if (i == size) {
            size *= 2;
            in = (char*)realloc(in, size * sizeof(char));
            if (in == NULL) {
                perror("realloc");
            }
        }
    }
    size = i + 1;
    in = (char*)realloc(in, size * sizeof(char));
    if (in == NULL) {
        perror("realloc");
    }
    in[size - 1] = '\0';
    return in;
}


int main() {
    std::string filePath;
    std::cout << "write path to file\n";
    std::cin >> filePath;

    FILE *file = fopen(filePath.c_str(), "a");

    int fd_input = shm_open("input", O_CREAT | O_RDWR, 0666);

    if (fd_input == -1) {
        shm_unlink("input");
        perror("shm_open");
    }

    if (ftruncate(fd_input, SHARED_MEMORY_SIZE) == -1) {
        shm_unlink("input");
        perror("ftruncate");
    }

    char * shared_input = (char *) mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_input, 0);

    if (shared_input == MAP_FAILED) {
        munmap(shared_input, SHARED_MEMORY_SIZE);
        shm_unlink("input");
        perror("mmap");
    }

    close(fd_input);

    int fd_error = shm_open("error", O_CREAT | O_RDWR, 0666);

    if (fd_error == -1) {
        shm_unlink("error");
        munmap(shared_input, SHARED_MEMORY_SIZE);
        shm_unlink("input");
        perror("shm_open");
    }

    if (ftruncate(fd_error, SHARED_MEMORY_SIZE) == -1) {
        shm_unlink("error");
        munmap(shared_input, SHARED_MEMORY_SIZE);
        shm_unlink("input");
        perror("ftruncate");
    }

    char * shared_error = (char *) mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_error, 0);

    if (shared_error == MAP_FAILED) {
        munmap(shared_error, SHARED_MEMORY_SIZE);
        shm_unlink("error");
        munmap(shared_input, SHARED_MEMORY_SIZE);
        shm_unlink("input");
        perror("mmap");
    }

    close(fd_error);

    char * input;
    int size;
    input = getInput(size);

    pid_t pid = fork();
    if (pid == 0) {
        checkStr(input, shared_error, file, size);
    } else if (pid > 0) {
        wait(NULL);
        int i = 0;
        for (int i = 0; i < strlen(shared_error); i++) {
            std::cout << shared_error[i];
        }
    } else {
        munmap(shared_error, SHARED_MEMORY_SIZE);
        shm_unlink("error");
        munmap(shared_input, SHARED_MEMORY_SIZE);
        shm_unlink("input");
        perror("fork");
    }

    munmap(shared_error, SHARED_MEMORY_SIZE);
    shm_unlink("error");
    munmap(shared_input, SHARED_MEMORY_SIZE);
    shm_unlink("input");
}
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

enum source_type
{
    F, // FILE  is used by lang in /usr/include/stdio.h it's [typedef struct _IO_FILE FILE;]
    D,
    NOT_EXIST
};

char *read_input_line();
enum source_type read_source(char **path);
void read_destination(char **path, char *source_name);
enum source_type get_source_type(const char *path);
void copy_file(const char *source_path, const char *destination_path);
void create_directories_recursively(const char *path);

int main(char *argv[], int argc)
{

    // take list of (files/folders) and (copy/cut) them to a new location
    // inform user with sum of sources and free space of destination
    // https://github.com/coreutils/coreutils/blob/master/src/cp.c

    char *source = NULL;
    char *destination = NULL;
    enum source_type src_type = read_source(&source);
    char *source_name = strrchr(source, '/');
    read_destination(&destination, source_name); // strrchr(path, '/') → returns a pointer to the last occurrence of '/'.
    if (src_type == F)
    {
        copy_file(source, destination);
    }
    else if (src_type == D)
    {
        printf("Directory source selected.\n");
    }
    free(source);
    free(destination);

    // error cases
    // 1. source doesn't exist ✅
    // 2. destination doesn't exist (create it)
    // 3. duplicated source
    // 4. different sources but same (file/folder) name  EX: src/a.txt, src2/a.txt -> dest/a.txt (error)
    // 5. file and dir with same name in source EX: src/a.txt, src/a.txt/ (error)
    // 6. take permission from user
    // 7. distination and source are the same

    return 0;
}

enum source_type read_source(char **path)
{
    printf("Source: "); // I can write it like this ==> write(1, "Source: ", 8); // 1 is stdout
    *path = read_input_line();
    enum source_type src_type = get_source_type(*path);
    while (src_type == NOT_EXIST)
    {
        printf("Not found!.\nSource: ");
        *path = read_input_line();
        src_type = get_source_type(*path);
    }
    return src_type;
}

void read_destination(char **path, char *source_name)
{
    printf("Destination: ");
    *path = read_input_line();
    enum source_type dest_type = get_source_type(*path);
    while (dest_type != D)
    {
        if (dest_type == NOT_EXIST)
        {
            printf("Creating destination directory(Y/N).\n");
            char answer = getchar();
            if (answer == 'Y' || answer == 'y')
            {
                create_directories_recursively(*path);
                break;
            }
        }
        else
        {
            printf("Destination must be a directory.\nDestination: ");
            *path = read_input_line();
            dest_type = get_source_type(*path);
        }
    }
    char last_char_index = strlen(*path) - 1;
    if ((*path)[last_char_index] == '/')
    {
        (*path)[last_char_index] = '\0';
    }
    *path = realloc(*path, strlen(*path) + strlen(source_name) + 1);
    strcat(*path, source_name);
}

char *
read_input_line()
{
    char *str = NULL;
    int size = 0;
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) // cand use [read(0, &ch, 1)] instead of getchar()
    {
        char *tmp = realloc(str, size + 2);
        if (!tmp)
        {
            free(str);
            perror("Failed to read input");
            exit(EXIT_FAILURE); // #define	EXIT_FAILURE	1 in stdlib
        }
        str = tmp;
        str[size++] = ch;
        str[size] = '\0';
    }

    return str;
}

enum source_type get_source_type(const char *path)
{
    struct stat source_state;
    stat(path, &source_state);
    // the prefix S_I stands for “Status – Indication (of) permission bits”,
    // S_IRUSR; // I can use this as permission indicator for the owner but [perror] will clear that is permission denied
    if (S_ISREG(source_state.st_mode))
    {
        return F;
    }
    else if (S_ISDIR(source_state.st_mode))
    {
        return D;
    }
    else
    {
        return NOT_EXIST;
    }
}

void copy_file(const char *source_path, const char *destination_path)
{

    printf("Copying file from %s to %s\n", source_path, destination_path);
    // open return [file descriptor] is a number for file in proccess
    int source_file = open(source_path, O_RDONLY);
    if (source_file == -1)
    {
        perror("Failed to open source file");
        exit(EXIT_FAILURE);
    }

    int destination_file = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destination_file == -1)
    {
        perror("Failed to open/create destination file");
        close(source_file);
        exit(EXIT_FAILURE);
    }

    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(source_file, buffer, sizeof(buffer))) > 0)
    {
        if (write(destination_file, buffer, bytes) != bytes)
        {
            perror("Failed to write to destination file");
            break;
        }
    }

    if (bytes == -1)
    {
        perror("Failed to read from source file");
    }

    close(source_file);
    close(destination_file);
}

void create_directories_recursively(const char *path)
{
    int len = strlen(path);
    char *tmp_path = malloc(len + 1);
    tmp_path[0] = '/';
    tmp_path[len] = '\0';
    for (int i = 1; i < len; i++)
    {

        if (path[i] == '/' || i == len - 1)
        {
            tmp_path[i] = '\0';
            if (mkdir(tmp_path, 0777) == -1)
            {
                if (errno != EEXIST)
                {
                    perror("Error creating directory");
                    free(tmp_path);
                    exit(EXIT_FAILURE);
                }
            }
        }

        tmp_path[i] = path[i];
    }
    if (path[len - 1] != '/')
    {
        if (mkdir(tmp_path, 0777) == -1)
        {
            if (errno != EEXIST)
            {
                perror("Error creating directory");
                free(tmp_path);
                exit(EXIT_FAILURE);
            }
        }
    }
    free(tmp_path);
}

// char* get_source_name(const char* path) {
//     char* tmp_path = strdup(path);
//     int len = strlen(tmp_path);
//     // I dont know why andy one want to copy the root directory but ok
//     if (len == 1){
//         return "/";
//     }
//     else if (tmp_path[len - 1] == '/')
//     {
//         tmp_path[len - 1] = '\0';
//     }
//     char *name = strdup(strrchr(tmp_path, '/') + 1);
//     free(tmp_path);

//     printf("name: [ %s ]: ", name);
//     // reead input if user want to change
// }

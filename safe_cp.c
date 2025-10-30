#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <stdbool.h>

#define NL printf("\n\n")
enum source_type
{
    F, // FILE  is used by lang in /usr/include/stdio.h it's [typedef struct _IO_FILE FILE;]
    D, // Directory
    NOT_EXIST
};
void copy_file(const char *source_path, const char *destination_path, const char *file_name, bool enable_overwrite);
enum source_type get_source_type(const char *path);
void copy_directory(const char *source_dir, const char *destination_dir, const char *dir_name, bool enable_overwrite);
void format_path(char **path);
void decode_source_path(const char *path, char **name, char **true_path);
void read_string(char *buffer, size_t buffer_size);
char read_char();
bool make_dir(const char *path);
void create_directories_recursively(const char *path);
char *cwd = NULL;
char *parent_dir = NULL;
int main(int argc, char *argv[])
{
    char **sources = NULL;
    char *destination = NULL;
    cwd = realpath(".", NULL);
    parent_dir = realpath("..", NULL);
    int source_count = 0;

    for (int i = 1; i < argc; i++)
    {
        if (!(strcmp(argv[i], "-s")))
        {
            while (++i < argc && argv[i][0] != '-')
            {
                sources = realloc(sources, sizeof(char *) * (source_count + 1));
                sources[source_count] = strdup(argv[i]);
                format_path(&sources[source_count]);
                source_count++;
            }
            i--; // Adjust index since the outer loop will increment it
        }
        else if (!(strcmp(argv[i], "-d")))
        {
            if (++i < argc && argv[i][0] != '-')
            {
                destination = strdup(argv[i]);
                format_path(&destination);
                create_directories_recursively(destination);
            }
        }
    }

    if (destination == NULL || sources == NULL || source_count == 0)
    {
        perror("Usage: safe_cp -s <source1> <source2> ... -d <destination_directory>");
        exit(EXIT_FAILURE);
    }

    enum source_type src_type = NOT_EXIST;
    char *name;
    char *source_path;

    for (int i = 0; i < source_count; i++)
    {
        decode_source_path(sources[i], &name, &source_path);
        src_type = get_source_type(source_path);
        if (src_type == F)
            copy_file(source_path, destination, name, true);
        else if (src_type == D)
            copy_directory(source_path, destination, name, true);
        else
            printf("Can't find Source %s . Skipping.\n\n", source_path);
        free(sources[i]);
        free(name);
        free(source_path);
    }

    free(sources);
    free(destination);
    free(cwd);
    free(parent_dir);
    printf("Done\n\n");
    return 0;
}

enum source_type get_source_type(const char *path)
{
    struct stat source_state;
    if (stat(path, &source_state) != 0)
        return NOT_EXIST;
    // the prefix S_I stands for “Status – Indication (of) permission bits”,
    // S_IRUSR; // I can use this as permission indicator for the owner but [perror] will clear that is permission denied
    if (S_ISREG(source_state.st_mode))
        return F;
    else if (S_ISDIR(source_state.st_mode))
        return D;
    else
        return NOT_EXIST;
}

void copy_file(const char *source_path, const char *destination_path, const char *file_name, bool enable_overwrite)
{

    // full path = destination_path/file_name\0
    char *full_destination_path = malloc(strlen(destination_path) + strlen(file_name) + 2);
    sprintf(full_destination_path, "%s/%s", destination_path, file_name);

    // open return [file descriptor] is a number for file in proccess
    int source_file = open(source_path, O_RDONLY);
    if (source_file == -1)
    {
        perror("Failed to open source file");
        free(full_destination_path);
        return;
    }
    enum source_type dest_type = get_source_type(full_destination_path);
    char new_name[256];

    while (dest_type == D)
    {
        printf("Destination %s is a directory.\nCannot overwrite a directory with a file.\nEnter new name for %s: ", full_destination_path, source_path);
        read_string(new_name, sizeof(new_name));
        NL;
        full_destination_path = realloc(full_destination_path, strlen(destination_path) + strlen(new_name) + 2);
        sprintf(full_destination_path, "%s/%s", destination_path, new_name);
        dest_type = get_source_type(full_destination_path);
    }
    while (dest_type == F && enable_overwrite)
    {
        printf("Destination %s already exists.\nDo you want to overwrite it by %s ? (y/n): ", full_destination_path, source_path);
        char response = read_char();
        NL;

        if (response != 'y' && response != 'n')
        {
            printf("Invalid response. Please enter 'y' or 'n'.\n\n");
            continue;
        }
        if (response == 'n')
        {
            printf("Enter new name for %s: ", source_path);
            read_string(new_name, sizeof(new_name));
            NL;
            full_destination_path = realloc(full_destination_path, strlen(destination_path) + strlen(new_name) + 2);
            sprintf(full_destination_path, "%s/%s", destination_path, new_name);
        }
        else
            break;

        dest_type = get_source_type(full_destination_path);
    }
    int destination_file = open(full_destination_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destination_file == -1)
    {
        perror("Failed to open/create destination file");
        close(source_file);
        free(full_destination_path);
        return;
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
    free(full_destination_path);
}

void copy_directory(const char *source_dir, const char *destination_dir, const char *dir_name, bool enable_overwrite)
{

    if (!strcmp(source_dir, "/"))
    {
        printf("Cannot copy root directory (/). Skipping copy.\n\n");
        return;
    }

    if (!(strcmp(source_dir, destination_dir)))
    {
        printf("Source and destination paths are the same (%s). Skipping copy.\n\n", source_dir);
        return;
    }

    int source_len = strlen(source_dir);
    int dest_len = strlen(destination_dir);

    // used [destination_dir[source_len] == '/']
    // cuz it might be like from : /home/user/dir1 to /home/user/dir123
    if (dest_len > source_len && !strncmp(source_dir, destination_dir, source_len) && destination_dir[source_len] == '/')
    {
        printf("Cannot copy parent directory (%s) into its child (%s). Skipping copy.\n\n", source_dir, destination_dir);
        return;
    }

    DIR *dir = opendir(source_dir);
    if (!dir)
    {
        perror("Failed to open source directory");
        return;
    }

    // full path = destination_dir/dir_name\0
    char *full_destination_path = malloc(strlen(destination_dir) + strlen(dir_name) + 2);
    sprintf(full_destination_path, "%s/%s", destination_dir, dir_name);

    enum source_type dest_type = get_source_type(full_destination_path);
    char new_name[256];

    while (dest_type == F)
    {
        printf("Destination %s is a file.\nCannot overwrite a file with a directory.\nEnter new name for %s: ", full_destination_path, source_dir);
        read_string(new_name, sizeof(new_name));
        NL;
        full_destination_path = realloc(full_destination_path, strlen(destination_dir) + strlen(new_name) + 2);
        sprintf(full_destination_path, "%s/%s", destination_dir, new_name);
        dest_type = get_source_type(full_destination_path);
    }
    char user_overwrite_response;
    while (dest_type == D && enable_overwrite)
    {
        printf("Destination %s already exists.\nDo you want to overwrite it by %s ? (y/n): ", full_destination_path, source_dir);
        user_overwrite_response = read_char();
        NL;

        if (user_overwrite_response != 'y' && user_overwrite_response != 'n')
        {
            printf("Invalid response. Please enter 'y' or 'n'.\n\n");
            continue;
        }
        if (user_overwrite_response == 'n')
        {
            printf("Enter new name for %s: ", source_dir);
            read_string(new_name, sizeof(new_name));
            NL;
            full_destination_path = realloc(full_destination_path, strlen(destination_dir) + strlen(new_name) + 2);
            sprintf(full_destination_path, "%s/%s", destination_dir, new_name);
        }
        else
            break;

        dest_type = get_source_type(full_destination_path);
    }

    // Create the destination directory
    if (!make_dir(full_destination_path))
    {
        closedir(dir);
        free(full_destination_path);
        return;
    }
    struct dirent *entry;
    enable_overwrite = (enable_overwrite && (user_overwrite_response == 'n'));
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char *source_path = malloc(strlen(source_dir) + strlen(entry->d_name) + 2);
        sprintf(source_path, "%s/%s", source_dir, entry->d_name);

        enum source_type src_type = get_source_type(source_path);
        if (src_type == F)
            copy_file(source_path, full_destination_path, entry->d_name, enable_overwrite);
        else if (src_type == D)
            copy_directory(source_path, full_destination_path, entry->d_name, enable_overwrite);
        else
            printf("Can't find Source %s . Skipping.\n\n", source_path);

        free(source_path);
    }
    free(full_destination_path);

    closedir(dir);
}

void format_path(char **path)
{
    int len = strlen(*path);
    if (len > 1 && (*path)[len - 1] == '/')
    {
        (*path)[len - 1] = '\0';
    }

    if ((*path)[0] == '/')
        return;

    else if (!strncmp("./", (*path), 2))
    {
        char *tmp = malloc(len + strlen(cwd) + 2);
        sprintf(tmp, "%s/%s", cwd, (*path) + 2);
        free(*path);
        *path = tmp;
        return;
    }
    else if (!strncmp("../", (*path), 3))
    {
        char *tmp = malloc(len + strlen(parent_dir) + 2);
        sprintf(tmp, "%s/%s", parent_dir, (*path) + 3);
        free(*path);
        *path = tmp;
        return;
    }
    else if (!strcmp("..", (*path)))
    {
        *path = strdup(parent_dir);
        return;
    }
    else if (!strcmp(".", (*path)))
    {
        *path = strdup(cwd);
        return;
    }
    else
    {
        char *tmp = malloc(len + strlen(cwd) + 2);
        sprintf(tmp, "%s/%s", cwd, *path);
        free(*path);
        *path = tmp;
    }
}

void decode_source_path(const char *path, char **name, char **true_path)
{
    int len = strlen(path);
    char *colon = strrchr(path, ':');

    if (get_source_type(path) == NOT_EXIST && colon && path[len - 1] != ':')
    {
        *name = strdup(colon + 1);
        int true_path_len = colon - path;
        *true_path = malloc(true_path_len + 1);
        strncpy(*true_path, path, true_path_len);
        (*true_path)[true_path_len] = '\0';
    }
    else
    {
        *name = strdup(strrchr(path, '/') + 1);
        *true_path = strdup(path);
    }
}

void create_directories_recursively(const char *path)
{
    enum source_type type = get_source_type(path);

    if (type == D)
        return;

    if (type == F)
    {
        printf("Path %s is a file, choose a different path.\n\n", path);
        exit(EXIT_FAILURE);
    }

    int len = strlen(path), i = 0;
    char *tmp_path = malloc(len + 1);
    tmp_path[len] = '\0';

    if (path[0] == '/')
    {
        i = 1;
        tmp_path[0] = '/';
    }
    else if (path[0] == '.')
    {
        i = 2;
        tmp_path[0] = '.';
        tmp_path[1] = '/';
    }

    for (; i < len; i++)
    {
        if (path[i] == '/')
        {
            tmp_path[i] = '\0';
            if (!make_dir(tmp_path))
            {
                free(tmp_path);
                exit(EXIT_FAILURE);
            }
        }
        tmp_path[i] = path[i];

        if (i == len - 1)
        {
            if (!make_dir(tmp_path))
            {
                free(tmp_path);
                exit(EXIT_FAILURE);
            }
        }
    }
    free(tmp_path);
}

void read_string(char *buffer, size_t buffer_size)
{
    if (fgets(buffer, buffer_size, stdin))
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0'; // remove newline
    }
}

// Reads a single non-whitespace character from user input (lowercased)
char read_char()
{
    char line[16];
    if (fgets(line, sizeof(line), stdin))
    {
        for (int i = 0; line[i] != '\0'; i++)
        {
            if (!isspace((unsigned char)line[i]))
            {
                return tolower(line[i]);
            }
        }
    }
    return '\0'; // return null if nothing valid entered
}

bool make_dir(const char *path)
{
    if (mkdir(path, 0755) == -1 && errno != EEXIST)
    {
        perror("Failed to create destination directory");
        return false;
    }
    return true;
}
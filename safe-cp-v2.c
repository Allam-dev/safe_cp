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
// /path/to/anything/ => /path/to/anything
void remove_last_slash(char **path);
void decode_source_path(const char *path, char **name, char **true_path);
void read_string(char *buffer, size_t buffer_size);
char read_char();
bool make_dir(const char *path);
bool create_directories_recursively(const char *path);
void show_help_msg();
int main(int argc, char *argv[])
{

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        show_help_msg();
        return 0;
    }

    char **sources = NULL;
    char *destination = NULL;
    int source_count = 0;

    for (int i = 1; i < argc; i++)
    {
        if (!(strcmp(argv[i], "-s")))
        {
            while (++i < argc && argv[i][0] != '-')
            {
                sources = realloc(sources, sizeof(char *) * (source_count + 1));
                sources[source_count] = strdup(argv[i]);
                remove_last_slash(&sources[source_count]);
                source_count++;
            }
            i--; // Adjust index since the outer loop will increment it
        }
        else if (!(strcmp(argv[i], "-d")))
        {
            if (++i < argc && argv[i][0] != '-')
            {
                destination = strdup(argv[i]);
                remove_last_slash(&destination);
                if (!create_directories_recursively(destination))
                {
                    if (sources)
                    {
                        for (int j = 0; j < source_count; j++)
                            free(sources[j]);
                        free(sources);
                    }
                    free(destination);
                    exit(EXIT_FAILURE);
                }
                char *tmp = strdup(destination);
                free(destination);
                destination = realpath(tmp, NULL);
                free(tmp);
            }
        }
    }

    if (destination == NULL || sources == NULL)
    {
        show_help_msg();
        if (sources)
        {
            for (int j = 0; j < source_count; j++)
                free(sources[j]);
            free(sources);
        }
        if (destination)
            free(destination);
        exit(EXIT_FAILURE);
    }

    enum source_type src_type = NOT_EXIST;
    char *name;
    char *source_path;

    for (int i = 0; i < source_count; i++)
    {
        decode_source_path(sources[i], &name, &source_path);
        printf("Processing source: %s\n", source_path);
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
    return 0;
}

enum source_type get_source_type(const char *path)
{
    struct stat source_state;
    enum source_type type = NOT_EXIST;
    if (stat(path, &source_state) != 0)
        return type;
    if (S_ISREG(source_state.st_mode))
        type = F;
    else if (S_ISDIR(source_state.st_mode))
        type = D;
    return type;
}

void copy_file(const char *source_path, const char *destination_path, const char *file_name, bool enable_overwrite_check)
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
    struct stat source_state;
    stat(source_path, &source_state);
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
    while (dest_type == F && enable_overwrite_check)
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
    int destination_file = open(full_destination_path, O_WRONLY | O_CREAT | O_TRUNC, source_state.st_mode);
    if (destination_file == -1)
    {
        perror("Failed to open/create destination file");
        close(source_file);
        free(full_destination_path);
        return;
    }
    int buffer_size = 100 * 1024 * 1024; // 100 MB
    char *buffer = NULL;
    while (!(buffer = malloc(buffer_size)) && buffer_size > 4096)
        buffer_size /= 2;
    if (buffer == NULL)
    {
        printf("Failed to allocate memory for file copy buffer.\n");
        close(source_file);
        close(destination_file);
        free(full_destination_path);
        return;
    }
    ssize_t bytes;
    while ((bytes = read(source_file, buffer, buffer_size)) > 0)
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
    free(buffer);
}

void copy_directory(const char *source_dir, const char *destination_dir, const char *dir_name, bool enable_overwrite_check)
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
    bool recursive_overwrite_check = enable_overwrite_check;

    // full path = destination_dir/dir_name\0
    char *full_destination_path = malloc(strlen(destination_dir) + strlen(dir_name) + 2);
    sprintf(full_destination_path, "%s/%s", destination_dir, dir_name);

    enum source_type dest_type = get_source_type(full_destination_path);

    if (dest_type == NOT_EXIST)
        recursive_overwrite_check = false;

    char new_name[256];
    while (dest_type == F)
    {
        printf("Destination %s is a file.\nCannot overwrite a file with a directory.\nEnter new name for %s: ", full_destination_path, source_dir);
        read_string(new_name, sizeof(new_name));
        NL;
        full_destination_path = realloc(full_destination_path, strlen(destination_dir) + strlen(new_name) + 2);
        sprintf(full_destination_path, "%s/%s", destination_dir, new_name);
        dest_type = get_source_type(full_destination_path);
        if (dest_type == NOT_EXIST)
            recursive_overwrite_check = false;
    }
    while (dest_type == D && enable_overwrite_check)
    {
        printf("Destination %s already exists.\nDo you want to overwrite it by %s ? (y/n): ", full_destination_path, source_dir);
        char want_overwrite = read_char();
        NL;

        if (want_overwrite != 'y' && want_overwrite != 'n')
        {
            printf("Invalid response. Please enter 'y' or 'n'.\n\n");
            continue;
        }
        if (want_overwrite == 'n')
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
        if (dest_type == NOT_EXIST)
            recursive_overwrite_check = false;
    }

    // Create the destination directory
    if (!make_dir(full_destination_path))
    {
        closedir(dir);
        free(full_destination_path);
        return;
    }
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char *source_path = malloc(strlen(source_dir) + strlen(entry->d_name) + 2);
        sprintf(source_path, "%s/%s", source_dir, entry->d_name);

        enum source_type src_type = get_source_type(source_path);
        if (src_type == F)
            copy_file(source_path, full_destination_path, entry->d_name, recursive_overwrite_check);
        else if (src_type == D)
            copy_directory(source_path, full_destination_path, entry->d_name, recursive_overwrite_check);
        else
            printf("Can't find Source %s . Skipping.\n\n", source_path);

        free(source_path);
    }
    free(full_destination_path);

    closedir(dir);
}

void remove_last_slash(char **path)
{
    int len = strlen(*path);
    if (len > 1 && (*path)[len - 1] == '/')
    {
        (*path)[len - 1] = '\0';
    }
}

void decode_source_path(const char *path, char **name, char **true_path)
{
    char *r_path = realpath(path, NULL);
    int len = strlen(path);
    char *colon = strrchr(path, ':');
    char *last_slash = strrchr(path, '/');

    if (r_path)
    {
        *name = strdup(strrchr(r_path, '/') + 1);
        *true_path = strdup(r_path);
    }
    else if (colon && colon > last_slash && path[len - 1] != ':')
    {
        *name = strdup(colon + 1);
        int true_path_len = colon - path;
        char *tmp = malloc(true_path_len + 1);
        strncpy(tmp, path, true_path_len);
        (tmp)[true_path_len] = '\0';
        r_path = realpath(tmp, NULL);
        if (r_path)
            *true_path = strdup(r_path);
        else
            *true_path = strdup(tmp);
        free(tmp);
    }
    else
    {
        *true_path = strdup(path);
        if (last_slash)
            *name = strdup(last_slash + 1);
        else
            *name = strdup(path);
    }
    free(r_path);
}

bool create_directories_recursively(const char *path)
{
    enum source_type type = get_source_type(path);

    if (type == D)
        return true;

    if (type == F)
    {
        printf("Path %s is a file, choose a different path.\n\n", path);
        return false;
    }

    printf("Destination directory %s does not exist.\nWant to create it (y/n)? : ", path);
    char response = read_char();
    NL;
    if (response != 'y')
    {
        printf("Directory creation aborted. Exiting.\n\n");
        return false;
    }

    int len = strlen(path), i = 0;
    char *tmp_path = malloc(len + 1);
    tmp_path[len] = '\0';

    if (path[0] == '/')
    {
        i = 1;
        tmp_path[0] = '/';
    }

    for (; i < len; i++)
    {
        if (path[i] == '/')
        {
            tmp_path[i] = '\0';
            if (!make_dir(tmp_path))
            {
                free(tmp_path);
                return false;
            }
        }
        tmp_path[i] = path[i];

        if (i == len - 1)
        {
            if (!make_dir(tmp_path))
            {
                free(tmp_path);
                return false;
            }
        }
    }
    free(tmp_path);
    return true;
}

void read_string(char *buffer, size_t buffer_size)
{
    while (1) // Loop until we get valid input
    {
        if (fgets(buffer, buffer_size, stdin))
        {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n')
                buffer[len - 1] = '\0'; // remove newline

            // Check if string is not empty
            if (strlen(buffer) > 0)
                break; // Good input, exit loop
        }
        else
        {
            // Handle EOF or read error, just exit
            buffer[0] = '\0';
            break;
        }

        // If we're still here, the string was empty
        printf("Name cannot be empty. Please enter a new name: ");
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
    if (mkdir(path, 0777) == -1 && errno != EEXIST)
    {
        perror("Failed to create destination directory");
        return false;
    }
    return true;
}

void show_help_msg()
{
    printf(
        "Usage: safe_cp -s <source1> <source2> ... -d <destination_directory>\n\n"
        "Description:\n"
        "  A safe, interactive alternative to the cp command.\n\n"
        "Options:\n"
        "  -s <sources>          Specify one or more source files or directories.\n"
        "                        Each source can optionally include a rename using ':'\n"
        "                        Example: ./old.txt:new.txt  (renames old.txt to new.txt)\n\n"
        "  -d <destination>      Specify the destination directory.\n"
        "                        If the destination (or any parent folder) doesn't exist,\n"
        "                        it will be created automatically — like 'mkdir -p'.\n\n"
        "  -h, --help            Display this help message.\n\n"
        "Behavior:\n"
        "  • Copies both files and directories recursively.\n"
        "  • Converts relative paths (., .., ./, ../) to absolute.\n"
        "  • Automatically creates missing destination directories.\n"
        "  • Prompts before overwriting existing files or directories.\n"
        "  • Skips copying root directory '/' or copying parent into its child.\n"
        "  • Renamed new names must not contain ':'.\n\n"
        "Examples:\n"
        "  safe_cp -s ./a.txt ./b.txt -d ../backup\n"
        "  safe_cp -s ./dir1 ./dir2 -d /home/user/data\n"
        "  safe_cp -s ./photo.jpg:newname.jpg ./video.mp4 -d ./media\n"
        "  safe_cp -s ../docs -d ./backup\n\n");
}
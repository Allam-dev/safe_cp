#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h> // Required for exit

int main()
{

  struct stat source_state;
  char *cwd = realpath(".", NULL);
  char *parent_dir = realpath("..", NULL);
  printf("cwd: %s\n parent_dir: %s\n", cwd, parent_dir);
  return 0;
}
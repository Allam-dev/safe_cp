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
  if (mkdir("bbbbbbbbbbb/x/e/gf/w/e", 0777) == -1)
  {
    perror("mkdir failed");
  }
  if (stat("..", &source_state) != 0)
    printf("stat error: %s\n", strerror(errno));
  // the prefix S_I stands for “Status – Indication (of) permission bits”,
  // S_IRUSR; // I can use this as permission indicator for the owner but [perror] will clear that is permission denied
  if (S_ISREG(source_state.st_mode))
    printf("It's a file\n");
  else if (S_ISDIR(source_state.st_mode))
    printf("It's a directory\n");
  else
    printf("It's not a file or directory\n");

  char *cwd = realpath("drafts", NULL);
  char *parent_dir = realpath("/home/allam/", NULL);
  printf("cwd: %s\n parent_dir: %s\n", cwd, parent_dir);

  char *home = getenv("HOME");
  printf("home: %s\n", home);

  char* home2 = realpath("~", NULL);
  printf("home2: %s\n", home2);
  return 0;
}
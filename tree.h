#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *StackGetAt(char ***stack, int index) { return *((*stack) + index); }

char *StackPop(char ***stack, int *size) {
   char *topDir = (char *)malloc(sizeof(char) *
                                 (strlen(StackGetAt(stack, *size - 1)) + 1));
   if (topDir == NULL) {
      perror("Heap Full.");
      exit(1);
   }
   strcpy(topDir, StackGetAt(stack, *size - 1));
   free(StackGetAt(stack, *size - 1));
   *stack = (char **)realloc(*stack, sizeof(char *) * (*size - 1));
   *size = *size - 1;
   return topDir;
}

void StackPut(char ***stack, int *size, char newDir[]) {
   if (stack == NULL) {
      if (*size != 0) {
         printf("StackPut, invalid argument\n");
      }
      *stack = (char **)malloc(sizeof(char *));
   } else {
      *stack = (char **)realloc(*stack, sizeof(char *) * (*size + 1));
   }
   if (stack == NULL) {
      perror("Heap Full.");
      exit(1);
   }
   char *topDir = (char *)malloc(sizeof(char) * (strlen(newDir) + 1));
   if (topDir == NULL) {
      perror("Heap Full.");
      exit(1);
   }
   strcpy(topDir, newDir);
   *((*stack) + *size) = topDir;
   *size = *size + 1;
}

char *StackToDir(char ***stack, int *size) {
   int dirLength = 0;
   int i;
   for (i = 0; i < *size; i++) {
      dirLength += strlen(StackGetAt(stack, i)) + 1;
   }
   char *dir = (char *)malloc(sizeof(char) * (dirLength + 1));
   if (dir == NULL) {
      perror("Directory is to large to store in heap.");
   }
   int dirPosition = 0;
   for (i = 0; i < *size; i++) {
      strcpy(dir + dirPosition, StackGetAt(stack, i));
      dirPosition += strlen(StackGetAt(stack, i));
      strcpy(dir + dirPosition, "/");
      dirPosition++;
   }
   dir[dirPosition] = '\0';
   return dir;
}

void DirToStack(char ***stack, int *size, const char *dir) {
   int homeDirLength = strlen(dir) + 1;
   int i, folderStart, folderLength;
   for (i = 0, folderLength = 1, folderStart = 0; i < homeDirLength;
        i++, folderLength++) {
      if (dir[i] == '/' || dir[i] == '\0') {
         if (folderLength == 1) {
            continue;
         }
         if (dir[i] == '/') {
            folderLength--;
         }
         char folder[folderLength];
         memcpy(folder, dir + folderStart, folderLength);
         folder[folderLength] = '\0';
         StackPut(stack, size, folder);
         folderStart = i + 1;
         folderLength = 0;
      }
   }
}

// copied from SO, makes sense though :)
char CompareString(char *str1, char *str2) {
   while (*str1 != '\0' && tolower(*str1) == tolower(*str2)) {
      ++str1;
      ++str2;
   }
   return (tolower(*str1) - tolower(*str2));
}

char *PrintUntilDirectory(char *path, int depth, char *lastDir, char lSwitch,
                          char aSwitch) {
   DIR *dir = opendir(path);
   if (dir == NULL) {
      perror("error opening dir");
      exit(1);
   }
   struct dirent *dp;

   // get number of files
   int dpCount = 0;
   while ((dp = readdir(dir)) != NULL) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
         continue;
      }
      if (dp->d_name[0] == '.' && !aSwitch) {
         continue;
      }
      dpCount++;
   }

   // there is probably a better way of doing this
   closedir(dir);
   dir = opendir(path);

   // malloc space and fill it in with files
   struct dirent **dps =
       (struct dirent **)malloc(sizeof(struct dirent *) * dpCount);
   if (dps == NULL) {
      perror("Heap Full, folder too large.");
      exit(1);
   }
   int dpIndex = 0;
   while ((dp = readdir(dir)) != NULL) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
         continue;
      }
      if (dp->d_name[0] == '.' && !aSwitch) {
         continue;
      }
      dps[dpIndex] = dp;
      dpIndex++;
   }

   // alphabetize files with simple insertion sort
   for (dpIndex = 0; dpIndex < dpCount; dpIndex++) {
      int dpSortIndex;
      for (dpSortIndex = dpIndex; dpSortIndex > 0; dpSortIndex--) {
         if (CompareString(dps[dpSortIndex]->d_name,
                           dps[dpSortIndex - 1]->d_name) < 0) {
            struct dirent *tempDP = dps[dpSortIndex];
            dps[dpSortIndex] = dps[dpSortIndex - 1];
            dps[dpSortIndex - 1] = tempDP;
         }
      }
   }

   // finally, print out files occuring after lastdir
   char *newLastDir = NULL;
   for (dpIndex = 0; dpIndex < dpCount; dpIndex++) {
      dp = dps[dpIndex];
      if (lastDir != NULL) {
         if (CompareString(dp->d_name, lastDir) <= 0) {
            continue;
         }
      }
      int i;
      for (i = 0; i <= depth; i++) {
         if (i == depth) {
            printf("|-- ");
         } else {
            printf("|   ");
         }
      }
      if (lSwitch) {
         struct stat fileStat;
         int pathLength = strlen(path);
         char *filePath =
             malloc(sizeof(char) * (strlen(dp->d_name) + pathLength + 2));
         if (filePath == NULL) {
            perror("Directory is to large to store in heap.");
            exit(1);
         }
         strcpy(filePath, path);
         filePath[pathLength++] = '/';
         strcpy(filePath + pathLength, dp->d_name);
         if (lstat(filePath, &fileStat) < 0) {
            perror("This shouldn't happen.");
            exit(1);
         }
         free(filePath);
         printf("[");
         if (S_ISDIR(fileStat.st_mode)) {
            printf("d");
         } else if (S_ISLNK(fileStat.st_mode)) {
            printf("l");
         } else {
            printf("-");
         }
         printf((fileStat.st_mode & S_IRUSR) ? "r" : "-");
         printf((fileStat.st_mode & S_IWUSR) ? "w" : "-");
         printf((fileStat.st_mode & S_IXUSR) ? "x" : "-");
         printf((fileStat.st_mode & S_IRGRP) ? "r" : "-");
         printf((fileStat.st_mode & S_IWGRP) ? "w" : "-");
         printf((fileStat.st_mode & S_IXGRP) ? "x" : "-");
         printf((fileStat.st_mode & S_IROTH) ? "r" : "-");
         printf((fileStat.st_mode & S_IWOTH) ? "w" : "-");
         printf((fileStat.st_mode & S_IXOTH) ? "x" : "-");
         printf("] ");
      }
      printf("%s\n", dp->d_name);
      if (dp->d_type == DT_DIR) {
         newLastDir = (char *)malloc(sizeof(char) * (strlen(dp->d_name) + 1));
         if (newLastDir == NULL) {
            perror("Heap Full.");
            exit(1);
         }
         strcpy(newLastDir, dp->d_name);
         break;
      }
   }

   free(dps);
   closedir(dir);
   return newLastDir;
}

void PrintDirTree(char ***stack, int *size, char *lastDir, char lSwitch,
                  char aSwitch, int homeSize) {
   char *currentDir = StackToDir(stack, size);
   char *foundDir = PrintUntilDirectory(currentDir, *size - homeSize, lastDir,
                                        lSwitch, aSwitch);
   free(currentDir);
   if (foundDir != NULL) {
      StackPut(stack, size, foundDir);
      PrintDirTree(stack, size, NULL, lSwitch, aSwitch, homeSize);
      char *newLastDir = StackPop(stack, size);
      PrintDirTree(stack, size, newLastDir, lSwitch, aSwitch, homeSize);
      free(newLastDir);
      free(foundDir);
      return;
   }

   if (*size == homeSize) {
      while (*size > 0) {
         free(StackPop(stack, size));
      }
   }
}

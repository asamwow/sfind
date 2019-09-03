#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

char SearchSubString(const char *string, const char *subString) {
   int stringLength = strlen(string);
   int subLength = strlen(subString);
   if (subLength == 0) {
      return 1;
   }
   if (subLength > stringLength) {
      return 0;
   }
   int charIndex;
   for (charIndex = 0; charIndex < stringLength - subLength + 1; charIndex++) {
      int subIndex;
      for (subIndex = charIndex; subIndex < charIndex + subLength; subIndex++) {
         if (string[subIndex] != subString[subIndex - charIndex]) {
            break;
         }
         if (subIndex == charIndex + subLength - 1) {
            return 1;
         }
      }
   }
   return 0;
}

char *SearchUntilDirectory(char *path, int depth, char *lastDir,
                           const char *subName, int argc,
                           const char *execArgs[]) {
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
      dps[dpIndex] = dp;
      dpIndex++;
   }

   // alphabetize files with simple insertion sort
   for (dpIndex = 0; dpIndex < dpCount; dpIndex++) {
      int dpSortIndex;
      for (dpSortIndex = dpIndex; dpSortIndex > 0; dpSortIndex--) {
         if (strcmp(dps[dpSortIndex]->d_name, dps[dpSortIndex - 1]->d_name) <
             0) {
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
         if (strcmp(dp->d_name, lastDir) <= 0) {
            continue;
         }
      }

      if (dp->d_type == DT_DIR) {
         newLastDir = (char *)malloc(sizeof(char) * (strlen(dp->d_name) + 1));
         if (newLastDir == NULL) {
            perror("Heap Full.");
            exit(1);
         }
         strcpy(newLastDir, dp->d_name);
         break;
      }

      char *fileName = NULL;
      if (subName != NULL) {
         if (SearchSubString(dp->d_name, subName)) {
            fileName = dp->d_name;
         }
      } else {
         fileName = dp->d_name;
      }
      if (fileName != NULL && argc != 0) {
         pid_t childID = fork();
         if (childID < 0) {
            perror("fork");
            exit(1);
         }
         if (childID == 0) {
            int cmdSize = strlen(execArgs[0])+1;
            char *shellCMD = (char*)malloc(sizeof(char)*(cmdSize));
            if (shellCMD == NULL) {
               perror("malloc");
               exit(1);
            }
            strcpy(shellCMD, execArgs[0]);
            int cmdPosition = cmdSize-1;
            int argIndex;
            for (argIndex = 1; argIndex < argc; argIndex++) {
               if (!strcmp(execArgs[argIndex], "{}")) {
                  int pathSize = strlen(path);
                  cmdSize += strlen(fileName) + pathSize + 1;
                  shellCMD = realloc(shellCMD, sizeof(char)*(cmdSize));
                  if (shellCMD == NULL) {
                     perror("realloc");
                     exit(1);
                  }
                  shellCMD[cmdPosition++] = ' ';
                  strcpy(shellCMD+cmdPosition, path);
                  cmdPosition += pathSize;
                  strcpy(shellCMD+cmdPosition, fileName);
               } else {
                  cmdSize += strlen(execArgs[argIndex])+1;
                  shellCMD = realloc(shellCMD, sizeof(char)*(cmdSize));
                  if (shellCMD == NULL) {
                     perror("realloc");
                     exit(1);
                  }
                  shellCMD[cmdPosition++] = ' ';
                  strcpy(shellCMD+cmdPosition, execArgs[argIndex]);
               }
               cmdPosition = cmdSize - 1;
            }
            strcpy(shellCMD+cmdPosition, "\0");
            if (execl("/bin/sh", "sh", "-c", shellCMD, NULL) != 0) {
               perror("execl");
               exit(1);
            }
         } else {
            int wstatus;
            do {
               pid_t w = waitpid(childID, &wstatus, WUNTRACED | WCONTINUED);
               if (w == -1) {
                  perror("waitpid");
                  exit(1);
               }
            } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
         }
      } else {
         if (fileName != NULL) {
            printf("%s%s\n", path, fileName);
         }
      }
   }

   free(dps);
   closedir(dir);
   return newLastDir;
}
void SearchDirTree(char ***stack, int *size, char *lastDir, int homeSize,
                   const char *subName, int argc, const char *execArgs[]) {
   char *currentDir = StackToDir(stack, size);
   char *foundDir = SearchUntilDirectory(currentDir, *size - homeSize, lastDir,
                                         subName, argc, execArgs);
   free(currentDir);
   if (foundDir != NULL) {
      StackPut(stack, size, foundDir);
      SearchDirTree(stack, size, NULL, homeSize, subName, argc, execArgs);
      char *newLastDir = StackPop(stack, size);
      SearchDirTree(stack, size, newLastDir, homeSize, subName, argc, execArgs);
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

void SFind(const char *directory, const char *subName, int argc,
           const char *execArgs[]) {
   int size = 0;
   char **stack = NULL;
   struct stat fileStat;
   if (lstat(directory, &fileStat) < 0) {
      perror("sfind");
      exit(1);
   }
   if (S_ISDIR(fileStat.st_mode)) {
      DirToStack(&stack, &size, directory);
      SearchDirTree(&stack, &size, NULL, size, subName, argc, execArgs);
   } else {
      printf("%s\n", directory);
   }
}

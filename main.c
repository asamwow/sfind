#include <linux/limits.h>

#include "tree.h"

int main(int argc, const char* argv[]) {
   // this will break if their multiple redundant arguments
   int lSwitch = 0;
   int aSwitch = 0;
   int argIndex;
   int argCount = 0;
   for (argIndex = 1; argIndex < argc; argIndex++) {
      if (!strcmp(argv[argIndex], "-l")) {
         lSwitch = argIndex;
         argCount++;
      } else if (!strcmp(argv[argIndex], "-a")) {
         aSwitch = argIndex;
         argCount++;
      } else if (!strcmp(argv[argIndex], "-al")) {
         aSwitch = argIndex;
         lSwitch = argIndex;
         argCount++;
      } else if (!strcmp(argv[argIndex], "-la")) {
         aSwitch = argIndex;
         lSwitch = argIndex;
         argCount++;
      }
   }

   // directory stack variables
   int size = 0;
   char **stack = NULL;

   char cwd[PATH_MAX];
   getcwd(cwd, sizeof(cwd));
   
   // run in current dir
   if (argc - argCount <= 1) {
      printf("%s\n", cwd);
      DirToStack(&stack, &size, cwd);
      PrintDirTree(&stack, &size, NULL, lSwitch, aSwitch, size);
      return 0;
   }

   // run at dir from argument
   for (argIndex = 1; argIndex < argc; argIndex++) {
      if (argIndex == lSwitch || argIndex == aSwitch) {
         continue;
      }
      if (argv[argIndex][0] == '/') {
         printf("%s\n", argv[argIndex]);
         DirToStack(&stack, &size, argv[argIndex]);
      }  else {
         int cwdSize = strlen(cwd);
         int dirSize = cwdSize + strlen(argv[argIndex]) + 1;
         char appendedCWD[dirSize];
         strcpy(appendedCWD, cwd);
         appendedCWD[cwdSize] = '/';
         strcpy(appendedCWD+cwdSize+1, argv[argIndex]);
         printf("%s\n", appendedCWD);
         DirToStack(&stack, &size, appendedCWD);
      }
      PrintDirTree(&stack, &size, NULL, lSwitch, aSwitch, size);
   }
}

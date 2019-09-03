#include <linux/limits.h>

#include "sfind.h"

int main(int argc, const char* argv[]) {
   if (argc < 2) {
      SFind(".", NULL, 0, NULL);
      return 0;
   }
   int execIndex = -1;
   int nameIndex = -1;
   int argIndex;
   for (argIndex = 1; argIndex < argc; argIndex++) {
      if (argIndex == nameIndex + 1 || argIndex == execIndex + 1) {
         continue;
      }
      if (!strcmp(argv[argIndex], "-exec")) {
         execIndex = argIndex;
      }
      if (!strcmp(argv[argIndex], "-name")) {
         nameIndex = argIndex;
      }
   }
   if (execIndex == argc - 1) {
      fprintf(stderr, "sfind: missing argument to `-exec'\n");
      return 1;
   }
   if (nameIndex == argc - 1) {
      fprintf(stderr, "sfind: missing argument to `-name'\n");
      return 1;
   }
   char filePath[PATH_MAX];
   if (execIndex == 1 || nameIndex == 1) {
      strcpy(filePath, ".\0");
   } else {
      strcpy(filePath, argv[1]);
   }
   if (execIndex == -1) {
      if (nameIndex == -1) {
         SFind(filePath, NULL, 0, NULL);
      } else {
         SFind(filePath, argv[nameIndex+1], 0, NULL);
      }
   } else {
      for (argIndex = execIndex+1; argIndex < argc; argIndex++) {
         //printf("%s\n", argv[argIndex]);
         if (!strcmp(argv[argIndex], ";")) {
            break;
         }
         if (argIndex == argc - 1) {
            fprintf(stderr, "sfind: missing argument to `-exec'\n");
            return 1;
         }
      }
      int execArgCount = argIndex - execIndex - 1;
      if (nameIndex == -1) {
         SFind(filePath, NULL, execArgCount, argv+execIndex+1);
      } else {
         SFind(filePath, argv[nameIndex+1], execArgCount, argv+execIndex+1);
      }
   }
   
   
}

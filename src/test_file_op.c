#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int found_flag = 0;
//int cmp_str(char *str1, char *str2);

int main()
{
  FILE *fp;
  size_t len = 0, read;
  char *line = NULL;
  char *check_file = "file2";
  fp = fopen("db.txt", "w");
  if (fp == NULL)
  {
	perror("File could not be opened\n");
	exit(EXIT_FAILURE);
  }
  char *to_write = (char *) malloc(sizeof(4096)); //PATH_MAX = 4096
  strcpy(to_write,"file1"); 
  fprintf(fp, "%s\n", to_write);
  strcpy(to_write, "file2");
  fprintf(fp, "%s\n", to_write);
  strcpy(to_write, "file3");
  fprintf(fp, "%s\n", to_write);
  fclose(fp);

  fp = fopen("db.txt", "r");
  if (fp == NULL)
  {
	perror("File could not be opened\n");
        exit(EXIT_FAILURE);
  }
  while ((read = getline(&line, &len, fp)) != -1) {
	printf("Line Length: %d, Line: %s", read, line);
        printf("Check file: %s\n", check_file);
        line[read-1] = '\0';
	if (!strcmp(check_file, line))
	{
		found_flag = 1;
		printf("Found flag: %d\n", found_flag);
		break;
	}
  }
  fclose(fp);
  if(line)
	free(line);
  exit(EXIT_SUCCESS);
} 

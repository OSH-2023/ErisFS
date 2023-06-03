#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* normalize path by removing . and .. in  the path */
/* normalize the combination of directory and file name if filename[0] != '/' */
/* normalize only the filename if filename[0] == '/' */
char *efs_normalize_path(const char *directory, const char *filename)
{
    char *fullpath;
    char *dst0, *dst, *src;


    if ((directory == NULL) && (filename[0] != '/'))
    {
        printf("directory is NULL and filename is not absolute path!\n");
        return NULL;
    }

    if (filename[0] != '/') /* relative path, join it with directory */
    {
        fullpath = (char *)malloc(strlen(directory) + strlen(filename) + 2);

        if (fullpath == NULL)
            return NULL;

        /* join path and file name */
        strcpy(fullpath, directory);
        strcat(fullpath, "/");
        strcat(fullpath, filename);
        strcat(fullpath, "\0");
    }
    else /* absolute path, use it directly */
    {
        strcpy(fullpath, filename); 

        if (fullpath == NULL)
            return NULL;
    }
    int len = strlen(fullpath);
    char* normalized = (char*)malloc((len + 1) * sizeof(char));

    int i = 0; // current position in the path
    int j = 0; // current position in the normalized path

    while (fullpath[i] != '\0') 
    {
        if (fullpath[i] == '/') 
        {
            // handle consecutive '/'
            while (fullpath[i + 1] == '/') 
            {
                i++;
            }

            if (fullpath[i + 1] == '.' && fullpath[i + 2] == '.' && (fullpath[i + 3] == '/' || fullpath[i + 3] == '\0')) 
            {
                // handle '..'
                while (normalized[j - 1] != '/') 
                {
                    j--; // go back to the previous '/'
                    if(j == 0) 
                    {
                        printf("[efs.c] efs_normalize_path is failed! invalid path!\n");
                        return NULL;
                    }
                }
                if (j > 0) 
                {
                    j--;
                }
                i += 3; //jump to next '/'
            }
            else if (fullpath[i + 1] == '.' && (fullpath[i + 2] == '/' || fullpath[i + 2] == '\0')) 
            {
                // handle '.' 
                i += 2; //jump to next '/'
            }
            else 
            {
                normalized[j++] = '/';
                i++;
            }
        }
        else 
        {
            normalized[j++] = fullpath[i++];
        }
    }

    // terminate the normalized path
    normalized[j] = '\0';
    return normalized;
}

int main(){
    //test efs_normalize_path
    char *directory = "/home";
    char *filename = ".././././/less///.././/less/.//test.c";
    char *fullpath = efs_normalize_path(directory, filename);
    printf("fullpath: %s\n", fullpath);
    return 0;
}
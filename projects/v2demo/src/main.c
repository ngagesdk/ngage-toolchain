#include <stdlib.h>
#include <stdio.h>

int __gccmain(int argc, char* argv[])
{
    FILE* file = fopen("E:\\hello.txt", "w");
    if (NULL == file)
    {
        return 1;
    }

    for (int i = 0; i < 100; i++)
    {
        fprintf(file, "Hello, world! %d\n", i);
    }
    fclose(file);

    return 0;
}

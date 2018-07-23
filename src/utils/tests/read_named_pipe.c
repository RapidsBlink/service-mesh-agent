#include "../log.h"
#include <unistd.h>
int main(int argc, char** argv){
    int messages = 0;
    char buf[1024];

    float eden_ratio;

    while (1)
    {
        FILE *myStream = fopen(argv[1], "r");
        if (myStream == NULL)
        {
            log_fatal("open named pipe failed");
            return 1;
        }
        while (fgets(buf, sizeof(buf), myStream) != 0)
        {
            if(buf[0] == 'T' || buf[0] == 'S')
                continue;
            sscanf(buf, "%*s%*s%*s%f", &eden_ratio);
            printf("%s\n", buf);
            printf("eden ratio %f\n", eden_ratio);
            messages++;
        }
        fclose(myStream);
        sleep(1);
    }

    return 0;
}
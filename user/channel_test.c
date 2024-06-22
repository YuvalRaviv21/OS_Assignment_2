#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main()
{
    int cd = channel_create();
    if (cd < 0)
    {
        printf("Failed to create channel\n");
        exit(1);
    }
    int pid = fork();
    if (pid == 0)
    {
        if (channel_put(cd, 42) < 0)
        {
            printf("\tFailed to put data in channel\n");
            exit(1);
        }
        if(channel_put(cd, 43))
        {
            printf("\tFailed to put data in channel\n");
            exit(1);
        }
        if (channel_destroy(cd))
        {
            printf("\tFailed to destroy channel\n");
            exit(1);
        }
    }
    else
    {
        int data;
        if (channel_take(cd, &data) < 0 || data != 42)                  // should go to sleep and wait for data because channel is created by main
        { // 42
            printf("Failed to take data(42) from channel\n");
            exit(1);
        }
        if (channel_take(cd, &data) < 0 || data != 43)                  // should go to sleep and wait for data because channel is created by main
        {
            printf("Failed to take data(43) from channel\n");
            exit(1);
        }
        if (channel_take(cd, &data) >= 0)                               // should return -1 because channel is destroyed by child after two puts
        {
            printf("Unexpected data from channel\n");
            exit(1);
        }
    }
    exit(0);
}
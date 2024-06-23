#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_PRIMES 100
#define DEFAULT_NUM_CHECKERS 3

int is_prime(int n)
{
    if (n < 2)
    {
        return 0;
    }
    for (int i = 2; i * i <= n; i++)
    {
        if (n % i == 0)
        {
            return 0;
        }
    }
    return 1;
}


void generator(int generator_channel) {
    int num = 2;
    while (1) {
        if (channel_put(generator_channel, num) < 0) {
            break; // Just exit without printing
        }
        num++;
    }
}

void checker(int id, int generator_channel, int printer_channel) {
    int num;
    while (1) {
        if (channel_take(generator_channel, &num) < 0) {
            sleep(1); // Delay to prevent message overlap
            if (channel_destroy(generator_channel) < 0) {
                printf("generator_channel destroyed by checker %d, pid %d\n", id, getpid());
            }
            printf("checker %d, pid %d exit\n", id, getpid());
            sleep(1); // Delay to ensure message is not mixed up
            exit(0);
        }
        if (is_prime(num)) {
            if (channel_put(printer_channel, num) < 0) {
                sleep(3); // Delay to prevent message overlap
                if (channel_destroy(generator_channel) == 0) {
                    printf("generator_channel destroyed by checker %d, pid %d\n", id, getpid());
                }
                printf("checker %d, pid %d exit\n", id, getpid());
                sleep(1); // Delay to ensure message is not mixed up
                exit(0);
            }
        }
    }
}

void printer(int printer_channel) {
    int num;
    int primes_found = 0;
    while (primes_found < MAX_PRIMES) {
        if (channel_take(printer_channel, &num) < 0) {
            sleep(1); // Delay to prevent message overlap
            exit(0);
        }
        printf("Prime %d: %d\n", primes_found + 1, num);
        primes_found++;
    }
    sleep(3); // Delay to prevent message overlap
    if (channel_destroy(printer_channel) == 0) {
        printf("printer_channel destroyed by printer, pid %d\n", getpid());
    }
    printf("printer exit, pid %d\n", getpid());
    sleep(1); // Delay to ensure the message is printed before exiting
    exit(0);
}

void wait_for_children(int num_checkers) {
    for (int i = 0; i < num_checkers + 1; i++) {  // +1 for the printer
        wait(0);
    }
}

int main(int argc, char *argv[]) {
    int num_checkers = (argc > 1) ? atoi(argv[1]) : DEFAULT_NUM_CHECKERS;

    while (1) {
        int generator_channel = channel_create();
        int printer_channel = channel_create();
        if (generator_channel < 0 || printer_channel < 0) {
            printf("Failed to create channels\n");
            exit(1);
        }

        int pid;
        for (int i = 0; i < num_checkers; i++) {
            if ((pid = fork()) == 0) {
                sleep(1); // Delay to prevent message overlap
                checker(i + 1, generator_channel, printer_channel);
                exit(0);
            }
            printf("\033[34mchecker pid %d\n\033[0m",pid);
        }

        if ((pid = fork()) == 0) {
            sleep(1); // Delay to prevent message overlap
            printer(printer_channel);
            exit(0);
        }
        printf("\033[32mprinter pid %d\n\033[0m",pid);
        // Wait for all child processes to exit
        generator(generator_channel);
        wait_for_children(num_checkers+1);

        sleep(1); 
        // Prompt the user to restart the system
        char buffer[10];
        printf("Restart the system? (y/n): ");
        gets(buffer, sizeof(buffer));
        if (buffer[0] != 'y' && buffer[0] != 'Y') {
            printf("shutdown by main/generetor procced %d pid\n",getpid());
            printf("System exit.\n");
            break;
        }
        // Wait a moment before restarting to prevent message overlap
        sleep(1);
    }

    return 0;
}
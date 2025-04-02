#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    uint64 ret_val1 = getpid();
    uint64 ret_val2 = getppid();
    printf("My student ID is 2021070173\n");
    printf("My pid is %ld\n", ret_val1);
    printf("My ppid is %ld\n", ret_val2);
    exit(0);
}
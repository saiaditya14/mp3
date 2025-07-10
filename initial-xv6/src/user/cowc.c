#include "kernel/types.h"
#include "kernel/memlayout.h"
#include "user/user.h"

// Test function for read-only access
void read_only_test(int iterations) {
    printf("Read-only test: ");
    int sz = 4096 * 10;
    char *p = sbrk(sz);
    if (p == (char*)-1) {
        printf("sbrk failed\n");
        exit(-1);
    }

    // Initialize memory
    for (char *q = p; q < p + sz; q += 4096) {
        *q = 1;
    }

    int start_faults = cowcamp();
    for (int i = 0; i < iterations; i++) {
        int pid = fork();
        if (pid == 0) {
            for (char *q = p; q < p + sz; q += 4096) {
                int x = *q; // Read without modifying
                (void)x;
            }
            exit(0);
        } else {
            wait(0);
        }
    }

    int end_faults = cowcamp();
    printf("COW page faults during read-only test: %d\n", end_faults - start_faults);
}

// Full write test: child writes to all allocated pages
void full_write_test(int iterations) {
    printf("Full write test: ");
    int sz = 4096 * 10;
    char *p = sbrk(sz);
    if (p == (char*)-1) {
        printf("sbrk failed\n");
        exit(-1);
    }

    for (char *q = p; q < p + sz; q += 4096) {
        *q = 1;
    }

    int start_faults = cowcamp();
    for (int i = 0; i < iterations; i++) {
        int pid = fork();
        if (pid == 0) {
            for (char *q = p; q < p + sz; q += 4096) {
                *q = 2;
            }
            exit(0);
        } else {
            wait(0);
        }
    }

    int end_faults = cowcamp();
    printf("COW page faults during full write test: %d\n", end_faults - start_faults);
}

// Partial write test: child writes to half the allocated pages
void partial_write_test(int iterations) {
    printf("Partial write test: ");
    int sz = 4096 * 10;
    char *p = sbrk(sz);
    if (p == (char*)-1) {
        printf("sbrk failed\n");
        exit(-1);
    }

    for (char *q = p; q < p + sz; q += 4096) {
        *q = 1;
    }

    int start_faults = cowcamp();
    for (int i = 0; i < iterations; i++) {
        int pid = fork();
        if (pid == 0) {
            for (char *q = p; q < p + sz / 2; q += 4096) {
                *q = 2;
            }
            exit(0);
        } else {
            wait(0);
        }
    }

    int end_faults = cowcamp();
    printf("COW page faults during partial write test: %d\n", end_faults - start_faults);
}

// Sparse write test: child writes to only the first page
void sparse_write_test(int iterations) {
    printf("Sparse write test: ");
    int sz = 4096 * 10;
    char *p = sbrk(sz);
    if (p == (char*)-1) {
        printf("sbrk failed\n");
        exit(-1);
    }

    for (char *q = p; q < p + sz; q += 4096) {
        *q = 1;
    }

    int start_faults = cowcamp();
    for (int i = 0; i < iterations; i++) {
        int pid = fork();
        if (pid == 0) {
            *p = 2;  // Write to only the first page
            exit(0);
        } else {
            wait(0);
        }
    }

    int end_faults = cowcamp();
    printf("COW page faults during sparse write test: %d\n", end_faults - start_faults);
}

int main(int argc, char *argv[]) {
    int iterations = 1;  // You can adjust the number of iterations for a larger sample size
    read_only_test(iterations);
    full_write_test(iterations);
    partial_write_test(iterations);
    sparse_write_test(iterations);
    exit(0);
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILE_NAME_SIZE 128
#define MAX_ID 100000
#define MAX_TIMESTAMP_SIZE 32140810
#define MAX_NAME_COMBINATIONS 308915776
int MAX_THREADS;
int compType;

typedef struct file{
    char* name;
    int id;
    char* timestamp;
} file;
typedef struct data{
    file** arr;
    int left;
    int right;
    int depth;
    file** count;
    long long* hash;
} data;

void getMaxThreads();
void display(file* arr[], int n);
void distributed_merge_sort(file *arr[], int n);
void *merge_sort(void *arg);
void merge(file *arr[], int left, int mid, int right);
bool comp(file *a, file *b);
void distributed_count_sort(file *arr[], int n);
void *count_sort_on_range(void *arg);

int main() {
    getMaxThreads();
    int n;
    scanf("%d", &n);
    file* arr[n];
    for(int i = 0; i < n; i++) {
        arr[i] = (file*)malloc(sizeof(file));
        arr[i]->name = (char*)malloc(MAX_FILE_NAME_SIZE * sizeof(char));
        arr[i]->timestamp = (char*)malloc(100 * sizeof(char));
        scanf("%s %d %s", arr[i]->name, &arr[i]->id, arr[i]->timestamp);
    }

    char type[10];
    scanf("%s", type);
    if(strcmp(type, "ID") == 0) {
        compType = 0;
    } else if(strcmp(type, "Name") == 0) {
        compType = 1;
    } else {
        compType = 2;
    }
    printf("%s\n", type);

    if(n > 42) {
        distributed_merge_sort(arr, n);
    } else {
        distributed_count_sort(arr, n);
    }

    display(arr, n);
}

void getMaxThreads() {
    MAX_THREADS = sysconf(_SC_NPROCESSORS_ONLN);
}

void display(file* arr[], int n) {
    for(int i = 0; i < n; i++) {
        printf("%s %d %s\n", arr[i]->name, arr[i]->id, arr[i]->timestamp);
    }
}

void distributed_merge_sort(file *arr[], int n) {
    // printf("Distributed Merge Sort\n");
    data first;
    first.arr = arr;
    first.left = 0;
    first.right = n - 1;
    first.depth = 0;
    first.count = NULL;
    first.hash = NULL;

    merge_sort(&first);
}

void *merge_sort(void *arg) {
    data* d = (data*)arg;
    if(d->left >= d->right) {
        return NULL;
    }

    int mid = (d->left + d->right) / 2;
    data left, right;
    left.arr = d->arr;
    left.left = d->left;
    left.right = mid;
    left.depth = d->depth + 1;
    left.count = d->count;
    left.hash = NULL;
    right.arr = d->arr;
    right.left = mid + 1;
    right.right = d->right;
    right.depth = d->depth + 1;
    right.count = d->count;
    right.hash = NULL;

    int cur_threads = 1 << d->depth;
    if(cur_threads < MAX_THREADS) {
        pthread_t thread;
        pthread_create(&thread, NULL, merge_sort, &left);
        merge_sort(&right);
        pthread_join(thread, NULL);
    } else {
        merge_sort(&left);
        merge_sort(&right);
    }
    merge(d->arr, d->left, mid, d->right);
    return NULL;
}

bool comp(file *a, file *b) {
    if(compType == 0) {
        return a->id < b->id;
    } else if(compType == 1) {
        return strcmp(a->name, b->name) < 0;
    } else {
        return strcmp(a->timestamp, b->timestamp) < 0;
    }
}

void merge(file *arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    file* L[n1];
    file* R[n2];
    for(int i = 0; i < n1; i++) {
        L[i] = arr[left + i];
    }
    for(int i = 0; i < n2; i++) {
        R[i] = arr[mid + 1 + i];
    }
    int idx1 = 0, idx2 = 0, idx = left;
    while(idx1 < n1 && idx2 < n2) {
        if(comp(L[idx1], R[idx2])) {
            arr[idx] = L[idx1];
            idx1++;
        } else {
            arr[idx] = R[idx2];
            idx2++;
        }
        idx++;
    }
    while(idx1 < n1) {
        arr[idx] = L[idx1];
        idx1++;
        idx++;
    }
    while(idx2 < n2) {
        arr[idx] = R[idx2];
        idx2++;
        idx++;
    }
}

void distributed_count_sort(file *arr[], int n) {
    int len;
    long long hash[n];
    if(compType == 0) {
        int max_id = 0;
        for(int i = 0; i < n; i++) {
            hash[i] = arr[i]->id;
            if(hash[i] > max_id) {
                max_id = hash[i];
            }
        }
        len = max_id + 1;
        if(max_id > MAX_ID) {
            distributed_merge_sort(arr, n);
            return;
        }
    } else if(compType == 1) {
        int max_name = 0;
        for(int i = 0; i < n; i++) {
            hash[i] = 0;
            long long p = 26 * 26 * 26 * 26 * 26;
            int idx = 0;
            if(strlen(arr[i]->name) > 6) {
                distributed_merge_sort(arr, n);
                return;
            }
            while(arr[i]->name[idx] != '\0') {
                hash[i] += 1LL * (arr[i]->name[idx] - 'a') * p;
                p /= 26;
                idx++;
            }
            if(hash[i] > max_name) {
                max_name = hash[i];
            }
        }
        len = max_name + 1;
    } else {
        int max_timestamp = 0;
        int year = (arr[0]->timestamp[0] - '0') * 1000 + (arr[0]->timestamp[1] - '0') * 100 + (arr[0]->timestamp[2] - '0') * 10 + (arr[0]->timestamp[3] - '0');
        for(int i = 0; i < n; i++) {
            if(year != ((arr[i]->timestamp[0] - '0') * 1000 + (arr[i]->timestamp[1] - '0') * 100 + (arr[i]->timestamp[2] - '0') * 10 + (arr[i]->timestamp[3] - '0'))) {
                distributed_merge_sort(arr, n);
                return;
            }
            hash[i] = 0;
            long long month = ((arr[i]->timestamp[5] - '0') * 10 + (arr[i]->timestamp[6] - '0')) * 31 * 24 * 60;
            long long day = ((arr[i]->timestamp[8] - '0') * 10 + (arr[i]->timestamp[9] - '0')) * 24 * 60;
            long long hour = ((arr[i]->timestamp[11] - '0') * 10 + (arr[i]->timestamp[12] - '0')) * 60;
            long long minute = ((arr[i]->timestamp[14] - '0') * 10 + (arr[i]->timestamp[15] - '0'));
            hash[i] = (month + day + hour + minute);
            if(hash[i] > max_timestamp) {
                max_timestamp = hash[i];
            }
        }
        len = max_timestamp + 1;
    }

    file** count;
    count = (file**)malloc(len * sizeof(file*));
    for(int i = 0; i < len; i++) {
        count[i] = NULL;
    }
    if(n <= MAX_THREADS) {
        MAX_THREADS = n / 2;
    }
    pthread_t threads[MAX_THREADS];
    for(int i = 0; i < MAX_THREADS; i++) {
        data* d = (data*)malloc(sizeof(data));
        d->arr = arr;
        d->left = i * n / (MAX_THREADS + 1);
        d->right = (i + 1) * n / (MAX_THREADS + 1) - 1;
        d->depth = 0;
        d->count = count;
        d->hash = hash;
        pthread_create(&threads[i], NULL, count_sort_on_range, d);
    }
    data* d = (data*)malloc(sizeof(data));
    d->arr = arr;
    d->left = MAX_THREADS * n / (MAX_THREADS + 1);
    d->right = n - 1;
    d->depth = 0;
    d->count = count;
    d->hash = hash;
    count_sort_on_range(d);
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    int idx = 0;
    for(int i = 0; i < len; i++) {
        if(count[i] != NULL) {
            arr[idx] = count[i];
            idx++;
        }
    }
}

void *count_sort_on_range(void *arg) {
    data* d = (data*)arg;
    for(int i = d->left; i <= d->right; i++) {
        if(d->count[d->hash[i]] == NULL) {
            d->count[d->hash[i]] = d->arr[i];
        }
    }
    return NULL;
}

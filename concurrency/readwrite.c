#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#define limit 2000
#define tes 3

int r,w,d;
int n,c,t;

typedef struct file {
    sem_t del;
    pthread_mutex_t users;
    int us;
    pthread_cond_t user;
    sem_t conc;
    sem_t write;
}file;

typedef struct query {
    int id;
    int storefile;
    file* file;
    int op;
    int t;
}query;

int mutex_timedlock1(pthread_mutex_t *mutex, const struct timespec *timeout) {
    struct timespec now;
    while (1) {
        clock_gettime(CLOCK_REALTIME, &now);
        if (now.tv_sec > timeout->tv_sec || (now.tv_sec == timeout->tv_sec && now.tv_nsec >= timeout->tv_nsec)) {
            return -1;
        }
        if (pthread_mutex_trylock(mutex) == 0) {
            return 0;
        }
        usleep(50000); // Sleep for 0.05 seconds
    }
}

int sem_timedwait1(sem_t *sem, const struct timespec *timeout) {
    struct timespec now;
    //sleep(8);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &now);
        //printf("%d %d\n",timeout->tv_sec,now.tv_sec);
        if (now.tv_sec > timeout->tv_sec || (now.tv_sec == timeout->tv_sec && now.tv_nsec >= timeout->tv_nsec)) {
            errno = ETIMEDOUT;
            return -1;
        }
        if (sem_trywait(sem) == 0) {
            return 0;
        }
        usleep(50000); // Sleep for 0.05 seconds
    }
}

int cmp(const void* a, const void* b) {
    const query* queryA = (const query*)a;
    const query* queryB = (const query*)b;

    if (queryA->t != queryB->t) {
        return queryA->t - queryB->t;
    } else {
        return queryA->op - queryB->op;
    }
}


void* read1(void* args) {
    query* q = (query*)args;
    file* use = q->file;
    struct timespec entry;
    clock_gettime(CLOCK_REALTIME, &entry);
    struct timespec timeout;
    timeout.tv_sec = entry.tv_sec + t;
    timeout.tv_nsec = 0;

    if (pthread_mutex_timedlock(&use->users, &timeout) == -1) {
        printf("\033[31mUser %d canceled the request due to no response at %d seconds\033[0m\n", q->id, q->t + t);
        return NULL;
    }
    use->us++;
    pthread_mutex_unlock(&use->users);
    sleep(1);

    if (sem_timedwait(&use->conc, &timeout) == -1) {
        printf("\033[31mUser %d canceled the request due to no response at %d seconds\033[0m\n", q->id, q->t + t);
        pthread_mutex_lock(&use->users);
        use->us--;
        if (use->us == 0) {
            pthread_cond_signal(&use->user);
        }
        pthread_mutex_unlock(&use->users);
        return NULL;
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    int currentsecs = now.tv_sec - entry.tv_sec;
    printf("\033[35mLAZY has taken up the read request of User %d at %d seconds\033[0m\n", q->id, q->t + currentsecs);
    sleep(r);
    printf("\033[32mThe read request for User %d was completed at %d seconds\033[0m\n", q->id, q->t + currentsecs + r);
    pthread_mutex_lock(&use->users);
    use->us--;
    if (use->us == 0) {
        pthread_cond_signal(&use->user);
    }
    pthread_mutex_unlock(&use->users);
    sem_post(&use->conc);
    return NULL;
}

void* write1(void* args) {
    query* q = (query*)args;
    file* use = q->file;
    struct timespec entry;
    clock_gettime(CLOCK_REALTIME, &entry);
    struct timespec timeout;
    timeout.tv_sec = entry.tv_sec + t;
    timeout.tv_nsec = 0;

    if (pthread_mutex_timedlock(&use->users, &timeout) == -1) {
        printf("\033[31mUser %d canceled the request due to no response at %d seconds\033[0m\n", q->id, q->t + t);
        return NULL;
    }
    use->us++;
    pthread_mutex_unlock(&use->users);
    sleep(1);

    if (sem_timedwait(&use->write, &timeout) == -1) {
        printf("\033[31mUser %d canceled the request due to no response at %d seconds\033[0m\n", q->id, q->t + t);
        pthread_mutex_lock(&use->users);
        use->us--;
        if (use->us == 0) {
            pthread_cond_signal(&use->user);
        }
        pthread_mutex_unlock(&use->users);
        return NULL;
    }

    if (sem_timedwait(&use->conc, &timeout) == -1) {
        printf("\033[31mUser %d canceled the request due to no response at %d seconds\033[0m\n", q->id, q->t + t);
        pthread_mutex_lock(&use->users);
        use->us--;
        if (use->us == 0) {
            pthread_cond_signal(&use->user);
        }
        pthread_mutex_unlock(&use->users);
        sem_post(&use->write);
        return NULL;
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    int currentsecs = now.tv_sec - entry.tv_sec;
    printf("\033[35mLAZY has taken up the write request of User %d at %d seconds\033[0m\n", q->id, q->t + currentsecs);
    sleep(w);
    printf("\033[32mThe write request for User %d was completed at %d seconds\033[0m\n", q->id, q->t + currentsecs + w);
    pthread_mutex_lock(&use->users);
    use->us--;
    if (use->us == 0) {
        pthread_cond_signal(&use->user);
    }
    pthread_mutex_unlock(&use->users);
    sem_post(&use->conc);
    sem_post(&use->write);
    return NULL;
}

void* delete1(void* args) {
    query* q = (query*)args;
    file* use = q->file;

    pthread_mutex_lock(&use->users);

    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += t;
    sleep(1);

    while (use->us > 0) {
        int ret = pthread_cond_timedwait(&use->user, &use->users, &timeout);
        if (ret == ETIMEDOUT) {
            printf("\033[31mUser %d canceled their request due to no response at %d seconds\033[0m\n", q->id, q->t + t);
            pthread_mutex_unlock(&use->users);
            return NULL;
        }
    }
    sem_wait(&use->del);
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    int currentsecs = now.tv_sec - timeout.tv_sec + t;
    printf("\033[35mLAZY has taken up delete request of User %d at %d seconds\033[0m\n", q->id, q->t + currentsecs);

    sleep(d);

    printf("\033[32mThe delete request for User %d was completed at %d seconds\033[0m\n", q->id, q->t + currentsecs + d);

    return NULL;
}

int main() {

    scanf("%d %d %d", &r, &w, &d);
    scanf("%d %d %d", &n, &c, &t);

    query query_stack[limit];
    int query_count = 0;
    char res[7];

    while (1) {
        scanf("%s", res);
        if (strcmp(res, "STOP") == 0) break;

        query_stack[query_count].id = atoi(res);
        scanf("%d", &query_stack[query_count].storefile);
        scanf("%s", res);
        if (res[0] == 'R') query_stack[query_count].op = 1;
        else if (res[0] == 'W') query_stack[query_count].op = 2;
        else if (res[0] == 'D') query_stack[query_count].op = 3;
        else query_stack[query_count].op = -1;
        scanf("%d", &query_stack[query_count].t);
        query_count++;
    }
    printf("\nLAZY has woken up!\n\n");
    query queries[query_count];
    int skip[query_count];
    file f[n + 1];
    
    for (int i = 0; i <= n; i++) {
        sem_init(&f[i].del, 0, 1);
        pthread_mutex_init(&f[i].users, NULL);
        pthread_cond_init(&f[i].user, NULL);
        sem_init(&f[i].conc, 0, c);
        sem_init(&f[i].write, 0, 1);
        f[i].us = 0;
    }
    
    for (int i = 0; i < query_count; i++) {
        skip[i] = 0;
        queries[i] = query_stack[i];
        if(queries[i].storefile<=n)queries[i].file = &f[queries[i].storefile];
    }

    qsort(queries, query_count, sizeof(query), cmp);

    pthread_t threads[query_count];
    int last_executed_time = 0;

    for (int i = 0; i < query_count; i++) {
        int sleep_duration = queries[i].t - last_executed_time;
        if (sleep_duration > 0) {
            sleep(sleep_duration);
        }

        if(queries[i].storefile>n){
            printf("\033[31mERROR: User %d was declined at %d seconds due to requesting an invalid file\033[0m\n",queries[i].id,queries[i].t);
            last_executed_time = queries[i].t;
            skip[i] = 1;
            continue;
        }

        if (sem_trywait(&queries[i].file->del) != 0) {
            printf("\033[37mUser %d has their request declined at %d seconds as the requested file was deleted\033[0m\n", queries[i].id, queries[i].t);
            last_executed_time = queries[i].t;
            skip[i] = 1;
            continue;
        }
        sem_post(&queries[i].file->del);

        if (queries[i].op == 1 && queries[i].storefile <= n) printf("\033[33mUser %d has made a request for performing READ operation on file %d at %d seconds\033[0m\n", queries[i].id, queries[i].storefile, queries[i].t);
        else if (queries[i].op == 2 && queries[i].storefile <= n) printf("\033[33mUser %d has made a request for performing WRITE operation on file %d at %d seconds\033[0m\n", queries[i].id, queries[i].storefile, queries[i].t);
        else if (queries[i].op == 3 && queries[i].storefile <= n) printf("\033[33mUser %d has made a request for performing DELETE operation on file %d at %d seconds\033[0m\n", queries[i].id, queries[i].storefile, queries[i].t);

        if (queries[i].op == 1) pthread_create(&threads[i], NULL, read1, &queries[i]);
        else if (queries[i].op == 2) pthread_create(&threads[i], NULL, write1, &queries[i]);
        else if (queries[i].op == 3) pthread_create(&threads[i], NULL, delete1, &queries[i]);
        else if (queries[i].op < 0){
            printf("\033[31mERROR: User %d requested an unsupported operation\033[0m\n",queries[i].id);
            skip[i] = 1;
        }
        else if (queries[i].storefile > n) printf("\033[31mERROR: User requested an invalid file\033[0m\n");

        last_executed_time = queries[i].t;
    }

    for (int i = 0; i < query_count; i++) {
        if(!skip[i])pthread_join(threads[i], NULL);
    }

    for (int i = 0; i <= n; i++) {
        sem_destroy(&f[i].del);
        pthread_mutex_destroy(&f[i].users);
        pthread_cond_destroy(&f[i].user);
        sem_destroy(&f[i].conc);
        sem_destroy(&f[i].write);
    }

    printf("\nLAZY has no more pending requests is going back to sleep!\n");
    return 0;
}
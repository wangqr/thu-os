#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define N 1000000
#define WORKER_NUM 20
#define MAX_THRESH 1000

uint32_t data[N];

pthread_t worker[WORKER_NUM];

struct queue_item {
    uint32_t *start;
    size_t length;
} queue[WORKER_NUM + 2*(N-MAX_THRESH)],
    *queue_head = queue, *queue_end = queue + 1;
size_t task_remain = 1;
pthread_mutex_t mutex_remain, mutex_queue_read, mutex_queue_write;
sem_t sem_queue;

int uint_comp(const void *a, const void *b) {
    const uint32_t *pa = (const uint32_t *) a, *pb = (const uint32_t *) b;
    return (*pa < *pb) ? -1 : (*pa > *pb);
}

void *qsort_worker(void *param __attribute__((unused))) {
    struct queue_item *current;
    uint32_t temp;
    while (1) {
        sem_wait(&sem_queue);
        pthread_mutex_lock(&mutex_queue_read);
        current = queue_head++;
        pthread_mutex_unlock(&mutex_queue_read);
        if (current->length > N) {
            // This is a quit signal
            return NULL;
        }
        else if (current->length < MAX_THRESH) {
            qsort(current->start, current->length, 4, uint_comp);
            pthread_mutex_lock(&mutex_remain);
            --task_remain;
            if (task_remain == 0) {
                pthread_mutex_lock(&mutex_queue_write);
                for (unsigned i = 1; i < WORKER_NUM; ++i) {
                    (queue_end++)->length = N + 1;
                    sem_post(&sem_queue);
                }
                pthread_mutex_unlock(&mutex_queue_write);
                return NULL;
            }
            pthread_mutex_unlock(&mutex_remain);
        }
        else {
            uint32_t *lo = current->start;
            uint32_t *mid = current->start + (current->length >> 1);
            uint32_t *hi = current->start + (current->length - 1);

            // sort LO, MID, HI
            if (*mid < *lo) {
                temp = *mid;
                *mid = *lo;
                *lo = temp;
            }
            if (*hi < *mid) {
                temp = *mid;
                *mid = *hi;
                *hi = temp;
                if (*mid < *lo) {
                    temp = *mid;
                    *mid = *lo;
                    *lo = temp;
                }
            }

            // partition
            uint32_t *left = lo + 1, *right = hi - 1;
            do {
                while (*left < *mid) {
                    ++left;
                }
                while (*mid < *right) {
                    --right;
                }
                if (left < right) {
                    temp = *left;
                    *left = *right;
                    *right = temp;
                    if (mid == left) {
                        mid = right;
                    } else if (mid == right) {
                        mid = left;
                    }
                    ++left;
                    --right;
                } else if (left == right) {
                    ++left;
                    --right;
                    break;
                }
            } while (left <= right);

            // assign work [lo, right] & [left, hi]
            // push larger partition first
            pthread_mutex_lock(&mutex_remain);
            ++task_remain;
            pthread_mutex_unlock(&mutex_remain);
            if (right - lo < hi - left) {
                pthread_mutex_lock(&mutex_queue_write);
                queue_end->start = left;
                (queue_end++)->length = hi - left + 1;
                sem_post(&sem_queue);
                queue_end->start = lo;
                (queue_end++)->length = right - lo + 1;
                sem_post(&sem_queue);
                pthread_mutex_unlock(&mutex_queue_write);
            } else {
                pthread_mutex_lock(&mutex_queue_write);
                queue_end->start = lo;
                (queue_end++)->length = right - lo + 1;
                sem_post(&sem_queue);
                queue_end->start = left;
                (queue_end++)->length = hi - left + 1;
                sem_post(&sem_queue);
                pthread_mutex_unlock(&mutex_queue_write);
            }
        }
    }
}

int main(void) {
    void *ret;
    if (pthread_mutex_init(&mutex_remain, NULL)
            || pthread_mutex_init(&mutex_queue_read, NULL)
            || pthread_mutex_init(&mutex_queue_write, NULL)) {
        puts("Mutex init failed.");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_queue, 0, 0)) {
        puts("Semaphore init failed.");
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < WORKER_NUM; ++i) {
        pthread_create(&worker[i], NULL, qsort_worker, NULL);
    }
    FILE *fp = fopen("unsorted", "r");
    if (N != fread(data, 4, N, fp)){
        puts("Input file load failed.");
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    // workers are blocked at the semaphore, so no need of mutex
    queue_head->start = data;
    queue_head->length = N;
    sem_post(&sem_queue);

    for (unsigned i = 0; i < WORKER_NUM; ++i) {
        pthread_join(worker[i], &ret);
    }

    fp = fopen("sorted", "w");
    fwrite(data, 4, N, fp);
    fclose(fp);
    return EXIT_SUCCESS;
}

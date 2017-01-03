#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// #define DEBUG

#ifdef DEBUG
#include <errno.h>
#endif

struct Customer {
    unsigned n, t_need;
    int t_in;
} *customers;

struct TellerThread {
    pthread_t p;
    unsigned n;
} *tellers;

unsigned customers_alloc, customers_top, customers_waiting, teller_n;
sem_t sem_customer;
pthread_mutex_t mutex_waiting_queue;
struct timespec c_start;

int comp_customer(const void *a, const void *b) {
    struct Customer *pa = (struct Customer *) a, *pb = (struct Customer *) b;
    return pa->t_in - pb->t_in;
}

double current_time() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - c_start.tv_sec + (now.tv_nsec - c_start.tv_nsec) / 1.0e9);
}

void *teller(void *param) {
    struct TellerThread *p = (struct TellerThread *) param;
    struct Customer *serving;
    unsigned called_num;
    while (1) {
        sem_wait(&sem_customer);
        pthread_mutex_lock(&mutex_waiting_queue);
        if (customers_waiting == customers_top) {
            pthread_mutex_unlock(&mutex_waiting_queue);
            break;
        }
        called_num = customers_waiting;
        serving = &customers[customers_waiting++];
        pthread_mutex_unlock(&mutex_waiting_queue);
        printf("[%.2f] Teller #%u started to serve customer #%u with number %u.\n",
               current_time(), p->n, serving->n, called_num);
        usleep(1000000u * serving->t_need);
        printf("[%.2f] Teller #%u finished serving customer #%u with number %u.\n",
               current_time(), p->n, serving->n, called_num);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    void *ret;
    if (argc < 2 || 1 != sscanf(argv[1], "%u", &teller_n)) {
        fputs("Usage: ", stdout);
        fputs(argv[0][0] ? argv[0] : "os-hw1", stdout);
        puts(" <teller_num>");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&mutex_waiting_queue, NULL)) {
        puts("Mutex init failed.");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_customer, 0, 0)) {
        puts("Semaphore init failed.");
        exit(EXIT_FAILURE);
    }
    tellers = malloc(teller_n * sizeof(*tellers));
    customers = malloc(sizeof(struct Customer));
    if (!tellers || !customers) {
        puts("Memory alloc failed.");
        exit(EXIT_FAILURE);
    }
    customers_alloc = 1;
    customers_top = 0;
    customers_waiting = 0;
    for (unsigned i = 0; i < teller_n; ++i) {
        tellers[i].n = i;
        pthread_create(&tellers[i].p, NULL, teller, (void *) &tellers[i]);
    }
    for (unsigned n, t_in, t_need; 3 == scanf("%u%u%u", &n, &t_in, &t_need); ++customers_top) {
        if (customers_top == customers_alloc) {
            customers_alloc <<= 1;
            customers = realloc(customers, customers_alloc * sizeof(*customers));
            if (!customers) {
                puts("Memory alloc failed.");
                exit(EXIT_FAILURE);
            }
        }
        customers[customers_top].n = n;
        customers[customers_top].t_in = t_in;
        customers[customers_top].t_need = t_need;
    }
    qsort(customers, customers_top, sizeof(*customers), comp_customer);
    clock_gettime(CLOCK_MONOTONIC, &c_start);
    for (unsigned i = 0; i < customers_top; ++i) {
        double cu = current_time();
        if (cu < customers[i].t_in) {
            usleep((useconds_t) (1000000 * (customers[i].t_in - cu)));
        }
        printf("[%.2f] Customer #%u entered the bank and got number %u.\n", current_time(), customers[i].n, i);
        sem_post(&sem_customer);
    }
    for (unsigned i = 0; i < teller_n; ++i)
        sem_post(&sem_customer);
    for (unsigned i = 0; i < teller_n; ++i)
        pthread_join(tellers[i].p, &ret);
#ifdef DEBUG
    if (sem_destroy(&sem_customer)) {
        printf("Semaphore destroy failed. (%u)\n", errno);
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_destroy(&mutex_waiting_queue)) {
        puts("Mutex destroy failed.");
        exit(EXIT_FAILURE);
    }
#endif
    return EXIT_SUCCESS;
}

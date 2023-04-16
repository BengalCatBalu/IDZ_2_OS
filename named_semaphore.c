#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define NUM_BEES 3
#define NUM_SECTORS 5
#define SEM_NAME "/bees_semaphore"

typedef struct {
    int unsearched_sectors;
} shared_data;

shared_data *create_shared_data() {
    shared_data *data = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    data->unsearched_sectors = NUM_SECTORS;
    return data;
}

void free_shared_data(shared_data *data) {
    munmap(data, sizeof(shared_data));
}

void search_sector(int bee_number, shared_data *data) {
    int search_time;
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);

    while (1) {
        sem_wait(sem);

        if (data->unsearched_sectors == 0) {
            sem_post(sem);
            break;
        }

        search_time = rand() % 5 + 1;
        data->unsearched_sectors--;

        sem_post(sem);

        printf("Bee %d is searching sector for %d seconds...\n", bee_number, search_time);
        sleep(search_time);
    }

    sem_close(sem);
    printf("Bee %d finished searching.\n", bee_number);
}

// Обработчик сигналов для корректного завершения программы при прерывании с клавиатуры
void signal_handler(int signum) {
    printf("\nCaught signal %d, terminating...\n", signum);
    exit(signum);
}

int main() {
    // Устанавливаем обработчик сигналов
    signal(SIGINT, signal_handler);
    // Инициализируем генератор случайных чисел
    srand(time(NULL));
    shared_data *data = create_shared_data();
    pid_t child_pids[NUM_BEES];

    sem_unlink(SEM_NAME);
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);

    for (int i = 0; i < NUM_BEES; i++) {
        child_pids[i] = fork();

        if (child_pids[i] == 0) {
            search_sector(i, data);
            exit(0);
        }
    }

    for (int i = 0; i < NUM_BEES; i++) {
        wait(NULL);
    }

    printf("All bees have finished searching.\n");

    sem_close(sem);
    sem_unlink(SEM_NAME);

    free_shared_data(data);

    return 0;
}


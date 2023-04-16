#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>

#define NUM_BEES 3
#define NUM_SECTORS 5

typedef struct {
    int unsearched_sectors;
} shared_data;

int create_semaphore() {
    int sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    semctl(sem_id, 0, SETVAL, 1);
    return sem_id;
}

void delete_semaphore(int sem_id) {
    semctl(sem_id, 0, IPC_RMID);
}

void semaphore_wait(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

void semaphore_post(int sem_id) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

shared_data *create_shared_data(int *shm_id) {
    *shm_id = shmget(IPC_PRIVATE, sizeof(shared_data), IPC_CREAT | 0666);
    if (*shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    shared_data *data = (shared_data *)shmat(*shm_id, NULL, 0);
    data->unsearched_sectors = NUM_SECTORS;
    return data;
}

void free_shared_data(int shm_id, shared_data *data) {
    shmdt(data);
    shmctl(shm_id, IPC_RMID, NULL);
}

void search_sector(int bee_number, shared_data *data, int sem_id) {
    int search_time;

    while (1) {
        semaphore_wait(sem_id);

        if (data->unsearched_sectors == 0) {
            semaphore_post(sem_id);
            break;
        }

        search_time = rand() % 5 + 1;
        data->unsearched_sectors--;

        semaphore_post(sem_id);

        printf("Bee %d is searching sector for %d seconds...\n", bee_number, search_time);
        sleep(search_time);
    }

    printf("Bee %d finished searching.\n", bee_number);
}

void handle_sigint(int sig) {
    printf("\nTerminating program due to keyboard interrupt...\n");
    exit(EXIT_SUCCESS);
}

int main() {
    int shm_id;
    int sem_id = create_semaphore();
    shared_data *data = create_shared_data(&shm_id);
    pid_t child_pids[NUM_BEES];

    signal(SIGINT, handle_sigint);

    for (int i = 0; i < NUM_BEES; i++) {
        child_pids[i] = fork();

        if (child_pids[i] == 0) {
            search_sector(i, data, sem_id);
            exit(0);
        }
    }

    for (int i = 0; i < NUM_BEES; i++) {
        wait(NULL);
    }

    printf("All bees have finished searching.\n");

    delete_semaphore(sem_id);
    free_shared_data(shm_id, data);

    return 0;
}

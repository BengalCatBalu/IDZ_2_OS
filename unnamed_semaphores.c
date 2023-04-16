#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define NUM_BEES 5
#define NUM_SECTORS 10

// Структура для хранения разделяемых данных
typedef struct {
    sem_t sem;                  // POSIX семафор для синхронизации
    int unsearched_sectors;     // Количество неисследованных участков
} shared_data;

// Создание и инициализация разделяемой памяти для данных
shared_data *create_shared_data() {
    shared_data *data = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&data->sem, 1, 1);
    data->unsearched_sectors = NUM_SECTORS;
    return data;
}

// Освобождение разделяемой памяти
void free_shared_data(shared_data *data) {
    munmap(data, sizeof(shared_data));
}

// Моделирование поиска участка пчелой
void search_sector(int bee_id, shared_data *data) {
    int sleep_time = rand() % 5 + 1;
    printf("Bee %d is searching sector for %d seconds...\n", bee_id, sleep_time);
    sleep(sleep_time);
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

    // Создаем разделяемую память для данных
    shared_data *data = create_shared_data();

    // Создаем процессы-пчелы
    for (int i = 0; i < NUM_BEES; i++) {
        if (fork() == 0) {
            while (1) {
                // Захватываем семафор перед проверкой и изменением данных
                sem_wait(&data->sem);
                if (data->unsearched_sectors <= 0) {
                    // Если нет неисследованных участков, освобождаем семафор и завершаем процесс
                    sem_post(&data->sem);
                    break;
                }
                // Уменьшаем количество неисследованных участков
                data->unsearched_sectors--;
                // Освобождаем семафор
                sem_post(&data->sem);

                // Проводим поиск на участке
                search_sector(i, data);
            }
            printf("Bee %d finished searching.\n", i);
            exit(0);
        }
    }

    // Ждем завершения всех процессов-пчел
    for (int i = 0; i < NUM_BEES; i++) {
        wait(NULL);
    }

    // Выводим сообщение о завершении поиска
    printf("All bees have finished searching.\n");

    // Уничтожаем семафор
    sem_destroy(&data->sem);

    // Освобождаем разделяемую память
    free_shared_data(data);

    return 0;
}



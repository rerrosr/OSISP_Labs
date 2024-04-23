#include "func.h"

void fillingMessages() {
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);     // Открыть совместно используемую память.
    if(shm_fd == -1) {
        printf("Error while opening shared memory.\n");
        exit(EXIT_FAILURE);
    }
                                                                 // Отобразить её.
    QUEUE* queue = mmap(NULL, sizeof(QUEUE)+MAX_MES_COUNT*sizeof(NODE)+MAX_MES_COUNT*sizeof(MESSAGE), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(queue == MAP_FAILED) {
        printf("Error while mapping shared memory.\n");
        exit(EXIT_FAILURE);
    }
                                                                 // Открыть семафоры.
    sem_t* queueAccess = sem_open(QUEUE_ACCESS_SEM_NAME, 0);
    if(queueAccess == SEM_FAILED) {
        printf("Error while opening queue access semaphore.\n");
        exit(EXIT_FAILURE);
    }

    sem_t* fillSem = sem_open(FILL_SEM_NAME, 0);
    if(fillSem == SEM_FAILED) {
        printf("Error while opening filling semaphore.\n");
        exit(EXIT_FAILURE);
    }
    while (continuing) {
        sem_wait(queueAccess);                                   // Ожидание доступа к памяти и заполнению.
        sem_wait(fillSem);

        if (queue->countAdded - queue->countDeleted < MAX_MES_COUNT) {    // Добавить, если есть место.
            push(queue);
            queue->countAdded++;
            printf("Added %d message:\n", queue->countAdded);
            printMes((MESSAGE*)(((NODE*)(queue->ringTail + (uintptr_t)queue))->message));
        } else
            printf("Queue is full!\n");

        sem_post(fillSem);                                       // Освободить семафоры.
        sem_post(queueAccess);
        sleep(3);
    }
                                                                 // Отсоединить память.
    munmap(queue, sizeof(QUEUE)+MAX_MES_COUNT*sizeof(NODE)+MAX_MES_COUNT*sizeof(MESSAGE));
    close(shm_fd);

    sem_close(queueAccess);                                      // Закрыть семафоры.
    sem_close(fillSem);
}
void extractingMessages() {
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);     // Открыть совместо используемую память.
    if(shm_fd == -1) {
        printf("Error while opening shared memory. Code: %d\n", errno);
        exit(EXIT_FAILURE);
    }
                                                                 // Отобразить совместо используемую память.
    QUEUE* queue = mmap(NULL, sizeof(QUEUE)+MAX_MES_COUNT*sizeof(NODE)+MAX_MES_COUNT*sizeof(MESSAGE), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(queue == MAP_FAILED) {
        printf("Error while mapping shared memory.\n");
        exit(EXIT_FAILURE);
    }
                                                                 // Открыть семафоры.
    sem_t* queueAccess = sem_open(QUEUE_ACCESS_SEM_NAME, 0);
    if(queueAccess == SEM_FAILED) {
        printf("Error while opening queue access semaphore.\n");
        exit(EXIT_FAILURE);
    }

    sem_t* extractSem = sem_open(EXTRACT_SEM_NAME, 0);
    if(extractSem == SEM_FAILED) {
        printf("Error while opening extracting semaphore.\n");
        exit(EXIT_FAILURE);
    }

    while(continuing) {
        sem_wait(queueAccess);                                   // Ожидание доступа к памяти и чтению.
        sem_wait(extractSem);

        if(queue->countAdded - queue->countDeleted > 0) {        // Чтение, если есть что читать.
            printf("Delete %d message:\n", queue->countDeleted+1);
            printMes((MESSAGE*)(((NODE*)(queue->ringHead + (uintptr_t)queue))->message));
            pop(queue);
            queue->countDeleted++;
        } else
            printf("Queue is empty!\n");

        sem_post(extractSem);                                    // Разблокировать семафоры.
        sem_post(queueAccess);
        sleep(3);
    }
                                                                 // Отсоединить память.
    munmap(queue, sizeof(QUEUE)+MAX_MES_COUNT*sizeof(NODE)+MAX_MES_COUNT*sizeof(MESSAGE));
    close(shm_fd);

    sem_close(queueAccess);                                      // Закрыть семафоры.
    sem_close(extractSem);
}

void changeContinuingStatus() {
    continuing ^= 1;
}
#include "func.h"
#include <stdint.h>

int continuing = 1;

STACKNODE* stackFiller = NULL;             // Процессы добавления.
STACKNODE* stackExtractor = NULL;          // Процессы удаления.
 
int main() {
    srand(time(NULL));

    shm_unlink(SHARED_MEMORY_NAME);             // Удаление, если созданы, семафоров и совместной памяти.
    sem_unlink(FILL_SEM_NAME);
    sem_unlink(EXTRACT_SEM_NAME);
    sem_unlink(QUEUE_ACCESS_SEM_NAME);

    signal(SIGUSR1, changeContinuingStatus);    // Сигнал отмены работы.

    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);     // Открыть совместо используемую память для чтения и записи всем.
    if(shm_fd == -1) {
        printf("Error while creating shared memory.\n");
        exit(EXIT_FAILURE);
    }                                                                      // Выделить место под заданное количество элементов очереди.
    ftruncate(shm_fd, sizeof(QUEUE)+MAX_MES_COUNT*sizeof(NODE)+MAX_MES_COUNT*sizeof(MESSAGE));
                                                                           // Получение образа памяти.
    QUEUE* queue = mmap(NULL, sizeof(QUEUE)+MAX_MES_COUNT*sizeof(NODE)+MAX_MES_COUNT*sizeof(MESSAGE), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(queue == MAP_FAILED) {
        printf("Error while mapping shared data.\n");
        exit(EXIT_FAILURE);
    }
    queue->ringHead = 0;                                                   // Значение указателей по умолчанию.
    queue->ringTail = 0;
    queue->countDeleted = 0;
    queue->countAdded = 0;
    queue->currentPlaceToWrite = (uintptr_t)queue + sizeof(QUEUE);         // Место для записи.

    sem_t* fillSem;                                                        // Инициализация семафоров.
    if((fillSem = sem_open(FILL_SEM_NAME, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
        printf("Error while open filling semaphore, code %d.\n", errno);
        exit(errno);
    }
    sem_t* extractSem;
    if((extractSem = sem_open(EXTRACT_SEM_NAME, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
        printf("Error while open extracting semaphore, code %d.\n", errno);
        exit(errno);
    }
    sem_t* queueAccess;
    if((queueAccess = sem_open(QUEUE_ACCESS_SEM_NAME, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
        printf("Error while open queue semaphore, code %d.\n", errno);
        exit(errno);
    }

    while(continuing) {
        char ch = getchar();
        switch (ch) {
            case 'w': { 
                pid_t pid = fork();                                       // Создание процесса заполнения.
                if(pid == -1) {
                    printf("Error occurred while creating new filler, error code %d.\n", errno);
                    exit(errno);
                } else if(pid == 0) {
                    fillingMessages();
                    exit(0);
                } else
                    pushStack(&stackFiller, pid);
                break;
            }
            case 's': 
                if(stackFiller!=NULL) {                                   // Удаление процесса заполнения.
                    kill(stackFiller->pid, SIGUSR1);
                    waitpid(stackFiller->pid, NULL, 0);
                    popStack(&stackFiller);
                } else
                    printf("There is no fillers.\n");
                break;
            case 'e': { 
                pid_t pid = fork();                                       // Создание процесса извлечения.
                if(pid == -1) {
                    printf("Error occurred while creating new extractor, error code %d.\n", errno);
                    exit(errno);
                } else if(pid == 0) {
                    extractingMessages();
                    exit(0);
                } else
                    pushStack(&stackExtractor, pid);
                break;
            }
            case 'd':
                if(stackExtractor!=NULL) {                                // Удаление процесса извлечения.
                    kill(stackExtractor->pid, SIGUSR1);
                    waitpid(stackExtractor->pid, NULL, 0);
                    popStack(&stackExtractor);
                } else
                    printf("There is no extractors.\n");
                break;
            case 'q':
                while(stackFiller != NULL) {                              // Конец.
                    kill(stackFiller->pid, SIGUSR1);
                    waitpid(stackFiller->pid, NULL, 0);
                    popStack(&stackFiller);
                }
                while(stackExtractor != NULL) {
                    kill(stackExtractor->pid, SIGUSR1);
                    waitpid(stackExtractor->pid, NULL, 0);
                    popStack(&stackExtractor);
                }
                printf("All fillers are extractors are killed. End of program.\n");
                continuing = 0;
                break;
        }
    }
                                                                           // Освобождение памяти.
    munmap(queue, sizeof(QUEUE)+MAX_MES_COUNT*sizeof(NODE)+MAX_MES_COUNT*sizeof(MESSAGE));
    close(shm_fd);
    shm_unlink(SHARED_MEMORY_NAME);

    sem_close(fillSem);                                                    // Освобождение семафоров.
    sem_unlink(FILL_SEM_NAME);
    sem_close(extractSem);
    sem_unlink(EXTRACT_SEM_NAME);
    sem_close(queueAccess);
    sem_unlink(QUEUE_ACCESS_SEM_NAME);

    return 0;
}
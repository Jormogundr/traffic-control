#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

sem_t semaphore;
// crossing time periods
int DELTA_LEFT = 3;
int DELTA_STRAIGHT = 2;
int DELTA_RIGHT = 1;

typedef struct _directions {
    char dir_original;
    char dir_target;
} directions;

typedef struct _state {
    directions dirs;
    float time;
    int number;

} state;

void state_init(state *state, char dir_original, char dir_target, float time, int number) {
    state->number = number;
    state->time = time;
    state->dirs.dir_original = dir_original;
    state->dirs.dir_target = dir_target;
}

void threadfunc() {
    while (1) {
        sem_wait(&semaphore);
        // critical section
        printf("Hello from da thread!\n");
        sem_post(&semaphore);
        sleep(1);
    }
}

void ArriveIntersection(directions dir) {

}

void CrossIntersection(directions dir) {

}

void ExitIntersection(directions dir) {

}

void Car(directions dir) {
    ArriveIntersection(dir);
    CrossIntersection(dir);
    ExitIntersection(dir);
}

int main(void) {
    // initialize semaphore, only to be used with threads in this process, set value to 1
    sem_init(&semaphore, 0, 1);
    
    pthread_t *thread;
    thread = (pthread_t *)malloc(sizeof(*car));
    
    // start the thread
    printf("Starting thread, semaphore is unlocked.\n");
    pthread_create(thread, NULL, (void*)threadfunc, NULL);
    
    getchar();
    
    sem_wait(&semaphore);
    printf("Semaphore locked.\n");
    
    // do stuff with whatever is shared between threads
    getchar();
    
    printf("Semaphore unlocked.\n");
    sem_post(&semaphore);
    
    getchar();
    
    return 0;
}
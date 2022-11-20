#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

sem_t semaphore;
const int DELTA_LEFT = 3;
const int DELTA_STRAIGHT = 2;
const int DELTA_RIGHT = 1;
int TIME = 0;
const int NUM_CARS = 8;
const int NUM_SEMAPHORES = 24;

// define all semaphores
sem_t stop_signs[4];
sem_t exits[4];
sem_t inner_square[12];
sem_t inner[4];
// read north_to_west as "heading north initially, turn west". The idea is to define arrays of shared semaphores
sem_t *north_to_west[5] = {&inner_square[7], &inner[2], &inner[3], &inner_square[11], &exits[3]};
sem_t *north_to_north[5] = {&inner_square[6], &inner_square[5], &inner_square[4], &inner_square[3], &exits[0]};
sem_t *west_to_south[5] = {&inner_square[4], &inner[1], &inner[2], &inner_square[8], &exits[2]};
sem_t *west_to_west[5] = {&inner_square[3], &inner_square[2], &inner_square[1], &inner_square[0], &exits[3]};
sem_t *south_to_south[5] = {&inner_square[0], &inner_square[11], &inner_square[10], &inner_square[9], &exits[2]};
sem_t *south_to_east[5] = {&inner_square[1], &inner[0], &inner[1], &inner_square[5], &exits[1]};
sem_t *east_to_east[5] = {&inner_square[9], &inner_square[8], &inner_square[7], &inner_square[6], &exits[1]};
sem_t *east_to_north[5] = {&inner_square[10], &inner[3], &inner[0], &inner_square[2], &exits[0]};

typedef struct _directions {
    char dir_original;
    char dir_target;
} directions;

typedef struct _state {
    directions dirs;
    float time;
    int cid;

} state;

void state_init(state *state, char dir_original, char dir_target, float time, int cid) {
    state->cid = cid;
    state->time = time;
    state->dirs.dir_original = dir_original;
    state->dirs.dir_target = dir_target;
}

void ArriveIntersection(state *car) {
    int value; // for debugging
    while (1) {
        sem_wait(&stop_signs[0]);
        // critical section start
        sem_getvalue(&stop_signs[0], &value);
        printf("Hello from da thread! Car %d has %d units avaialble\n", car->cid, value);
        // critical section end
        sem_post(&stop_signs[0]);
        sleep(1);
    }
}

void CrossIntersection(state *car) {
    return;
}

void ExitIntersection(state *car) {
    return;
}

void Car(state *car) {
    ArriveIntersection(&car);
    CrossIntersection(&car);
    ExitIntersection(&car);
}

void print_car(state *car) {
    printf("Time: %f Car: %s (->%s ->%s) %s", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);
}

void initialize_semaphores() {
    int n = 0;
    int value;
    n = sizeof(stop_signs)/sizeof(stop_signs[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&stop_signs[i], 0, 1);
        sem_getvalue(&stop_signs[i], &value);
        printf("stop sign semaphore %d has been allocated with %d \n", i, *(&value));
    }
    n = sizeof(exits)/sizeof(exits[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&exits[i], 0, 1);
        sem_getvalue(&exits[i], &value);
        printf("exit semaphore %d has been allocated with %d \n", i, *(&value));
    }
    n = sizeof(inner_square)/sizeof(inner_square[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&inner_square[i], 0, 1);
        sem_getvalue(&inner_square[i], &value);
        printf("inner_square semaphore %d has been allocated with %d \n", i, *(&value));
    }
    n = sizeof(inner)/sizeof(inner[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&inner[i], 0, 1);
        sem_getvalue(&inner[i], &value);
        printf("inner semaphore %d has been allocated with %d \n", i, *(&value));
    }
}

float secondsToMicroseconds(float n) {
    return n*1000000;
}

float microsecondsToSeconds(float n) {
    return n/1000000;
}

void assign_car_states(state *car_states) {
    int cids[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    float arrival_times[8] = {1.1, 2.0, 3.3, 3.5, 4.2, 4.4, 5.7, 5.9};
    char dirs_original[8] = {'N','N','N','S','S','N','E','W'};
    char dirs_target[8] = {'N','N','W','S','E','N','N','N'};
    for (int i = 0; i < NUM_CARS; i++) {
        car_states[i].dirs.dir_original = dirs_original[i];
        car_states[i].dirs.dir_target = dirs_target[i];
        car_states[i].cid = cids[i];
        car_states[i].time = arrival_times[i];
        
        // create thread for car
        pthread_t *car_thread; 
        car_thread = (pthread_t *)malloc(sizeof(*car_thread));

        // sleep until arrival time
        float t = secondsToMicroseconds(arrival_times[i]); // for debugging
        usleep(t); // return 0 on success
        printf("Thread is waiting for %f us before creation \n", microsecondsToSeconds(t));

        // start the thread -- send car to intersection
        printf("Starting thread, semaphore is unlocked.\n");
        pthread_create(car_thread, NULL, (void*)ArriveIntersection, &car_states[i]);
    }
}

int main(void) {
    // initialize semaphores
    initialize_semaphores();

    // initialize car states
    state cars[8];
    assign_car_states(cars);
    
    getchar();
    
    sem_wait(&stop_signs[0]);
    printf("Semaphore locked.\n");
    
    // do stuff with whatever is shared between threads
    getchar();
    
    printf("Semaphore unlocked.\n");
    sem_post(&stop_signs[0]);
    
    getchar();
    
    return 0;
}
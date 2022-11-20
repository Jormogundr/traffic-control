#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>

sem_t semaphore;
const int DELTA_LEFT = 3;
const int DELTA_STRAIGHT = 2;
const int DELTA_RIGHT = 1;
int TIME = 0;
const int NUM_CARS = 8;
const int NUM_SEMAPHORES = 24;

// define all semaphores
sem_t stop_signs[4];
sem_t intersection[20];
// read north_to_west as "heading north initially, turn west". The idea is to define arrays of shared semaphores
sem_t *north_to_west[5] = {&intersection[11], &intersection[18], &intersection[19], &intersection[15], &intersection[3]};
sem_t *north_to_north[5] = {&intersection[10], &intersection[9], &intersection[8], &intersection[7], &intersection[0]};
sem_t *west_to_south[5] = {&intersection[8], &intersection[17], &intersection[18], &intersection[13], &intersection[2]};
sem_t *west_to_west[5] = {&intersection[7], &intersection[6], &intersection[5], &intersection[4], &intersection[3]};
sem_t *south_to_south[5] = {&intersection[4], &intersection[15], &intersection[14], &intersection[13], &intersection[2]};
sem_t *south_to_east[5] = {&intersection[5], &intersection[16], &intersection[17], &intersection[9], &intersection[1]};
sem_t *east_to_east[5] = {&intersection[13], &intersection[12], &intersection[11], &intersection[10], &intersection[1]};
sem_t *east_to_north[5] = {&intersection[14], &intersection[19], &intersection[16], &intersection[6], &intersection[0]};

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

/***
 * Check lock on stop sign dependning on the original direction of travel. Thread will wait in this function until semaphore for the stop sign is released.
*/
void check_first_at_sign(state *car) {
    char origin = car->dirs.dir_original; // original direction of travel
    switch (origin) {
        case 'N': 
            sem_wait(&stop_signs[2]);
        case 'E':
            sem_wait(&stop_signs[3]);
        case 'S':
            sem_wait(&stop_signs[0]);
        case 'W':
            sem_wait(&stop_signs[1]);
    }
} 

int check_and_set_intersection_locks(int collision_points[]) {
    int n = sizeof(collision_points)/sizeof(collision_points[0]);
    int value;
    int idx;
    for (int i; i < n; i++) {
        idx = collision_points[i];
        sem_getvalue(&intersection[idx], &value);
        if (value == 0) {
            return 0;
        }
        else {
            sem_wait(&intersection[idx]);
        }
    }
    return 1;
}


void check_intersection(state *car) {
    char origin = car->dirs.dir_original; // original direction of travel
    char target = car->dirs.dir_target; 
    int collision_points[5] = {0,0,0,0,0};
    // generate a list of integers corresponding to intersection semaphore indices
    if (origin == 'N' && target == 'W') {
        int collision_points[5] = {11,18,19,15,3};
    }
    if (origin == 'N' && target == 'N') {
        int collision_points[5] = {10,9,8,7,0};
    }
    if (origin == 'W' && target == 'S') {
        int collision_points[5] = {8,17,18,12,2};
    }
    if (origin == 'W' && target == 'W') {
        int collision_points[5] = {7,6,5,4,3};
    }
    if (origin == 'S' && target == 'S') {
        int collision_points[5] = {4,15,14,13,2};
    }
    if (origin == 'S' && target == 'E') {
        int collision_points[5] = {5,16,17,9,1};
    }
    if (origin == 'E' && target == 'E') {
        int collision_points[5] = {13,12,11,10,1};
    }
    if (origin == 'E' && target == 'N') {
        int collision_points[5] = {14,19,16,6,0};
    }

    // check if intersection is clear -- and if so, acquire lock on relevant semaphore indices
    check_and_set_intersection_locks(collision_points);
    
}

void ArriveIntersection(state *car) {
    int value; // for debugging
    printf("Time: %f Car %d (->%c->%c)  arriving \n", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);
    //printf("Hello from da thread! Car %d has arrived at intersection with %d units avaialble\n", car->cid, value);
    check_first_at_sign(car);
    check_intersection(car);
    sem_getvalue(&stop_signs[0], &value);
    sleep(1);
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

// void print_event(state *car, const char s[]) {
//     printf("Time: %f Car: %s (->%s ->%s) %s %s", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target, s);
// }

void initialize_semaphores() {
    int n = 0;
    n = sizeof(stop_signs)/sizeof(stop_signs[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&stop_signs[i], 0, 1);
    }
    n = sizeof(intersection)/sizeof(intersection[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&intersection[i], 0, 1);
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

        // sleep until car arrival time
        float t = secondsToMicroseconds(arrival_times[i]);
        usleep(t); // return 0 on success

        // start the thread -- send car to intersection
        pthread_create(car_thread, NULL, (void*)ArriveIntersection, &car_states[i]);
    }
}

int main(void) {
    // initialize semaphores
    initialize_semaphores();

    // initialize car states and create threads for each
    state cars[8];
    assign_car_states(cars);
    
    return 0;
}
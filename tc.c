#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>
#include "common.h"

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
sem_t *east_to_south = &intersection[2];
sem_t *north_to_east = &intersection[1];
sem_t *west_to_north = &intersection[0];
sem_t *south_to_west = &intersection[3];


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

void check_and_set_intersection_locks(sem_t *collision_points[]) {
    int n = sizeof(collision_points)/sizeof(collision_points[0]);
    int value;
    for (int i; i < n; i++) {
        sem_getvalue(&collision_points[i], &value);
        if (value == 0) {
            return;
        }
        else {
            sem_wait(&intersection[i]);
        }
    }
}

sem_t *check_intersection(state *car) {
    char origin = car->dirs.dir_original; // original direction of travel
    char target = car->dirs.dir_target; 
    int value;
    // generate a list of integers corresponding to intersection semaphore indices
    if (origin == 'N' && target == 'W') {
        return &north_to_west;
    }
    else if (origin == 'N' && target == 'N') {
        sem_getvalue(&north_to_north, &value);
        return &north_to_north;
    }
    else if (origin == 'W' && target == 'S') {
        return &west_to_south;
    }
    else if (origin == 'W' && target == 'W') {
        return &west_to_west;
    }
    else if (origin == 'S' && target == 'S') {
        return &south_to_south;
    }
    else if (origin == 'S' && target == 'E') {
        return &south_to_east;
    }
    else if (origin == 'E' && target == 'E') {
        return &east_to_east;
    }
    else if (origin == 'E' && target == 'N') {
        return &east_to_north;
    }
    else if (origin == 'N' && target == 'E') {
        return &north_to_east;
    }
    else if (origin == 'W' && target == 'N') {
        return &west_to_north;
    }
    else if (origin == 'S' && target == 'W') {
        return &south_to_west;
    }
    else if (origin == 'E' && target == 'S') {
        return &east_to_south;
    }

    // check if intersection is clear -- and if so, acquire lock on relevant semaphore indices
    // check_and_set_intersection_locks();
    
}

char get_turn(state *car) {
    char origin = car->dirs.dir_original; // original direction of travel
    char target = car->dirs.dir_target; 
    // L -> left, R -> right, S -> straight. R is the default -- if not any of below cases it must be a right turn
    char turn = 'R';
    // generate a list of integers corresponding to intersection semaphore indices
    if (origin == 'N' && target == 'W') {
        turn = 'L';
    }
    if (origin == 'N' && target == 'N') {
        turn = 'S';
    }
    if (origin == 'W' && target == 'S') {
        turn = 'L';
    }
    if (origin == 'W' && target == 'W') {
        turn = 'S';
    }
    if (origin == 'S' && target == 'S') {
        turn = 'S';
    }
    if (origin == 'S' && target == 'E') {
        turn = 'L';
    }
    if (origin == 'E' && target == 'E') {
        turn = 'S';
    }
    if (origin == 'E' && target == 'N') {
        turn = 'L';
    }
    return turn;
}

void ArriveIntersection(state *car) {
    int value; // for debugging
    //sem_getvalue(&stop_signs[0], &value);
    //printf("Hello from da thread! Car %d has arrived at intersection with %d units avaialble\n", car->cid, value);

    printf("Time: %f Car %d (->%c->%c)  arriving \n", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);
    check_first_at_sign(car);
    sem_t *collision_points = check_intersection(car);
    check_and_set_intersection_locks(collision_points);
    char turn = get_turn(car);
    CrossIntersection(turn, car);
    ExitIntersection(car, collision_points);
    
    sleep(1);
}

void CrossIntersection(char turn, state *car) {
    printf("Time: %f Car %d (->%c->%c)      crossing \n", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);
    // wait for time depending on type of turn
    switch (turn) {
        case 'L':
            Spin(DELTA_LEFT);
        case 'R':
            Spin(DELTA_RIGHT);
        case 'S':
            Spin(DELTA_STRAIGHT); 
    }
}

void ExitIntersection(state *car, int collision_points[]) {
    printf("Time: %f Car %d (->%c->%c)          exiting \n", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);
    int n = sizeof(collision_points)/sizeof(collision_points[0]);
    for (int i = 0; i < n; i++) {
        sem_post(&collision_points[i]);
    }
}

void initialize_semaphores() {
    int n = 0;
    int value; // for debugging
    n = sizeof(stop_signs)/sizeof(stop_signs[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&stop_signs[i], 0, 1);
    }
    n = sizeof(intersection)/sizeof(intersection[0]);
    for (int i  = 0; i < n; i++) {
        sem_init(&intersection[i], 0, 1);
        sem_getvalue(&intersection[i], &value);
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
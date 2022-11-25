#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include "common.h"

sem_t semaphore;
float DELTA_LEFT = 3.0;
float DELTA_STRAIGHT = 2.0;
float DELTA_RIGHT = 1.0;
const int NUM_CARS = 8;
const int NUM_SEMAPHORES = 24;
float GLOBAL_TIME = 0;
char PREV_CAR_TARGET = 'N';
char PREV_CAR_ORIGIN = 'N';
int DEBUG = 0;

// define all semaphores
sem_t stop_signs[4];
sem_t intersection[20];
// read north_to_west as "heading north initially, turn west". The idea is to define arrays of shared semaphores
sem_t *north_to_west[5] = {&intersection[11], &intersection[18], &intersection[19], &intersection[15], &intersection[3]};
sem_t *north_to_north[5] = {&intersection[10], &intersection[9], &intersection[8], &intersection[7], &intersection[0]};
sem_t *west_to_south[5] = {&intersection[8], &intersection[17], &intersection[18], &intersection[12], &intersection[2]};
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


float secondsToMicroseconds(float n) {
    return n*1000000;
}

float microsecondsToSeconds(float n) {
    return n/1000000;
}

/***
 * Check lock on stop sign dependning on the original direction of travel. Thread will wait in this function until semaphore for the stop sign is released.
*/
void check_and_lock_stop_sign(state *car) {
    char origin = car->dirs.dir_original; // original direction of travel
    if (origin == 'N') {
        sem_trywait(&stop_signs[2]);
    }
    else if (origin == 'E') {
        sem_trywait(&stop_signs[3]);
    }
    else if (origin == 'S') {
        sem_trywait(&stop_signs[0]);
    }
    else {
        sem_trywait(&stop_signs[1]);
    }
} 

void check_and_unlock_stop_sign(state *car) {
    char origin = car->dirs.dir_original; // original direction of travel
    if (origin == 'N') {
        sem_post(&stop_signs[2]);
    }
    else if (origin == 'E') {
        sem_post(&stop_signs[3]);
    }
    else if (origin == 'S') {
        sem_post(&stop_signs[0]);
    }
    else {
        sem_post(&stop_signs[1]);
    }
}

// check if intersection is clear -- and if so, acquire lock on relevant semaphore indices
void check_and_lock_intersection(state *car, int collision_points[5], int array_size) {
    int idx, sub_idx;
    int value;
    for (int i = 0; i < array_size; i++) {
        idx = collision_points[i];
        if (DEBUG) printf("In scope check_and_lock_intersection for car %d collision point %d is being checked/locked \n", car->cid, collision_points[i]);
        // break for dummy val -1
        if (idx < 0)
            break;
        sem_getvalue(&intersection[idx], &value);
        if (DEBUG) printf("PRE WAIT Check and lock for car %d: collision point %d has semaphore units = %d \n", car->cid, collision_points[i], value);

        // for (int j = i; j < array_size; j++) {
        //     sub_idx = collision_points[j];
        //     sem_getvalue(&intersection[sub_idx], &value);
        //     while (abs(value) > 0)
        //     ;
        // }
        sem_trywait(&intersection[idx]);
        

        sem_getvalue(&intersection[idx], &value);
        if (DEBUG) printf("POST WAIT Check and lock for car %d: collision point %d has semaphore units = %d \n", car->cid, collision_points[i], value);
    }
}

void check_and_unlock_intersection(state *car, int collision_points[5], int array_size) {
    int idx;
    int value;
    for (int i = 0; i < array_size; i++) {
        idx = collision_points[i];
        if (DEBUG) printf("In scope check_and_unlock_intersection for car %d collision point %d is being checked/unlocked \n", car->cid, collision_points[i]);
        // break for dummy val -1
        if (idx < 0)
            break;

        sem_post(&intersection[idx]);

        sem_getvalue(&intersection[idx], &value);
        if (DEBUG) printf("POST POST Check and unlock for car %d: collision point %d has semaphore units = %d \n", car->cid, collision_points[i], value);
    }
}

void assign_collision_points(state *car, int collision_points[5], int points[5], int array_size) {
    if (DEBUG) {
        printf("In scope assign_collision_points... \n");
        printf("Assigning collision points in intersection for car %d (->%c, ->%c)... \n", car->cid, car->dirs.dir_original, car->dirs.dir_target);
    }
    for (int i = 0 ; i < array_size; i++) {
        // break for dummy val -1
        if (points[i] < 0) 
            break;
        collision_points[i] = points[i];
        if (DEBUG) printf("Car %d (->%c, ->%c) has been assigned collision point %d for index %d \n", car->cid, car->dirs.dir_original, car->dirs.dir_target, points[i], i);
    }
}

void check_intersection(state *car, int *collision_points, int array_size) {
    char origin = car->dirs.dir_original; // original direction of travel
    char target = car->dirs.dir_target; 
    int value;

    if (DEBUG) {
        printf("In scope check_intersection()...\n");
        printf("Checking intersection for car %d (->%c, ->%c) \n", car->cid, car->dirs.dir_original, car->dirs.dir_target);
    }
    
    // generate a list of integers corresponding to intersection semaphore indices then check and set semaphores bast on the collision points
    if (origin == 'N' && target == 'W') {
        int points_to_assign[5] = {11, 18, 19, 15, 3};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'N' && target == 'N') {
        int points_to_assign[5] = {10, 9, 8, 7, 0};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }   
    else if (origin == 'W' && target == 'S') {
        int points_to_assign[5] = {8, 17, 18, 12, 2};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'W' && target == 'W') {
        int points_to_assign[5] = {7,6,5,4,3};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'S' && target == 'S') {
        int points_to_assign[5] = {4,15,14,13,2};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'S' && target == 'E') {
        int points_to_assign[5] = {5,16,17,9,1};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'E' && target == 'E') {
        int points_to_assign[5] = {4,5,6,7,1};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'E' && target == 'N') {
        int points_to_assign[5] = {14,19,16,6,0};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'N' && target == 'E') {
        int points_to_assign[5] = {1, -1, -1, -1, -1};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'W' && target == 'N') {
        int points_to_assign[5]= {0, -1, -1, -1, -1};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'S' && target == 'W') {
        int points_to_assign[5] = {3, -1, -1, -1, -1};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }
    else if (origin == 'E' && target == 'S') {
        int points_to_assign[5] = {2, -1, -1, -1, -1};
        assign_collision_points(car, collision_points, points_to_assign, array_size);
        check_and_lock_intersection(car, collision_points, array_size);
    }

    if (DEBUG) {
        for (int i = 0 ; i < array_size; i++) {
            printf("In scope check_intersection, car %d (->%c, ->%c) has been assigned collision point %d for index %d \n", car->cid, car->dirs.dir_original, car->dirs.dir_target, collision_points[i], i);
        }
    }
}

char get_turn(state *car) {
    char origin = car->dirs.dir_original; // original direction of travel
    char target = car->dirs.dir_target; 
    int number_of_collision_points;
    // L -> left, R -> right, S -> straight. R is the default -- if not any of below cases it must be a right turn
    char turn = 'R';

    if (origin == 'N' && target == 'W') {
        turn = 'L';
    }
    else if (origin == 'N' && target == 'N') {
        turn = 'S';
    }
    else if (origin == 'W' && target == 'S') {
        turn = 'L';
    }
    else if (origin == 'W' && target == 'W') {
        turn = 'S';
    }
    else if (origin == 'S' && target == 'S') {
        turn = 'S';
    }
    else if (origin == 'S' && target == 'E') {
        turn = 'L';
    }
    else if (origin == 'E' && target == 'E') {
        turn = 'S';
    }
    else if (origin == 'E' && target == 'N') {
        turn = 'L';
    }
    return turn;
}

int assign_number_of_collision_points(char turn) {
    int number_of_collision_points;
    // based on type of turn, assign number of potential collision points (# semaphores to lock)
    if (turn == 'L' || turn == 'S') {
        number_of_collision_points = 5;
    }
    else {
        number_of_collision_points = 1;
    }

    return number_of_collision_points;
}

void ArriveIntersection(state *car) {
    int collision_points[5] = {0,0,0,0,0};
    int array_size = 5;
    int number_of_collision_points;
    char turn;

    printf("Time: %f Car %d (->%c->%c)  arriving \n", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);

    // skip lock of stop sign if previous car and current car are heading same direction with same origin
    // if (!(car->dirs.dir_target == PREV_CAR_TARGET && car->dirs.dir_original == PREV_CAR_ORIGIN))
    check_and_lock_stop_sign(car);
    check_intersection(car, collision_points, array_size);
    PREV_CAR_TARGET = car->dirs.dir_target;
    PREV_CAR_ORIGIN = car->dirs.dir_original;
    
    if (DEBUG) {
        for (int i = 0 ; i < array_size; i++) {
            printf("In scope ArriveIntersection, car %d (->%c, ->%c) has been assigned collision point %d for index %d \n", car->cid, car->dirs.dir_original, car->dirs.dir_target, collision_points[i], i);
        }
    }

    turn = get_turn(car);
    if (DEBUG) printf("In scope ArriveIntersection,  Car %d (->%c->%c)  arriving has been assigned a turn %c \n", car->cid, car->dirs.dir_original, car->dirs.dir_target, turn);
    number_of_collision_points = assign_number_of_collision_points(turn);
    if (DEBUG) printf("In scope ArriveIntersection,  Car %d (->%c->%c)  arriving has number of collision points %d \n", car->cid, car->dirs.dir_original, car->dirs.dir_target, number_of_collision_points);
    CrossIntersection(car, collision_points, turn);
    ExitIntersection(car, collision_points, array_size);
}

void CrossIntersection(state *car, int collision_points[5], char turn) {
    printf("Time: %f Car %d (->%c->%c)      crossing \n", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);
    if (DEBUG) printf("In scope CrossIntersection, car %d (->%c->%c) is making turn %c \n", car->cid, car->dirs.dir_original, car->dirs.dir_target, turn);

    // since we are crossing, free stop sign for next car
    check_and_unlock_stop_sign(car);

    // wait for time depending on type of turn
    if (turn == 'L') {
            if (DEBUG) printf("pre spin for car %d for turn %c \n", car->cid, turn);
            Spin(DELTA_LEFT);
            car->time += DELTA_LEFT;
            if (DEBUG) printf("post spin for car %d for turn %c \n", car->cid, turn);
    }
    else if (turn == 'S') {
            if (DEBUG) printf("pre spin for car %d for turn %c \n", car->cid, turn);
            Spin(DELTA_STRAIGHT);
            car->time += DELTA_STRAIGHT;
            if (DEBUG) printf("post spin for car %d for turn %c \n", car->cid, turn);
    }
    else if (turn == 'R') {
            if (DEBUG) printf("pre spin for car %d for turn %c \n", car->cid, turn);
            Spin(DELTA_RIGHT);
            car->time += DELTA_RIGHT;
            if (DEBUG) printf("post spin for car %d for turn %c \n", car->cid, turn);
    }
}

void ExitIntersection(state *car, int collision_points[5], int array_size) {
    printf("Time: %f Car %d (->%c->%c)          exiting \n", car->time, car->cid, car->dirs.dir_original, car->dirs.dir_target);
    check_and_unlock_intersection(car, collision_points, array_size);
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
    }
}

void assign_car_states(state *cars) {
    // create array of threads for cars
    pthread_t car_threads[8]; 
    int cids[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    float arrival_times[8] = {1.1, 2.0, 3.3, 3.5, 4.2, 4.4, 5.7, 5.9};
    char dirs_original[8] = {'N','N','N','S','S','N','E','W'};
    char dirs_target[8] = {'N','N','W','S','E','N','N','N'};
    for (int i = 0; i < NUM_CARS; i++) { 
        cars[i].dirs.dir_original = dirs_original[i];
        cars[i].dirs.dir_target = dirs_target[i];
        cars[i].cid = cids[i];
        cars[i].time = arrival_times[i];
        
        // sleep until car arrival time
        float dt = arrival_times[i];
        if (i > 0) {
            dt -= arrival_times[i - 1];
        }
        unsigned int t = secondsToMicroseconds(dt);
        usleep(t); // return 0 on success

        // start the thread -- send car to intersection
        car_threads[i] = (pthread_t *)malloc(sizeof(car_threads[0]));
        pthread_create(&car_threads[i], NULL, (void*)ArriveIntersection, &cars[i]);
        // pthread_setname_np(&car_threads[i], "car %d");

        // thread must post semaphore before reaching this point
        // pthread_cancel(car_thread);
    }

    for (int i = 0; i < NUM_CARS; i++) {
        pthread_join(car_threads[i], NULL);
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
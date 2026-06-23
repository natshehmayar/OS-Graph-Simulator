#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>

typedef struct QueueNode {
    int travelerIndex;
    pid_t pid;
    int currentNode;
    int requestedNode;

    double arrivalTime;
    double waitingStartTime;

    int remainingPath;

    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int size;
} IntersectionQueue;

typedef enum {
    SCHED_FCFS,
    SCHED_SJF
} SchedulerType;

QueueNode* choose_next_process(
    IntersectionQueue *queue,
    SchedulerType schedulerType
);

QueueNode* fcfs_scheduler(
    IntersectionQueue *queue
);

QueueNode* sjf_scheduler(
    IntersectionQueue *queue
);

#endif
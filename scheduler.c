#include "scheduler.h"
#include <stddef.h>

QueueNode* fcfs_scheduler(IntersectionQueue *queue)
{
    if (!queue || !queue->front)
        return NULL;

    return queue->front;
}

QueueNode* sjf_scheduler(IntersectionQueue *queue)
{
    if (!queue || !queue->front)
        return NULL;

    QueueNode *best = queue->front;
    QueueNode *cur = queue->front;

    while (cur)
    {
        if (cur->remainingPath < best->remainingPath)
            best = cur;

        cur = cur->next;
    }

    return best;
}

QueueNode* choose_next_process(
    IntersectionQueue *queue,
    SchedulerType type
)
{
    if (type == SCHED_FCFS)
        return fcfs_scheduler(queue);

    return sjf_scheduler(queue);
}
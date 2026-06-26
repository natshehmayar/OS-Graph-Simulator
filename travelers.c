#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <unistd.h>     // fork(), sleep()
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // kill()
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include "scheduler.h"


#define STATUS_MOVING 0
#define STATUS_WAITING 1
#define STATUS_ENTERED 2
#define STATUS_FINISHED 3

#define GUI_STATUS_READY 0
#define GUI_STATUS_MOVING 1
#define GUI_STATUS_WAITING 2
#define GUI_STATUS_INSIDE 3
#define GUI_STATUS_FINISHED 4


#define MAX_NODES 20
#define MAX_EDGES 100
#define NODE_RADIUS 28
#define INF 1000000000
#define MAX_TRAVELERS 10

// basic data structers  for grahps
typedef struct {
    int src;
    int dst;
    int weight;
} Edge;

typedef struct {
    float x;
    float y;
} Position;

// section 4
typedef struct {
    pid_t pid;
    int sourceNode;
    int destinationNode;
    int shortestPath[MAX_NODES];
    int pathLength;
    int totalDistance;

    // traveler-spesific movement variables
    int currentPathIndex;
    int currentJump;
    float entityX;
    float entityY;
    int animationFinished;
    int waitingForNode;
    int movingFromNode;
    int movingToNode;
    int insideNode;
    int guiStatus;
    double insideStartTime;
    int insideReleaseSent;
    int leaveAckSent;
    double waitingStartTime;
    double totalWaitingTime;   // Milestone 7: total time spent waiting in queues
    int waitingAckSent;        // 0 = not granted yet, 1 = scheduler already granted permission
    Color color; // a unique color for every traveler
} Traveler;

typedef struct {
    int travelerIndex;
    int currentNode;
    int nextNode;
    int finished;
    pid_t pid;
    int status;
} Message;

// Milestone 7 : waiting queue item for each intersection
// typedef struct QueueNode {
//     int travelerIndex;
//     pid_t pid;
//     int currentNode;
//     int requestedNode;
//     double arrivalTime;
//     double waitingStartTime;
//     int remainingPath;
//     struct QueueNode *next;
// } QueueNode;

// typedef struct {
//     QueueNode *front;
//     QueueNode *rear;
//     int size;
// } IntersectionQueue;

Edge edges[MAX_EDGES];
Position positions[MAX_NODES];
Traveler travelers[MAX_TRAVELERS];
IntersectionQueue intersectionQueues[MAX_NODES];
int pipes[MAX_TRAVELERS][2];
sem_t *nodeLocks;
sem_t *leaveSync;
sem_t *outsideSync;
sem_t *waitShownSync;
sem_t *insideDoneSync;
sem_t *childrenReady;
sem_t *schedulerSync;

int nodeCount;
int edgeCount;
int travelerCount;// number of travelers (sons)

// typedef enum {
//     SCHED_FCFS,
//     SCHED_SJF
// } SchedulerType;

SchedulerType schedulerType;

int isPlaying = 0;
int startSent = 0;
volatile sig_atomic_t startFlag = 0;

void startHandler(int sig) {
    startFlag = 1;
}
double lastMoveTime = 0;

// color matrix for distinguishing travelers
Color travelerColors[MAX_TRAVELERS] = {
    ORANGE, GOLD, LIME, MAROON, PURPLE, VIOLET, DARKGREEN, PINK, MAGENTA, BEIGE
};

IntersectionQueue *get_waiting_queue(int intersection);
const char* getSchedulerName();
int get_edge_weight(int src, int dst);

const char *statusText(int status) {
    switch (status) {
        case GUI_STATUS_MOVING:
            return "MOVING";
        case GUI_STATUS_WAITING:
            return "WAITING";
        case GUI_STATUS_INSIDE:
            return "INSIDE NODE";
        case GUI_STATUS_FINISHED:
            return "FINISHED";
        default:
            return "READY";
    }
}

void waitSemaphore(sem_t *sem) {
    while (sem_wait(sem) == -1 && errno == EINTR) {
    }
}


void init_intersection_queues() {
    for (int i = 0; i < MAX_NODES; i++) {
        intersectionQueues[i].front = NULL;
        intersectionQueues[i].rear = NULL;
        intersectionQueues[i].size = 0;
    }
}

int is_intersection_queue_empty(int intersection) {
    if (intersection < 0 || intersection >= MAX_NODES) {
        return 1;
    }

    return intersectionQueues[intersection].size == 0;
}

void add_to_intersection_queue(int intersection, int travelerIndex, int currentNode) {
    if (intersection < 0 || intersection >= MAX_NODES) {
        return;
    }

    QueueNode *newNode = malloc(sizeof(QueueNode));
    if (newNode == NULL) {
        perror("malloc queue node");
        return;
    }

    newNode->travelerIndex = travelerIndex;
    newNode->pid = travelers[travelerIndex].pid;
    newNode->currentNode = currentNode;
    newNode->requestedNode = intersection;
    newNode->arrivalTime = GetTime();
    newNode->waitingStartTime = GetTime();

    newNode->remainingPath = get_edge_weight(currentNode, intersection);

    newNode->next = NULL;

    if (intersectionQueues[intersection].rear == NULL) {
        intersectionQueues[intersection].front = newNode;
        intersectionQueues[intersection].rear = newNode;
    } else {
        intersectionQueues[intersection].rear->next = newNode;
        intersectionQueues[intersection].rear = newNode;
    }

    intersectionQueues[intersection].size++;

    printf("Traveler %d added to waiting queue of node %d. Queue size: %d\n",
           travelerIndex, intersection, intersectionQueues[intersection].size);
}

int get_edge_weight(int src, int dst) {
    for (int i = 0; i < edgeCount; i++) {
        if (edges[i].src == src && edges[i].dst == dst) {
            return edges[i].weight;
        }
    }

    return INF;
}

int queue_has_granted_traveler(int intersection) {
    IntersectionQueue *queue = get_waiting_queue(intersection);

    if (queue == NULL) {
        return 0;
    }

    QueueNode *current = queue->front;
    while (current != NULL) {
        int idx = current->travelerIndex;

        if (idx >= 0 && idx < travelerCount && travelers[idx].waitingAckSent) {
            return 1;
        }

        current = current->next;
    }

    return 0;
}

void grant_waiting_travelers_by_scheduler() {
    double currentTime = GetTime();

    for (int node = 0; node < nodeCount; node++) {
        if (is_intersection_queue_empty(node) || queue_has_granted_traveler(node)) {
            continue;
        }

        if (sem_trywait(&nodeLocks[node]) == -1) {
            continue;
        }

        QueueNode *selected = choose_next_process(get_waiting_queue(node), schedulerType);

        if (selected == NULL) {
            sem_post(&nodeLocks[node]);
            continue;
        }

        int selectedTraveler = selected->travelerIndex;

        if (selectedTraveler < 0 || selectedTraveler >= travelerCount) {
            sem_post(&nodeLocks[node]);
            continue;
        }

        // Keep WAIT visible briefly in GUI before allowing the traveler to enter the reserved node.
        if (currentTime - travelers[selectedTraveler].waitingStartTime >= 0.2) {
            travelers[selectedTraveler].waitingAckSent = 1;
            sem_post(&schedulerSync[selectedTraveler]);

            printf("Scheduler %s selected Traveler %d for node %d\n",
                   getSchedulerName(), selectedTraveler, node);
            fflush(stdout);
        } else {
            sem_post(&nodeLocks[node]);
        }
    }
}

int remove_from_intersection_queue(int intersection, int travelerIndex) {
    if (intersection < 0 || intersection >= MAX_NODES) {
        return 0;
    }

    QueueNode *current = intersectionQueues[intersection].front;
    QueueNode *previous = NULL;

    while (current != NULL) {
        if (current->travelerIndex == travelerIndex) {
            if (previous == NULL) {
                intersectionQueues[intersection].front = current->next;
            } else {
                previous->next = current->next;
            }

            if (intersectionQueues[intersection].rear == current) {
                intersectionQueues[intersection].rear = previous;
            }

            free(current);
            intersectionQueues[intersection].size--;

            printf("Traveler %d removed from waiting queue of node %d. Queue size: %d\n",
                   travelerIndex, intersection, intersectionQueues[intersection].size);

            return 1;
        }

        previous = current;
        current = current->next;
    }

    return 0;
}

IntersectionQueue *get_waiting_queue(int intersection) {
    if (intersection < 0 || intersection >= MAX_NODES) {
        return NULL;
    }

    return &intersectionQueues[intersection];
}

void clear_intersection_queues() {
    for (int i = 0; i < MAX_NODES; i++) {
        QueueNode *current = intersectionQueues[i].front;

        while (current != NULL) {
            QueueNode *next = current->next;
            free(current);
            current = next;
        }

        intersectionQueues[i].front = NULL;
        intersectionQueues[i].rear = NULL;
        intersectionQueues[i].size = 0;
    }
}



void sendMessage(int pipeFd, int travelerIndex, int currentNode, int nextNode, int status, int finished) {
    Message msg;

    msg.travelerIndex = travelerIndex;
    msg.currentNode = currentNode;
    msg.nextNode = nextNode;
    msg.finished = finished;
    msg.pid = getpid();
    msg.status = status;

    write(pipeFd, &msg, sizeof(Message));
}

sem_t *createSemaphoreArray(int count, int initialValue, const char *name) {
    sem_t *array = mmap(NULL,
                 sizeof(sem_t) * count,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS,
                 -1,
                 0);

    if (array == MAP_FAILED) {
        perror(name);
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        if (sem_init(&array[i], 1, initialValue) == -1) {
            perror(name);
            return NULL;
        }
    }

    return array;
}

Position outsideNodePosition(int currentNode, int nextNode) {
    Position outside = positions[nextNode];
    float sx = positions[currentNode].x;
    float sy = positions[currentNode].y;
    float tx = positions[nextNode].x;
    float ty = positions[nextNode].y;
    float dx = tx - sx;
    float dy = ty - sy;
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0) {
        outside.x = tx - (dx / len) * (NODE_RADIUS + 12);
        outside.y = ty - (dy / len) * (NODE_RADIUS + 12);
    }

    return outside;
}

void calculatePositions() {
    float centerX = 500;
    float centerY = 410;
    float radius = 220;
    for (int i = 0; i < nodeCount; i++) {
        float angle = (2 * PI * i) / nodeCount - PI / 2;
        positions[i].x = centerX + radius * cosf(angle);
        positions[i].y = centerY + radius * sinf(angle);
    }
}

void drawArrow(float startX, float startY, float endX, float endY, Color color) {
    float dx = endX - startX;
    float dy = endY - startY;
    float length = sqrtf(dx * dx + dy * dy);
    if (length == 0) return;

    float ux = dx / length;
    float uy = dy / length;
    float realStartX = startX + ux * NODE_RADIUS;
    float realStartY = startY + uy * NODE_RADIUS;
    float realEndX = endX - ux * NODE_RADIUS;
    float realEndY = endY - uy * NODE_RADIUS;

    DrawLineEx((Vector2){realStartX, realStartY}, (Vector2){realEndX, realEndY}, 2, color);

    float arrowSize = 12;
    Vector2 tip = {realEndX, realEndY};
    Vector2 left = {realEndX - arrowSize * ux + arrowSize * 0.5f * uy, realEndY - arrowSize * uy - arrowSize * 0.5f * ux};
    Vector2 right = {realEndX - arrowSize * ux - arrowSize * 0.5f * uy, realEndY - arrowSize * uy + arrowSize * 0.5f * ux};
    DrawTriangle(tip, left, right, color);
}

// dijksta
void runDijkstraForTraveler(int tIdx) {
    int dist[MAX_NODES];
    int visited[MAX_NODES];
    int previous[MAX_NODES];
    int srcNode = travelers[tIdx].sourceNode;
    int dstNode = travelers[tIdx].destinationNode;

    for (int i = 0; i < nodeCount; i++) {
        dist[i] = INF;
        visited[i] = 0;
        previous[i] = -1;
    }
    dist[srcNode] = 0;

    for (int count = 0; count < nodeCount; count++) {
        int current = -1;
        int bestDistance = INF;
        for (int i = 0; i < nodeCount; i++) {
            if (!visited[i] && dist[i] < bestDistance) {
                bestDistance = dist[i];
                current = i;
            }
        }
        if (current == -1) break;
        visited[current] = 1;

        for (int i = 0; i < edgeCount; i++) {
            if (edges[i].src == current) {
                int neighbor = edges[i].dst;
                int weight = edges[i].weight;
                if (dist[current] + weight < dist[neighbor]) {
                    dist[neighbor] = dist[current] + weight;
                    previous[neighbor] = current;
                }
            }
        }
    }

    travelers[tIdx].pathLength = 0;
    travelers[tIdx].totalDistance = dist[dstNode];

    if (dist[dstNode] == INF) return;

    int tempPath[MAX_NODES];
    int tempLength = 0;
    int current = dstNode;
    while (current != -1) {
        tempPath[tempLength++] = current;
        current = previous[current];
    }

    for (int i = tempLength - 1; i >= 0; i--) {
        travelers[tIdx].shortestPath[travelers[tIdx].pathLength++] = tempPath[i];
    }
}

int parse_scheduler_type(int argc, char *argv[], char **fileName) {
    if (argc != 4) {
        printf("Usage: ./sim -schd fcfs <file_name>\n");
        printf("Usage: ./sim -schd sjf <file_name>\n");
        return 0;
    }

    if (strcmp(argv[1], "-schd") != 0) {
        printf("Error: missing -schd option\n");
        return 0;
    }

    if (strcmp(argv[2], "fcfs") == 0) {
        schedulerType = SCHED_FCFS;
    } else if (strcmp(argv[2], "sjf") == 0) {
        schedulerType = SCHED_SJF;
    } else {
        printf("Error: unknown scheduler type\n");
        printf("Use: fcfs or sjf\n");
        return 0;
    }

    *fileName = argv[3];
    return 1;
}

const char* getSchedulerName() {
    if (schedulerType == SCHED_FCFS) {
        return "FCFS";
    }

    return "SJF";
}

void schedule_intersection(int intersection)
{
    IntersectionQueue *queue =
        get_waiting_queue(intersection);

    QueueNode *selected =
        choose_next_process(queue, schedulerType);

    if (selected != NULL)
    {
        printf(
            "Scheduler selected traveler %d for node %d\n",
            selected->travelerIndex,
            intersection
        );
    }
}

int main(int argc, char *argv[]) {
    char *fileName;

    if (!parse_scheduler_type(argc, argv, &fileName)) {
        return 1;
    }

    FILE *fp = fopen(fileName, "r");

    if (fp == NULL) {
        printf("file error\n");
        return 1;
    }

    fscanf(fp, "%d %d", &nodeCount, &edgeCount);
    for (int i = 0; i < edgeCount; i++) {
        fscanf(fp, "%d %d %d", &edges[i].src, &edges[i].dst, &edges[i].weight);
    }

    fscanf(fp, "%d", &travelerCount);
    for (int i = 0; i < travelerCount; i++) {
        fscanf(fp, "%d %d", &travelers[i].sourceNode, &travelers[i].destinationNode);

        // set defult values for each traveler
        travelers[i].currentPathIndex = 0;
        travelers[i].currentJump = 0;
        travelers[i].animationFinished = 0;
        travelers[i].waitingForNode = -1;
        travelers[i].movingFromNode = -1;
        travelers[i].movingToNode = -1;
        travelers[i].insideNode = -1;
        travelers[i].guiStatus = GUI_STATUS_READY;
        travelers[i].insideStartTime = 0;
        travelers[i].insideReleaseSent = 0;
        travelers[i].leaveAckSent = 0;
        travelers[i].waitingStartTime = 0;
        travelers[i].totalWaitingTime = 0;
        travelers[i].waitingAckSent = 0;
        travelers[i].color = travelerColors[i % MAX_TRAVELERS];

                // Milestone 5: child will calculate its own path
        travelers[i].pathLength = 0;

    }
    fclose(fp);

    calculatePositions();
    init_intersection_queues();

    nodeLocks = mmap(NULL,
                 sizeof(sem_t) * nodeCount,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS,
                 -1,
                 0);

    if (nodeLocks == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    for (int i = 0; i < nodeCount; i++) {
        if (sem_init(&nodeLocks[i], 1, 1) == -1) {
            perror("sem_init");
            return 1;
        }
    }

    /* Per-traveler sync semaphores. Each protocol step has its own semaphore
       so acknowledgements from one node cannot accidentally unlock another. */
    leaveSync = createSemaphoreArray(travelerCount, 0, "leaveSync");
    outsideSync = createSemaphoreArray(travelerCount, 0, "outsideSync");
    waitShownSync = createSemaphoreArray(travelerCount, 0, "waitShownSync");
    insideDoneSync = createSemaphoreArray(travelerCount, 0, "insideDoneSync");
    schedulerSync = createSemaphoreArray(travelerCount, 0, "schedulerSync");

    if (leaveSync == NULL || outsideSync == NULL ||
        waitShownSync == NULL || insideDoneSync == NULL || schedulerSync == NULL) {
        return 1;
    }

    childrenReady = mmap(NULL,
                 sizeof(sem_t),
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS,
                 -1,
                 0);

    if (childrenReady == MAP_FAILED) {
        perror("mmap childrenReady");
        return 1;
    }

    if (sem_init(childrenReady, 1, 0) == -1) {
        perror("sem_init childrenReady");
        return 1;
    }

      // Initial positions are based on the source node.
      // The child will calculate the path and send updates to the parent.
    for (int i = 0; i < travelerCount; i++) {

        travelers[i].entityX = positions[travelers[i].sourceNode].x;
        travelers[i].entityY = positions[travelers[i].sourceNode].y;

        travelers[i].pathLength = 1;
        travelers[i].shortestPath[0] = travelers[i].sourceNode;
    }

    for (int i = 0; i < travelerCount; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }

    // --- יצירת תהליכי הבנים (Fork Processes) ---
    for (int i = 0; i < travelerCount; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }
        if (pid == 0) {
            sigset_t startMask;
            sigset_t oldMask;

            for (int p = 0; p < travelerCount; p++) {
                close(pipes[p][0]);
                if (p != i) {
                    close(pipes[p][1]);
                }
            }

            sigemptyset(&startMask);
            sigaddset(&startMask, SIGUSR1);
            sigprocmask(SIG_BLOCK, &startMask, &oldMask);
            signal(SIGUSR1, startHandler);

            runDijkstraForTraveler(i);
            sem_post(childrenReady);

            while (!startFlag) {
                sigsuspend(&oldMask);
            }
            sigprocmask(SIG_UNBLOCK, &startMask, NULL);

            //  (Child Process)
            // printf("[%d] started\n", getpid());
            // fflush(stdout);

            if (travelers[i].pathLength == 0) {
                sendMessage(pipes[i][1], i, travelers[i].sourceNode, -1, STATUS_FINISHED, 1);
                close(pipes[i][1]);
                exit(0);
            }

            int currentNode = travelers[i].shortestPath[0];

            sendMessage(pipes[i][1], i, currentNode, currentNode, STATUS_WAITING, 0);
            waitSemaphore(&schedulerSync[i]);

            sendMessage(pipes[i][1], i, currentNode, currentNode, STATUS_ENTERED, travelers[i].pathLength <= 1);
            waitSemaphore(&insideDoneSync[i]);

            if (travelers[i].pathLength <= 1) {
                sendMessage(pipes[i][1], i, currentNode, -1, STATUS_FINISHED, 1);
                close(pipes[i][1]);
                exit(0);
            }

            for (int p = 0; p < travelers[i].pathLength - 1; p++) {
                int nodeToEnter = travelers[i].shortestPath[p + 1];
                int isLastNode = (p + 1 == travelers[i].pathLength - 1);

                sendMessage(pipes[i][1], i, currentNode, nodeToEnter, STATUS_MOVING, 0);
                waitSemaphore(&leaveSync[i]);
                waitSemaphore(&outsideSync[i]);

                sendMessage(pipes[i][1], i, currentNode, nodeToEnter, STATUS_WAITING, 0);
                waitSemaphore(&schedulerSync[i]);

                sendMessage(pipes[i][1], i, currentNode, nodeToEnter, STATUS_ENTERED, isLastNode);
                waitSemaphore(&insideDoneSync[i]);

                if (isLastNode) {
                    sendMessage(pipes[i][1], i, nodeToEnter, -1, STATUS_FINISHED, 1);
                } else {
                    currentNode = nodeToEnter;
                }
            }
            close(pipes[i][1]);

            /* Child finished its path and exits cleanly. */
            exit(0);
        } else {
            close(pipes[i][1]);
            fcntl(pipes[i][0], F_SETFL, O_NONBLOCK);


             // Parent process code stores the PID of the child
            travelers[i].pid = pid;
        }
    }

    for (int i = 0; i < travelerCount; i++) {
        waitSemaphore(childrenReady);
    }

    // The father is running the GUI and managing the game
    InitWindow(1000, 700, "Milestone 6 - Node Synchronization");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
       Rectangle button = {40, 30, 120, 40};

       for (int i = 0; i < travelerCount; i++) {
          Message msg;

          while (read(pipes[i][0], &msg, sizeof(Message)) == sizeof(Message)) {
              int idx = msg.travelerIndex;

              if (msg.status == STATUS_MOVING) {
                  printf("Traveler %d MOVING to node %d\n", idx, msg.nextNode);

                  travelers[idx].waitingForNode = -1;
                  travelers[idx].movingFromNode = msg.currentNode;
                  travelers[idx].movingToNode = msg.nextNode;
                  travelers[idx].insideNode = -1;
                  travelers[idx].guiStatus = GUI_STATUS_MOVING;
                  travelers[idx].insideReleaseSent = 0;
                  travelers[idx].leaveAckSent = 0;
                  travelers[idx].waitingAckSent = 0;

                  if (msg.nextNode != -1 && travelers[idx].pathLength < MAX_NODES) {
                      int last = travelers[idx].pathLength - 1;

                      if (travelers[idx].shortestPath[last] != msg.nextNode) {
                          travelers[idx].shortestPath[travelers[idx].pathLength] = msg.nextNode;
                          travelers[idx].pathLength++;
                      }
                  }
              } else if (msg.status == STATUS_WAITING) {
                  Position outside = outsideNodePosition(msg.currentNode, msg.nextNode);

                  add_to_intersection_queue(msg.nextNode, idx, msg.currentNode);

                  travelers[idx].waitingForNode = msg.nextNode;
                  travelers[idx].movingFromNode = msg.currentNode;
                  travelers[idx].movingToNode = -1;
                  travelers[idx].insideNode = -1;
                  travelers[idx].guiStatus = GUI_STATUS_WAITING;
                  travelers[idx].waitingStartTime = GetTime();
                  travelers[idx].waitingAckSent = 0;
                  travelers[idx].entityX = outside.x;
                  travelers[idx].entityY = outside.y;

                  printf("Traveler %d WAITING outside node %d\n", idx, msg.nextNode);
              } else if (msg.status == STATUS_ENTERED) {
                  if (travelers[idx].waitingStartTime > 0) {
                      double waited = GetTime() - travelers[idx].waitingStartTime;
                      travelers[idx].totalWaitingTime += waited;
                      printf("Traveler %d waited %.2f seconds before entering node %d. Total waiting time: %.2f seconds\n",
                             idx, waited, msg.nextNode, travelers[idx].totalWaitingTime);
                      travelers[idx].waitingStartTime = 0;
                  }

                  remove_from_intersection_queue(msg.nextNode, idx);

                  travelers[idx].waitingForNode = -1;
                  travelers[idx].movingFromNode = -1;
                  travelers[idx].movingToNode = -1;
                  travelers[idx].insideNode = msg.nextNode;
                  travelers[idx].guiStatus = GUI_STATUS_INSIDE;
                  travelers[idx].insideStartTime = GetTime();
                  travelers[idx].insideReleaseSent = 0;
                  travelers[idx].waitingAckSent = 0;
                  travelers[idx].entityX = positions[msg.nextNode].x;
                  travelers[idx].entityY = positions[msg.nextNode].y;

                  printf("Traveler %d ENTERED node %d%s\n",
                         idx, msg.nextNode, msg.finished ? " (destination)" : "");
              } else if (msg.status == STATUS_FINISHED) {
                  travelers[idx].waitingForNode = -1;
                  travelers[idx].movingFromNode = -1;
                  travelers[idx].movingToNode = -1;
                  travelers[idx].insideNode = -1;
                  travelers[idx].guiStatus = GUI_STATUS_FINISHED;
                  travelers[idx].animationFinished = 1;

                  if (msg.currentNode >= 0) {
                      travelers[idx].entityX = positions[msg.currentNode].x;
                      travelers[idx].entityY = positions[msg.currentNode].y;
                  }

                  printf("[PID=%d] finished. Traveler %d total waiting time: %.2f seconds\n",
                         msg.pid, idx, travelers[idx].totalWaitingTime);
              }

              fflush(stdout);
          }
       }

        if (CheckCollisionPointRec(GetMousePosition(), button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isPlaying = !isPlaying;
            if (isPlaying && !startSent) {
                lastMoveTime = GetTime();

                for (int j = 0; j < travelerCount; j++) {
                    kill(travelers[j].pid, SIGUSR1);
                }
                startSent = 1;
            }
        }

        // --- Update the movement of all passengers simultaneously ---
        if (isPlaying) {
            double currentTime = GetTime();
            int allFinished = 1;

            for (int i = 0; i < travelerCount; i++) {
                if (!travelers[i].animationFinished) {
                    allFinished = 0;
                }

                if (travelers[i].guiStatus == GUI_STATUS_INSIDE &&
                    !travelers[i].insideReleaseSent &&
                    currentTime - travelers[i].insideStartTime >= 1.0) {
                    int leavingNode = travelers[i].insideNode;
                    travelers[i].insideReleaseSent = 1;
                    printf("Traveler %d LEAVING node %d\n", i, leavingNode);
                    fflush(stdout);
                    if (leavingNode >= 0 && leavingNode < nodeCount) {
                        sem_post(&nodeLocks[leavingNode]);
                    }
                    sem_post(&insideDoneSync[i]);
                }

                if (travelers[i].animationFinished ||
                    travelers[i].movingFromNode == -1 ||
                    travelers[i].movingToNode == -1) {
                    continue;
                }
                if (travelers[i].waitingForNode != -1) {
                    continue;
                }

                // The logic of the movement is every 300 milliseconds (0.3 seconds)
                if (currentTime - lastMoveTime >= 0.3) {
                    int src = travelers[i].movingFromNode;
                    int dst = travelers[i].movingToNode;
                    int edgeWeight = 1;

                    for (int e = 0; e < edgeCount; e++) {
                        if (edges[e].src == src && edges[e].dst == dst) {
                            edgeWeight = edges[e].weight;
                            break;
                        }
                    }

                    travelers[i].currentJump++;
                    float t = (float)travelers[i].currentJump / edgeWeight;
                    Position outside = outsideNodePosition(src, dst);

                    travelers[i].entityX = positions[src].x + t * (outside.x - positions[src].x);
                    travelers[i].entityY = positions[src].y + t * (outside.y - positions[src].y);

                    if (!travelers[i].leaveAckSent) {
                        travelers[i].leaveAckSent = 1;
                        sem_post(&leaveSync[i]);
                    }

                    if (travelers[i].currentJump >= edgeWeight) {
                        if (travelers[i].currentPathIndex < travelers[i].pathLength - 1 &&
                            travelers[i].shortestPath[travelers[i].currentPathIndex + 1] == dst) {
                            travelers[i].currentPathIndex++;
                        }
                        travelers[i].currentJump = 0;
                        travelers[i].movingFromNode = -1;
                        travelers[i].movingToNode = -1;
                        travelers[i].entityX = outside.x;
                        travelers[i].entityY = outside.y;

                        sem_post(&outsideSync[i]);
                    }
                }
            }

            grant_waiting_travelers_by_scheduler();

            if (currentTime - lastMoveTime >= 0.3) {
                lastMoveTime = currentTime;
            }

            if (startSent && allFinished) {
                isPlaying = 0;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw a play button
        DrawRectangleRec(button, LIGHTGRAY);
        DrawRectangleLinesEx(button, 2, DARKGRAY);
        DrawText(isPlaying ? "STOP" : "PLAY", 75, 40, 22, BLACK);

        DrawText("IPC Simulation (Milestone 7)", 230, 30, 28, DARKBLUE);
        DrawText("BLACK / WAIT = Waiting outside node", 600, 30, 20, BLACK);
        DrawText(TextFormat("Current Scheduler: %s", getSchedulerName()), 230, 65, 22, MAROON);

        // Drawing lines (edges) and weights
        for (int i = 0; i < edgeCount; i++) {
            int src = edges[i].src;
            int dst = edges[i].dst;
            drawArrow(positions[src].x, positions[src].y, positions[dst].x, positions[dst].y, BLACK);

            float midX = (positions[src].x + positions[dst].x) / 2;
            float midY = (positions[src].y + positions[dst].y) / 2;
            float dx = positions[dst].x - positions[src].x;
            float dy = positions[dst].y - positions[src].y;
            float length = sqrtf(dx * dx + dy * dy);

            float offsetX = (length != 0) ? (-dy / length * 18) : 0;
            float offsetY = (length != 0) ? (dx / length * 18) : 0;

            DrawCircle(midX + offsetX, midY + offsetY, 15, RAYWHITE);
            DrawText(TextFormat("%d", edges[i].weight), midX + offsetX - 6, midY + offsetY - 10, 20, RED);
        }

        // Drawing of fixed graph stations
        for (int i = 0; i < nodeCount; i++) {
            DrawCircle(positions[i].x, positions[i].y, NODE_RADIUS, BLUE);
            DrawCircleLines(positions[i].x, positions[i].y, NODE_RADIUS, DARKBLUE);
            DrawText(TextFormat("%d", i), positions[i].x - 6, positions[i].y - 11, 22, WHITE);
        }

        // Draw all the moving objects (each object in its own color)
        for (int i = 0; i < travelerCount; i++) {
            if (travelers[i].pathLength > 0) {

                Color drawColor = travelers[i].color;

                if (travelers[i].waitingForNode != -1) {
                    drawColor = BLACK;
                }

                DrawCircle(travelers[i].entityX, travelers[i].entityY, 14, drawColor);

                if (travelers[i].waitingForNode != -1) {
                    DrawText("WAIT", travelers[i].entityX - 20, travelers[i].entityY - 35, 14, RED);
                }

                DrawCircleLines(travelers[i].entityX, travelers[i].entityY, 14, BLACK);
            }
        }

        // Display a small side panel showing the status of each child (PID)
        DrawRectangle(40, 90, 420, travelerCount * 25 + 10, Fade(LIGHTGRAY, 0.8f));
        for (int i = 0; i < travelerCount; i++) {
            DrawCircle(55, 105 + i * 25, 7, travelers[i].color);
            DrawText(TextFormat("PID: %d  %s  WT: %.2f",
                                travelers[i].pid,
                                statusText(travelers[i].guiStatus),
                                travelers[i].totalWaitingTime),
                     70, 95 + i * 25, 16, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();
    clear_intersection_queues();

    for (int i = 0; i < travelerCount; i++) {
        if (!travelers[i].animationFinished) {
            kill(travelers[i].pid, SIGTERM);
        }
    }

    // Wait for all children before releasing shared synchronization objects.
    for (int i = 0; i < travelerCount; i++) {
        waitpid(travelers[i].pid, NULL, 0);
    }

    for (int i = 0; i < nodeCount; i++) {
        sem_destroy(&nodeLocks[i]);
    }

    for (int i = 0; i < travelerCount; i++) {
        sem_destroy(&leaveSync[i]);
        sem_destroy(&outsideSync[i]);
        sem_destroy(&waitShownSync[i]);
        sem_destroy(&insideDoneSync[i]);
        sem_destroy(&schedulerSync[i]);
    }
    sem_destroy(childrenReady);

    munmap(nodeLocks, sizeof(sem_t) * nodeCount);
    munmap(leaveSync, sizeof(sem_t) * travelerCount);
    munmap(outsideSync, sizeof(sem_t) * travelerCount);
    munmap(waitShownSync, sizeof(sem_t) * travelerCount);
    munmap(insideDoneSync, sizeof(sem_t) * travelerCount);
    munmap(schedulerSync, sizeof(sem_t) * travelerCount);
    munmap(childrenReady, sizeof(sem_t));

    printf("\n=== Waiting Time Statistics (%s) ===\n", getSchedulerName());
    for (int i = 0; i < travelerCount; i++) {
        printf("Traveler %d total waiting time: %.2f seconds\n",
               i, travelers[i].totalWaitingTime);
    }

    printf("Parent Process finished safely.\n");
    return 0;
}


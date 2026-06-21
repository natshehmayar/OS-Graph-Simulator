#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <unistd.h>     // fork(), sleep()
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // kill()

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
    int isWaiting;
    double waitStartTime;
    int animationFinished;
    Color color; // a unique color for every traveler
} Traveler;

typedef struct {
    int travelerIndex;
    int currentNode;
    int nextNode;
    int finished;
    pid_t pid;
} Message;

Edge edges[MAX_EDGES];
Position positions[MAX_NODES];
Traveler travelers[MAX_TRAVELERS];
int pipes[MAX_TRAVELERS][2];

int nodeCount;
int edgeCount;
int travelerCount; // number of travelers (sons)

int isPlaying = 0;
double lastMoveTime = 0;

// color matrix for distinguishing travelers
Color travelerColors[MAX_TRAVELERS] = {
    ORANGE, GOLD, LIME, MAROON, PURPLE, VIOLET, DARKGREEN, PINK, MAGENTA, BEIGE
};

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./sim input.txt\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
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
        travelers[i].isWaiting = 0;
        travelers[i].animationFinished = 0;
        travelers[i].color = travelerColors[i % MAX_TRAVELERS];

        // calculate the path in advance before (fork)
        runDijkstraForTraveler(i);

        if (travelers[i].pathLength > 0) {
            travelers[i].entityX = positions[travelers[i].shortestPath[0]].x;
            travelers[i].entityY = positions[travelers[i].shortestPath[0]].y;
        }
    }
    fclose(fp);

    calculatePositions();

// Update the correct initial coordinates after calculating the circular positions
    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].pathLength > 0) {
            travelers[i].entityX = positions[travelers[i].shortestPath[0]].x;
            travelers[i].entityY = positions[travelers[i].shortestPath[0]].y;
        }
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
            close(pipes[i][0]);

            //  (Child Process)
            printf("[%d] started\n", getpid()); // הדפסה נדרשת בטרמינל
            fflush(stdout);

            Message msg;

            msg.travelerIndex = i;
            msg.currentNode = travelers[i].sourceNode;
            msg.nextNode = -1;
            msg.finished = 0;
            msg.pid = getpid();

            write(pipes[i][1], &msg, sizeof(Message));
            close(pipes[i][1]);

            while (1) {
                sleep(1);
            }
            exit(0);
        } else {
            close(pipes[i][1]);

             // Parent process code stores the PID of the child
            travelers[i].pid = pid;
        }
    }

    // The father is running the GUI and managing the game
    InitWindow(1000, 700, "Milestone 4 - Multi-Process Travelers");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Rectangle button = {40, 30, 120, 40};


        if (CheckCollisionPointRec(GetMousePosition(), button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isPlaying = !isPlaying;
            if (isPlaying) {
                lastMoveTime = GetTime();
            }
        }

        // --- Update the movement of all passengers simultaneously ---
        if (isPlaying) {
            double currentTime = GetTime();
            int allFinished = 1;

            for (int i = 0; i < travelerCount; i++) {
                if (travelers[i].animationFinished || travelers[i].pathLength <= 1) {
                    continue;
                }
                allFinished = 0; // There is at least one traveler who has not yet finished

                // The logic of waiting at the station (1 second per passenger)
                if (travelers[i].isWaiting) {
                    if (currentTime - travelers[i].waitStartTime >= 1.0) {
                        travelers[i].isWaiting = 0;
                    } else {
                        continue;
                    }
                }

                // The logic of the movement is every 300 milliseconds (0.3 seconds)
                if (currentTime - lastMoveTime >= 0.3) {
                    int src = travelers[i].shortestPath[travelers[i].currentPathIndex];
                    int dst = travelers[i].shortestPath[travelers[i].currentPathIndex + 1];
                    int edgeWeight = 1;

                    for (int e = 0; e < edgeCount; e++) {
                        if (edges[e].src == src && edges[e].dst == dst) {
                            edgeWeight = edges[e].weight;
                            break;
                        }
                    }

                    travelers[i].currentJump++;
                    float t = (float)travelers[i].currentJump / edgeWeight;

                    travelers[i].entityX = positions[src].x + t * (positions[dst].x - positions[src].x);
                    travelers[i].entityY = positions[src].y + t * (positions[dst].y - positions[src].y);

                    if (travelers[i].currentJump >= edgeWeight) {
                        travelers[i].currentPathIndex++;
                        travelers[i].currentJump = 0;

                        if (travelers[i].currentPathIndex >= travelers[i].pathLength - 1) {
                            travelers[i].animationFinished = 1;
                            // Upon the traveler's arrival at the destination, the father sends a signal to finalize the son's process
                            kill(travelers[i].pid, SIGKILL);
                        } else {
                            travelers[i].isWaiting = 1;
                            travelers[i].waitStartTime = currentTime;
                        }
                    }
                }
            }

            if (currentTime - lastMoveTime >= 0.3) {
                lastMoveTime = currentTime;
            }

            if (allFinished) {
                isPlaying = 0;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw a play button
        DrawRectangleRec(button, LIGHTGRAY);
        DrawRectangleLinesEx(button, 2, DARKGRAY);
        DrawText(isPlaying ? "STOP" : "PLAY", 75, 40, 22, BLACK);

        DrawText("Multi-Process Simulation (Milestone 4)", 230, 30, 28, DARKBLUE);

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
                DrawCircle(travelers[i].entityX, travelers[i].entityY, 14, travelers[i].color);
                DrawCircleLines(travelers[i].entityX, travelers[i].entityY, 14, BLACK);
            }
        }

        // Display a small side panel showing the status of each child (PID)
        DrawRectangle(40, 90, 220, travelerCount * 25 + 10, Fade(LIGHTGRAY, 0.8f));
        for (int i = 0; i < travelerCount; i++) {
            DrawCircle(55, 105 + i * 25, 7, travelers[i].color);
            DrawText(TextFormat("Child PID: %d %s", travelers[i].pid, travelers[i].animationFinished ? "(Arrived)" : ""), 70, 95 + i * 25, 16, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();

    //  (Zombie Processes)
    for (int i = 0; i < travelerCount; i++) {
        kill(travelers[i].pid, SIGKILL); // To confirm
        waitpid(travelers[i].pid, NULL, 0);
    }

    printf("Parent Process finished safely.\n");
    return 0;
}
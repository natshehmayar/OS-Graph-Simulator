#include <stdio.h>
#include <math.h>
#include <raylib.h>

#define MAX_NODES 20
#define MAX_EDGES 100
#define NODE_RADIUS 28

typedef struct
{
    int src;
    int dst;
    int weight;
} Edge;

typedef struct
{
    float x;
    float y;
} Position;

Edge edges[MAX_EDGES];
Position positions[MAX_NODES];

int nodeCount;
int edgeCount;
int sourceNode;
int destinationNode;

#define INF 1000000000

int shortestPath[MAX_NODES];
int pathLength;
int totalDistance;

int isPlaying = 0;
int animationFinished = 0;

int currentPathIndex = 0;
int currentJump = 0;
double lastMoveTime = 0;
double waitStartTime = 0;
int isWaiting = 0;

float entityX;
float entityY;

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

void drawArrow(float startX, float startY, float endX, float endY, Color color){float dx = endX - startX;float dy = endY - startY;float length = sqrtf(dx * dx + dy * dy);

    if (length == 0) return;

    float ux = dx / length;
    float uy = dy / length;

    float realStartX = startX + ux * NODE_RADIUS;
    float realStartY = startY + uy * NODE_RADIUS;

    float realEndX = endX - ux * NODE_RADIUS;
    float realEndY = endY - uy * NODE_RADIUS;

    DrawLineEx(
    (Vector2){realStartX, realStartY},
    (Vector2){realEndX, realEndY},
    2,
    color
);

    float arrowSize = 12;

    Vector2 tip = {realEndX, realEndY};

    Vector2 left = {
        realEndX - arrowSize * ux + arrowSize * 0.5f * uy,
        realEndY - arrowSize * uy - arrowSize * 0.5f * ux
    };

    Vector2 right = {
        realEndX - arrowSize * ux - arrowSize * 0.5f * uy,
        realEndY - arrowSize * uy + arrowSize * 0.5f * ux
    };

    DrawTriangle(tip, left, right, color);

}

void runDijkstra() {
    int dist[MAX_NODES];
    int visited[MAX_NODES];
    int previous[MAX_NODES];

    for (int i = 0; i < nodeCount; i++) {
        dist[i] = INF;
        visited[i] = 0;
        previous[i] = -1;
    }

    dist[sourceNode] = 0;

    for (int count = 0; count < nodeCount; count++) {
        int current = -1;
        int bestDistance = INF;

        for (int i = 0; i < nodeCount; i++) {
            if (!visited[i] && dist[i] < bestDistance) {
                bestDistance = dist[i];
                current = i;
            }
        }

        if (current == -1) {
            break;
        }

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

    pathLength = 0;
    totalDistance = dist[destinationNode];

    if (dist[destinationNode] == INF) {
        return;
    }

    int tempPath[MAX_NODES];
    int tempLength = 0;
    int current = destinationNode;

    while (current != -1) {
        tempPath[tempLength] = current;
        tempLength++;
        current = previous[current];
    }

    for (int i = tempLength - 1; i >= 0; i--) {
        shortestPath[pathLength] = tempPath[i];
        pathLength++;
    }

}

int main(int argc, char *argv[]) {
    if (argc != 2)
    {
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
        fscanf(fp, "%d %d %d",
               &edges[i].src,
               &edges[i].dst,
               &edges[i].weight);
    }

    fscanf(fp, "%d %d", &sourceNode, &destinationNode);

    fclose(fp);

    calculatePositions();
    runDijkstra();

    if (pathLength > 0) {
        entityX = positions[shortestPath[0]].x;
        entityY = positions[shortestPath[0]].y;
    }

    InitWindow(1000, 700, "Milestone 2 - Graph GUI");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Rectangle button = {40, 30, 120, 40};

        if (CheckCollisionPointRec(GetMousePosition(), button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

            if (!animationFinished) {

                isPlaying = !isPlaying;

                if (isPlaying) {
                    lastMoveTime = GetTime();
                }
            }
        }

        if (isPlaying && !animationFinished && pathLength > 1) {
            double currentTime = GetTime();

            if (isWaiting) {
                if (currentTime - waitStartTime >= 1.0) {
                    isWaiting = 0;
                } else {
                    lastMoveTime = currentTime;

                }
            }

            if (currentTime - lastMoveTime >= 0.3) {
                int src = shortestPath[currentPathIndex];
                int dst = shortestPath[currentPathIndex + 1];

                int edgeWeight = 1;

                for (int i = 0; i < edgeCount; i++) {
                    if (edges[i].src == src && edges[i].dst == dst) {
                        edgeWeight = edges[i].weight;
                        break;
                    }
                }

                currentJump++;

                float t = (float)currentJump / edgeWeight;

                entityX = positions[src].x + t * (positions[dst].x - positions[src].x);
                entityY = positions[src].y + t * (positions[dst].y - positions[src].y);





                if (currentJump >= edgeWeight) {
                currentPathIndex++;
                currentJump = 0;

                if (currentPathIndex >= pathLength - 1) {
                    animationFinished = 1;
                    isPlaying = 0;
                } else {
                    isWaiting = 1;
                    waitStartTime = currentTime;
                }
            }

            lastMoveTime = currentTime;
        }
    }

    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawRectangleRec(button, LIGHTGRAY);
    DrawRectangleLinesEx(button, 2, DARKGRAY);
    DrawText(isPlaying ? "STOP" : "PLAY", 75, 40, 22, BLACK);

    DrawText("Directed Weighted Graph", 330, 30, 28, DARKBLUE);

    if (pathLength == 0) {
        DrawText("No path found", 420, 65, 24, RED);
    } else {
        DrawText(TextFormat("Shortest distance: %d", totalDistance), 370, 65, 22, DARKGREEN);
    }

    for (int i = 0; i < edgeCount; i++) {
        int src = edges[i].src;
        int dst = edges[i].dst;

        drawArrow(
positions[src].x,
positions[src].y,
positions[dst].x,
positions[dst].y,
BLACK

);

        float midX = (positions[src].x + positions[dst].x) / 2;
        float midY = (positions[src].y + positions[dst].y) / 2;

        float dx = positions[dst].x - positions[src].x;
        float dy = positions[dst].y - positions[src].y;
        float length = sqrtf(dx * dx + dy * dy);

        float offsetX = 0;
        float offsetY = 0;

        if (length != 0) {
            offsetX = -dy / length * 18;
            offsetY = dx / length * 18;
        }

        float weightX = midX + offsetX;
        float weightY = midY + offsetY;

        DrawCircle(weightX, weightY, 15, RAYWHITE);
        DrawText(TextFormat("%d", edges[i].weight),
                 weightX - 6,
                 weightY - 10,
                 20,
                 RED);
    }

    if (pathLength > 1) {
        for (int i = 0; i < pathLength - 1; i++) {
            int src = shortestPath[i];
            int dst = shortestPath[i + 1];

            drawArrow(
                positions[src].x,
                positions[src].y,
                positions[dst].x,
                positions[dst].y,
                GREEN
            );
        }
    }

    for (int i = 0; i < nodeCount; i++) {
        DrawCircle(positions[i].x, positions[i].y, NODE_RADIUS, BLUE);
        DrawCircleLines(positions[i].x, positions[i].y, NODE_RADIUS, DARKBLUE);

        DrawText(TextFormat("%d", i),
                 positions[i].x - 6,
                 positions[i].y - 11,
                 22,
                 WHITE);
    }

    if (pathLength > 0) {
        DrawCircle(entityX, entityY, 14, ORANGE);
        DrawCircleLines(entityX, entityY, 14, BROWN);
    }

    if (animationFinished) {
        DrawText("Arrived at destination!", 330, 110, 24, DARKGREEN);

                }

    EndDrawing();
}

CloseWindow();

return 0;

}
#include <stdio.h>
#include <math.h>
#include <raylib.h>

#define MAX_NODES 20
#define MAX_EDGES 100
#define NODE_RADIUS 28

typedef struct {
    int src;
    int dst;
    int weight;
} Edge;

typedef struct {
    float x;
    float y;
} Position;

Edge edges[MAX_EDGES];
Position positions[MAX_NODES];

int nodeCount;
int edgeCount;

void calculatePositions() {
    float centerX = 500;
    float centerY = 350;
    float radius = 220;

    for (int i = 0; i < nodeCount; i++) {
        float angle = (2 * PI * i) / nodeCount - PI / 2;

        positions[i].x = centerX + radius * cosf(angle);
        positions[i].y = centerY + radius * sinf(angle);
    }
}

void drawArrow(float startX, float startY, float endX, float endY) {
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

    DrawLineEx(
        (Vector2){realStartX, realStartY},
        (Vector2){realEndX, realEndY},
        2,
        BLACK
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

    DrawTriangle(tip, left, right, BLACK);
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
        fscanf(fp, "%d %d %d",
               &edges[i].src,
               &edges[i].dst,
               &edges[i].weight);
    }

    fclose(fp);

    calculatePositions();

    InitWindow(1000, 700, "Milestone 2 - Graph GUI");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText("Directed Weighted Graph", 330, 30, 28, DARKBLUE);

        for (int i = 0; i < edgeCount; i++) {
            int src = edges[i].src;
            int dst = edges[i].dst;

            drawArrow(
                positions[src].x,
                positions[src].y,
                positions[dst].x,
                positions[dst].y
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

        for (int i = 0; i < nodeCount; i++) {
            DrawCircle(positions[i].x, positions[i].y, NODE_RADIUS, BLUE);
            DrawCircleLines(positions[i].x, positions[i].y, NODE_RADIUS, DARKBLUE);

            DrawText(TextFormat("%d", i),
                     positions[i].x - 6,
                     positions[i].y - 11,
                     22,
                     WHITE);
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
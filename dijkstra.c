#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>     // מיועד ל-fork(), sleep()
#include <sys/types.h>  // מיועד ל-waitpid()
#include <sys/wait.h>
#include <signal.h>     // מיועד ל-kill()
#include <raylib.h>



// Node structure for adjacency list
typedef struct Node{
    int dest;   //  destination vertex
    int weight;  // edge wight
    struct Node* next;   // pointer to next in list
} Node;

Node** graph;  // array of adjacency lists

typedef struct
{
    pid_t pid;
    int src;
    int dest;
    int* path;
    int path_size;
    int current_step;
    Color color;
} Traveler;

int* calculate_dijkstra (int start, int end, int N, int* path_count, int* total_dist)
{
    int *dist = malloc(N * sizeof(int));
    int *visited = malloc(N , sizeof(int));
    int *parent = malloc(N * sizeof(int));

    if (!dist || !visited || !parent)
    {
        printf("Memory allocation failed in Dijksta\n");
        return NULL;
    }

    for (int i = 0; i < N; i++)
    {
        dist[i] = INT_MAX;
        visited[i] = 0;
        parent[i] = -1;
    }

    dist[start] = 0;

    if (start == end) {
        *path_count = 1;
        *total_dist = 0:
        int *path = malloc(sizeof(int));
        path[0] = start;
        free(dist); free(visited); free(parent);
        return path;
    }

    for (int i = 0; i < N - 1; i++) {
        int min = INT_MAX, u = -1;

        for (int j = 0; j < N; j++) {
            if (!visited[j] && dist[j] < min)
            {
                min = dist[j];
                u = j;
            }
        }

        if (u == -1) break;

        visited[u] = 1;
        Node* temp = graph[u];
        while (temp)
        {
            int v = temp->dest;
            int weight = temp->weight;

            if (!visited[v] && dist[u] != INT_MAX && dist[u] + weight < dist[v])
            {
                dist[v] = dist[u] + weight;
                parent[v] = u;
            }
            temp = temp->next;
        }
    }

    if (dist[end] == INT_MAX)
    {
        free(dist); free(visited); free(parent);
        return NULL;
    }

    int *temp_path = malloc(N * sizeof(int));
    int count = 0;
    for (int v = end; v != -1; v = parent[v])
    {
        temp_path[count++] = v;
    }

    int *path = malloc(count * sizeof(int));
    int idx = 0;
    for (int i = count - 1; i >= 0; i--)
    {
        path [idx++] = temp_path[i];
    }

    *path_count = count;
    *total_dist = dist[end];

    free(temp_path);
    free(dist);
    free(visited);
    free(parent);

    return path;
}

int main(int argc, const char * argv[]) {

    //  Check correct number of arguments
    if (argc != 2) {
        printf("Usage: ./dijkstra <file>\n");
        return 1;
    }
    // open input file
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        printf("Error opening file!\n");
        return 1;
    }


    int N, M;
    // Read number of vertices (N) and edges (M)
    if (fscanf(file, "%d %d", &N, &M) != 2) {
        printf("Invalid input\n");
        return 1;
    }

    // Allocate graph (array of pointers)
    graph = (Node**) malloc(N * sizeof(Node*));
    if (!graph) {
        printf("Memory allocation failed\n");
        return 1;
    }

    // Initialize all adjacency lists to NULL
    for (int i = 0; i <N ; i++) {
        graph[i] = NULL;
    }
    // Read edges
    for (int i = 0; i < M; i++) {
        int src, dst, weight;

        // Read edge data
        if (fscanf(file, "%d %d %d", &src, &dst, &weight) != 3) {
            printf("Invalid input\n");
            return 1;
        }

        // Validate input
        if (weight < 0) {
            printf("Invalid input\n");
            return 1;
        }
        if (src < 0 || src >= N) {
            printf("Invalid input\n");
            return 1;
        }
        if (dst < 0 || dst >= N) {
            printf("Invalid input\n");
            return 1;
        }

        // Create new node
        Node* newNode = (Node*)malloc(sizeof(Node));
        if (!newNode) {
            printf("Memory allocation failed\n");
            return 1;
        }
        newNode->dest = dst;
        newNode->weight = weight;

        // Insert at beginning of adjacency list
        newNode->next = graph[src];
        graph[src] = newNode;
    }

    int num_travelers;
    [span_6](start_span)if (fscanf(file, "%d", &num_travelers) != 1) { // قراءة عدد المسافرين[span_6](end_span)
        printf("Invalid input format for travelers count\n");
        fclose(file);
        return 1;
    }

    Traveler* travelers = malloc(num_travelers * sizeof(Traveler));
    if (!travelers) {
        printf("Memory allocation failed for travelers\n");
        fclose(file);
        return 1;
    }

    [span_7](start_span)// مصفوفة الألوان المتاحة لإعطاء كل مسافر لوناً مختلفاً في الـ GUI[span_7](end_span)
    Color available_colors[] = { RED, BLUE, GREEN, ORANGE, PURPLE, LIME, GOLD, PINK };
    int num_colors = sizeof(available_colors) / sizeof(Color);

    [span_8](start_span)// حساب المسار مسبقاً لكل مسافر بواسطة الأب[span_8](end_span)
    for (int i = 0; i < num_travelers; i++) {
        [span_9](start_span)if (fscanf(file, "%d %d", &travelers[i].src, &travelers[i].dest) != 2) {[span_9](end_span)
            printf("Invalid traveler data\n");
            fclose(file);
            return 1;
        }

        int dummy_dist = 0;
        [span_10](start_span)// استدعاء الدالة المفرزة لحساب مسار دايَكسترا لكل مسافر[span_10](end_span)
        travelers[i].path = calculate_dijkstra(travelers[i].src, travelers[i].dest, N, &travelers[i].path_size, &dummy_dist);
        travelers[i].current_step = 0;
        travelers[i].color = available_colors[i % num_colors]; [span_11](start_span)// تخصيص لون فريد ومختلف[span_11](end_span)
        travelers[i].pid = -1;
    }

    int start, end;

    // Read start and end vertices
    if (fscanf(file, "%d %d", &start, &end) != 2) {
        printf("Invalid input\n");
        return 1;
    }
    fclose(file);

    for (int i = 0; i < num_travelers; i++) {
        pid_t pid = fork(); [span_12](start_span)// تفريع العملية[span_12](end_span)

        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }
        else if (pid == 0) {
            // ---- تהליך הבן (الابن) ----
            printf("[%d] started\n", getpid()); [span_13](start_span)// طباعة المعرف فوراً للترمينال[span_13](end_span)
            fflush(stdout); // إجبار التخزين المؤقت على الطباعة المباشرة

            [span_14](start_span)// الأبناء ينامون ولا يفعلون شيئاً، الأب هو من يحركهم على الـ GUI وينهيهم بالسيجنال[span_14](end_span)
            while (1) {
                sleep(1);
            }
            exit(0); // حماية لضمان عدم خروج عمليات الأبناء مطلقاً خارج هذه الحلقة
        }
        else {
            // ---- תהליך האב (الأب) ----
            travelers[i].pid = pid; [span_15](start_span)// الأب يحتفظ بمعرف الابن لإرسال الإشارة له لاحقاً[span_15](end_span)
        }
    }

    const int windowWidth = 800;
    const int windowHeight = 600;
    InitWindow(windowWidth, windowHeight, "OS Project - Milestone 4 Simulation");
    SetTargetFPS(10); // تحريك الخطوات بمعدل هادئ وواضح على الشاشة

    int active_travelers = num_travelers;

    [span_16](start_span)// حلقة الـ GUI وإدارة تحركات المسافرين متزامنة على الشاشة[span_16](end_span)

    while (!WindowShouldClose() && active_travelers > 0) {

        // 1. تحديث منطق الحركة لكل مسافر خطوة بخطوة
        for (int i = 0; i < num_travelers; i++) {
            // التأكد من أن المسافر لم ينهِ مساره بعد (عملية الابن ما زالت حية)
            if (travelers[i].pid != -1) {
                // إذا شارف على الوصول إلى نهاية مصفوفة المسار المحسوبة له
                if (travelers[i].current_step >= travelers[i].path_size - 1) {
                    kill(travelers[i].pid, SIGKILL); [span_17](start_span)// الأب يرسل سيجنال لإنهاء عملية الابن[span_17](end_span)
                    travelers[i].pid = -1;           // وضع علامة بأن الابن تم إنهاؤه
                    active_travelers--;
                } else {
                    travelers[i].current_step++;     // الانتقال للعقدة التالية في المسار التخيلي للـ GUI
                }
            }
        }

        [span_18](start_span)// 2. رسم المكونات الرسومية على الشاشة[span_18](end_span)
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Simulating Travelers Moving Simultaneously...", 20, 20, 20, DARKGRAY);

        for (int i = 0; i < N; i++) {
            int nodeX = 100 + (i * 100) % 600;
            int nodeY = 150 + (i * 80) % 400;
            DrawCircle(nodeX, nodeY, 20, LIGHTGRAY);
            DrawText(TextFormat("%d", i), nodeX - 5, nodeY - 8, 15, BLACK);
        }

        [span_19](start_span)// رسم المسافرين بألوانهم المختلفة أثناء حركتهم[span_19](end_span)
        for (int i = 0; i < num_travelers; i++) {
            if (travelers[i].pid != -1 && travelers[i].path != NULL) {
                int current_node = travelers[i].path[travelers[i].current_step];
                int curX = 100 + (current_node * 100) % 600;
                int curY = 150 + (current_node * 80) % 400;

                [span_20](start_span)// رسم دائرة تمثل الـ Traveler بلونه الخاص الفريد[span_20](end_span)
                DrawCircle(curX, curY, 12, travelers[i].color);
                DrawText(TextFormat("P%d", i+1), curX - 8, curY - 25, 12, travelers[i].color);
            }
        }

        EndDrawing();
    }

    CloseWindow(); // إغلاق نافذة الواجهة الرسومية

    // --- الإضافة الجديدة: الأب ينتظر كافة الأبناء قبل الخروج والإنهاء تماماً (Wait) ---
    for (int i = 0; i < num_travelers; i++) {
        if (travelers[i].pid != -1) {
            kill(travelers[i].pid, SIGKILL); // التأكد التام من إغلاق أي عملية متبقية
        }
        waitpid(travelers[i].pid, NULL, 0); [span_21](start_span)// الأب ينتظر الأبناء لمنع الـ Zombie processes[span_21](end_span)
    }

    // --- تنظيف وتنظيف الذاكرة المحجوزة بالكامل (من الكود القديم والمستحدث) ---
    for (int i = 0; i < N; i++) {
        Node* temp = graph[i];
        while (temp) {
            Node* next = temp->next;
            free(temp);
            temp = next;
        }
    }
    free(graph);

    for (int i = 0; i < num_travelers; i++) {
        if (travelers[i].path) {
            free(travelers[i].path);
        }
    }
    free(travelers);

    printf("Simulation finished cleanly.\n");
    return 0;
}

    // Allocate arrays
    int *dist = malloc(N * sizeof(int));
    int *visited = calloc(N, sizeof(int));
    int *parent = malloc(N * sizeof(int));


    // Check allocations
    if (!dist || !visited || !parent) {
        printf("Memory allocation failed\n");
        return 1;
    }

    // initialize arrays
    for (int i = 0; i < N; i++) {
        dist[i] = INT_MAX;
        visited[i] = 0;   // infinite distance
        parent[i] = -1;    // no parent
    }

    // Validate start/end
    if (start < 0 || start >= N || end < 0 || end >= N) {
        printf("Invalid input\n");
        return 1;
    }

    dist[start] = 0;

    // Special case: start == end
    if (start == end) {
        printf("0\n0\n");
        return 0;
    }
    // Dijkstra algorithm (O(N^2))
    for (int i = 0; i < N - 1; i++) {
        int min = INT_MAX, u = -1;

        // Find unvisited node with smallest distance
        for (int j = 0; j < N; j++) {
            if (!visited[j] && dist[j] < min) {
                min = dist[j];
                u = j;
            }
        }

        if (u == -1) break;

        visited[u] = 1;
        // Traverse adjacency list of u
        Node* temp = graph[u];
        while (temp) {
            int v = temp->dest;
            int weight = temp->weight;

            // Relaxation step
            if (!visited[v] && dist[u] != INT_MAX && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
            }

            temp = temp->next;
        }
    }
    // If no path found
    if (dist[end] == INT_MAX) {
        printf("No path found\n");
        return 0;
    }
    // Reconstruct path
    int *path = malloc(N * sizeof(int));
    if (!path) {
        printf("Memory allocation failed\n");
        return 1;
    }
    int count = 0;
    // Build path from end to start
    for (int v = end; v != -1; v = parent[v]) {
        path[count++] = v;
    }

    // Print path in correct order

    for (int i = count - 1; i >= 0; i--) {
        printf("%d", path[i]);
        if (i > 0) printf(" -> ");
    }

    printf("\n%d\n", dist[end]);

    // free graph memory
    for (int i = 0; i < N; i++) {
        Node* temp = graph[i];
        while (temp) {
            Node* next = temp->next;
            free(temp);
            temp = next;
        }
    }

    // free arrays
    free(visited);
    free(parent);
    free(dist);
    free(path);
    free(graph);

    return 0;
}

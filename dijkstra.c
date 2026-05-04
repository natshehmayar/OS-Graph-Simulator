#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

// Node structure for adjacency list
typedef struct Node{
    int dest;   //  destination vertex
    int weight;  // edge wight
    struct Node* next;   // pointer to next in list
} Node;

Node** graph;  // array of adjacency lists

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

    int start, end;

    // Read start and end vertices
    if (fscanf(file, "%d %d", &start, &end) != 2) {
        printf("Invalid input\n");
        return 1;
    }
    fclose(file);

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

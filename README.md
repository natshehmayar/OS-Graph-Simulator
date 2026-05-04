# Milestone 1: Dijkstra Shortest Path

## Team Members

* Mayar Natsheh
* Toleen
* Layan
* Nawal

## Overview

This project implements graph processing using **Dijkstra's Algorithm**.
The program reads a directed weighted graph from a file, builds it in memory, and computes the shortest path between two vertices.

---

## Input Format

```
N M
src dst weight
src dst weight
...
start end
```

### Explanation

* **N** – Number of vertices
* **M** – Number of edges
* **src** – Source vertex
* **dst** – Destination vertex
* **weight** – Edge weight (must be non-negative)
* **start, end** – Vertices for Dijkstra's algorithm

---

## Output Format

### If a path exists:

1. The shortest path
   `(v1 -> v2 -> v3 -> ... -> vn)`
2. The total weight of the path

### If no path exists:

```
No path found
```

### If `start == end`:

```
0
0
```

---

## Error Handling

If the input is invalid, the program prints:

```
Invalid input
```

---

## Compilation and Run

### Compile

```
make milestone1
```

### Run

```
./dijkstra input.txt
```

# Milestone 1: Dijkstra Shortest Path

## Team Members

* Mayar Natsheh
* Toleen Maswadeh
* Layan Rabba
* Nawal Dwaik

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

---

# Milestone 2: Graph Visualization

## Overview

This milestone adds a visual representation of the graph using Raylib.

The program displays the graph vertices and edges on the screen.
Each vertex is shown as a circle with its number, and each edge is shown as a directed line between two vertices.

## Features

* Draws all graph vertices
* Draws all directed weighted edges
* Displays edge weights
* Displays the start and end vertices

## Code Explanation

The graph is stored using an array of edges.
Each edge contains a source vertex, destination vertex, and weight.

The function `calculatePositions()` places the vertices in a circular layout so the graph is readable.

The function `drawArrow()` draws each directed edge as an arrow and displays the edge direction clearly.

## Compilation and Run

### Compile

```bash
make milestone2
```

### Run

```bash
./sim input.txt
```

---

# Milestone 3: Shortest Path Animation

## Overview

This milestone adds animation to the graph visualization.

An object moves along the shortest path from the start vertex to the destination vertex.

The movement is based on the edge weights, so larger weights take more animation steps than smaller weights.

## Features

* Adds a Play / Stop button
* Animates an object along the shortest path
* Highlights the shortest path in green
* Movement follows the shortest path found by Dijkstra's Algorithm
* Edge weights affect the animation progress
* The animation stops when the destination vertex is reached

## Code Explanation

The animation uses the shortest path stored in `travelers[t].shortestPath`.

The shortest path is highlighted in green to show the route used by the moving object.

The moving object starts at the source vertex using `entityX` and `entityY`.

The Play / Stop button changes the value of `isPlaying`.

For each edge in the shortest path, the program finds the edge weight.
The weight controls the number of animation jumps.

This means an edge with weight `W` is divided into `W` equal jumps.

Each jump happens every 300 milliseconds.
When the object reaches an intermediate vertex, it waits for one second before continuing.

The graph remains visible during the animation.

When the destination is reached, the animation stops and the message `Arrived at destination!` is displayed.

## Compilation and Run

### Compile

```bash
make milestone3
```

### Run

```bash
./sim input.txt
```

---

## Technologies Used

* C Programming Language
* Raylib Graphics Library
* Dijkstra's Algorithm
* Makefile

## Notes

* The graph is directed and weighted.
* Negative weights are considered invalid input.
* The program supports up to 15 vertices.
* Milestone 2 displays the graph statically.
* Milestone 3 adds interactive animation.

# Milestone 4: Multiple Processes and Parent Process

## Overview

This milestone extends the project from a single traveler to multiple travelers moving simultaneously on the graph.

The program uses multiple processes created with `fork()`.

A parent process manages the graphical interface and computes the shortest paths, while each child process represents a traveler.

The parent process is responsible for displaying all travelers on the graph and controlling their movement according to the paths computed by Dijkstra's Algorithm.

---

## Features

* Supports multiple travelers at the same time
* Uses `fork()` to create a child process for each traveler
* Parent process computes the shortest path for every traveler
* Each traveler is displayed with a different color
* All travelers move simultaneously on the graph
* Child processes print their PID when created
* Parent process terminates child processes when their journey is complete
* Parent process waits for all children before exiting

---

## Input Format

### Graph Definition

```text
N M
src dst weight
src dst weight
...
```

### Travelers

```text
travelerCount
source destination
source destination
...
```

## Compilation and Run

### Compile

```bash
make milestone4
```

### Run

```bash
./sim input.txt
```

---

## Technologies Used

* C Programming Language
* Raylib Graphics Library
* Dijkstra's Algorithm
* Process Management (`fork`, `kill`, `waitpid`)
* Makefile

---

## Notes

* Each traveler has an independent shortest path.
* Travelers are displayed using different colors.
* All animations occur concurrently.
* Child processes do not compute paths in this milestone.
* The parent process performs all path calculations.
* Child processes are introduced in preparation for IPC in Milestone 5.

# Milestone 5: IPC Between Processes

## IPC Mechanism

The IPC tool used in this milestone is:

* Unnamed Pipes

### Why Pipes?

* Simple communication between parent and child processes
* Reliable message passing
* Suitable for sending traveler location updates

## Overview

In this milestone each child process computes its own shortest path using Dijkstra's Algorithm.

The child sends location updates through a Pipe.

The parent process receives the messages, prints log information, and updates the GUI.
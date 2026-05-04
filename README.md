Team's members: 
Mayar Natsheh, 

milestone 1 : Dijkstra Shortest Path

Overview:

This project implements the foundation for graph processing using Dijkstra's algorthm. The program reads a directed weighted graph from a file, builds it in memory, and computes the shortest path between two vertices.

===========================================
Input Format:

N M
src dst weight
src dst weight
...
start end

Explanation :
N - number of vertices
M - number of edge
src - source vertex
dist - destination vertex
weight - edge weight (must be non-negative)
start and end - vertices for Dijkstra

=============================================
Output Format:
The output should be if path exists :
1. The shortest path.   ( v1 -> v2 -> v3 -> ... -> vn)
2. The total weight of the path.

if no path exists:
"No path found"

if "start == end":
0
0

=================================================
Error Handling:
The program print:
"Invalid input"

===========================================
Compilation and Run:
Compile:
make milestone1

Run:
./dijkstra input.txt

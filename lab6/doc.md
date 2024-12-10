## Algorithm
Detecting a Hamiltonian cycle: Perform DFS until we have a path containing all the nodes only once, after which we check to see if the last node has a path to the first one.
For the multithreaded version, we add a search from a neighbor to the thread pool if it is not full, or process that neighbor sequentially, except for the first neighbor of a node, for which the search is always performed on the current thread.

## Benchmark 1
### Input: a graph with 100 000 nodes, having edges from node i to node i+1 (first neighbor is always the correct one)
#### Singlethreaded : 49400ms ~ 49s
#### Multithreaded : 49431ms ~ 49s

## Benchmark 2
### Input: a graph with 10 nodes, having edges from node i to node i+1; each node has 3 neighbors: the first 2 neighbors will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 0ms
#### Multithreaded : 128ms

## Benchmark 3
### Input: a graph with 60 nodes, having edges from node i to node i+1; each node has 2 neighbors: the first will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 2495ms
#### Multithreaded : 140ms

## Benchmark 4
### Input: a graph with 64 nodes, having edges from node i to node i+1; each node has 2 neighbors: the first will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 18969ms ~ 19s
#### Multithreaded : 138ms

## Benchmark 5
### Input: a graph with 10000 nodes, having edges from node i to node i+1; every 500th node has 3 neighbors: the first 2 neighbors will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 13577ms ~ 14s
#### Multithreaded : 104224ms ~ 1m42s

## Benchmark 6
### Input: a graph with 50 nodes, having edges from node i to node i+1; each node has 3 neighbors: the first 2 neighbors will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 544587ms ~ 9m45s
#### Multithreaded : 8403ms ~ 8s
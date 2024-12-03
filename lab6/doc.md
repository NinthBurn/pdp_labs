## Algorithm
Detecting a Hamiltonian cycle: Perform DFS until we have a path containing all the nodes only once, after which we check to see if the last node has a path to the first one.
For the multithreaded version, we add a new task to the thread pool for each recursive call, except for the first neighbor of a node, which is done sequentially.

## Benchmark 1
### Input: a graph with 100 000 nodes, having edges from node i to node i+1 (first neighbor is always the correct one)
#### Singlethreaded : 52770ms
#### Multithreaded : 52476ms

## Benchmark 2
### Input: a graph with 10 nodes, having edges from node i to node i+1; each node has 3 neighbors: the first 2 neighbors will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 8ms
#### Multithreaded : 0ms

## Benchmark 3
### Input: a graph with 60 nodes, having edges from node i to node i+1; each node has 2 neighbors: the first will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 4189ms
#### Multithreaded : 127ms

## Benchmark 4
### Input: a graph with 64 nodes, having edges from node i to node i+1; each node has 2 neighbors: the first will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 58283ms
#### Multithreaded : 12ms

## Benchmark 5
### Input: a graph with 64 nodes, having edges from node i to node i+1; each node has 2 neighbors: the first will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 256892ms
#### Multithreaded : 14ms

## Benchmark 6
### Input: a graph with 10000 nodes, having edges from node i to node i+1; every 500th node has 3 neighbors: the first 2 neighbors will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 49202ms ~ 50s
#### Multithreaded : 175339ms ~ 3m

## Benchmark 7
### Input: a graph with 50 nodes, having edges from node i to node i+1; each node has 3 neighbors: the first 2 neighbors will most likely not be the correct choice for a Hamiltonian cycle, while the last neighbor is always correct
#### Singlethreaded : 915400ms ~ 15m 
#### Multithreaded : 8403ms ~ 8s
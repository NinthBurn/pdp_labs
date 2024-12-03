#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <queue>
#include <condition_variable>
#include <functional>
#include <future>
#include <chrono>
using namespace std;

#define THREAD_POOL_SIZE 16
#define VERIFY_SOLUTION true
#define PRINT_SOLUTION false

class ThreadPool {
private:
    vector<thread> workers;
    queue<function<void()>> tasks;

    mutex queueMutex;
    condition_variable condition;
    bool stop = false;
    atomic<int> activeTasks{0};

public:
    ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;
                        task = move(this->tasks.front());
                        this->tasks.pop();
                    }
                    ++activeTasks;
                    task();
                    --activeTasks;
                }
            });
        }
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (thread& worker : workers) {
            worker.join();
        }
    }

    template<typename F>
    auto enqueue(F&& f) -> future<typename result_of<F()>::type> {
        using return_type = typename result_of<F()>::type;

        auto task = make_shared<packaged_task<return_type()>>(forward<F>(f));
        future<return_type> res = task->get_future();

        {
            unique_lock<mutex> lock(queueMutex);
            if (stop) throw runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task] { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    bool isBusy() {
        return activeTasks.load() >= workers.size();
    }

    bool isEmpty() {
        return activeTasks.load() == 0;
    }

    void waitForThreads() {
        while(!isEmpty())
            this_thread::sleep_for(chrono::milliseconds(100));
    }
    
}tpool(THREAD_POOL_SIZE);

class Timer{
private:
    chrono::_V2::system_clock::time_point start;
    chrono::_V2::system_clock::time_point end;

public:
    Timer() {}

    void startTimer() {
        start = chrono::system_clock::now();
    }

    void endTimer() {
        end = chrono::system_clock::now();
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        cout << "Timer stopped after " << elapsed_seconds << " milliseconds.\n";
    }
}timeit;

class Graph {
public:
    int verticesCount;
    vector<vector<bool>> adjacencyMatrix;
    vector<vector<int>> adjacencyList;

    Graph(int v) : verticesCount(v), adjacencyMatrix(v, vector<bool>(v, false)), adjacencyList(v, vector<int>()) {}

    void addEdge(int u, int v) {
        adjacencyMatrix[u][v] = true;
        adjacencyList[u].push_back(v);
    }

    const vector<int>& getNeighbors(int v) const {
        return adjacencyList[v];
    }

    bool hasEdge(int u, int v) const {
        return adjacencyMatrix[u][v];
    }

    bool isSafe(int vertex, const vector<int>& path, int pos) {
        for(int i = 0; i < verticesCount; ++i) {
            if(path[i] == vertex)
                return false;

            // reached the end of current sequence
            if(path[i] == -1)
                break;
        }
            

        return (pos < verticesCount);
    }

    void printSolution(vector<int>& path, int startVertex) {
        if(!PRINT_SOLUTION) return;

        cout << "Hamiltonian Cycle: ";
        for (int i = 0; i < verticesCount; i++) {
            cout << path[i] << " ";
        }
        cout << startVertex << "\n";
    }

    bool verifySolution(vector<int>& path) {
        if(!VERIFY_SOLUTION)
            return true;

        for(int currentVertex = 0; currentVertex < verticesCount; ++currentVertex) {
            int count = 0;

            for(auto& vertex : path) {
                if(vertex == currentVertex) {
                    ++count;
                    if(count > 1) {
                        cout << "Vertex appears multiple times in the path\n";
                        return false;
                    }
                        
                }
            }
        }

        int len = path.size();
        for(int i = 0; i < len - 1; ++i)
            if(!adjacencyMatrix[path[i]][path[i + 1]]) {
                cout << "Vertices in path do not have an edge connecting them\n";
                return false;
            }

        return true;
    }

    void hamiltonianUtilST(int startVertex, vector<int>& path, int pos, atomic<bool>& found) {
        if (found) return;

        if (pos == verticesCount) {
            int lastVertex = path[pos - 1];
            if (adjacencyMatrix[lastVertex][startVertex] && !found) {
                if(!verifySolution(path))
                    return;

                found = true;

                printSolution(path, startVertex);
            }

            return;
        }

        int lastVertex = path[pos - 1];

        // for(int neighbor = 0; neighbor < verticesCount; ++neighbor) {
        //     if (isSafe(neighbor, path, pos) && adjacencyMatrix[path[pos - 1]][neighbor]) {
        for (auto neighbor : adjacencyList[lastVertex]) {
            if (isSafe(neighbor, path, pos)) {
                path[pos] = neighbor;

                hamiltonianUtilST(startVertex, path, pos + 1, found);
                if (found) return;
                path[pos] = -1;
            }
        }
    }

    void hamiltonianUtil(int startVertex, vector<int> path, int pos, atomic<bool>& found, bool isStarterThread) {
        if (found) return;
        int lastVertex = path[pos - 1];

        if (pos == verticesCount) {
            if (adjacencyMatrix[lastVertex][startVertex] && !found) {
                if(!verifySolution(path))
                    return;
                found = true;

                printSolution(ref(path), startVertex);
            }

            return;
        }

        int skipFirst = 0;

        // search for all neighbors except the first one using the thread pool
        for (auto neighbor : adjacencyList[lastVertex]) {
            if (skipFirst++ > 0 && isSafe(neighbor, ref(path), pos)) {
                if (found) return;

                path[pos] = neighbor;

                // if(!tpool.isBusy()){
                    tpool.enqueue([this, &found, path, startVertex, pos] {
                        hamiltonianUtil(startVertex, path, pos + 1, found, false);
                        }
                    );
                // } else {
                //     hamiltonianUtilST(startVertex, path, pos + 1, found);
                //     if (found) return;
                // }
                
                path[pos] = -1;
            }
        }

        // this thread will check if there exists a cycle continuing from the first neighbor
        if(!adjacencyList[lastVertex].empty()) {
            path[pos] = adjacencyList[lastVertex][0];

            hamiltonianUtilST(startVertex, path, pos + 1, found);
            
            if(isStarterThread)
                tpool.waitForThreads();

            if (found) return;

            // Backtrack
            path[pos] = -1;
        }

    }

    void findHamiltonianCycle(int startVertex) {
        atomic<bool> found(false);
        atomic<bool> isStarterThread(true);
        vector<int> path(verticesCount, -1);
        path[0] = startVertex;

        hamiltonianUtil(startVertex, path, 1, found, true);

        if (!found) {
            cout << "No Hamiltonian Cycle found." << endl;
        }
    }

    void findHamiltonianCycleST(int startVertex) {
        atomic<bool> found(false);
        vector<int> path(verticesCount, -1);
        path[0] = startVertex;

        hamiltonianUtilST(startVertex, path, 1, found);

        if (!found) {
            cout << "No Hamiltonian Cycle found." << endl;
        }
    }

    static Graph generateHamiltonianCycle(int vertexCount, int unproductiveNeighbors, int neighborInterval) {
        Graph graph(vertexCount);

        for (int i = 0; i < vertexCount; ++i) {
            if(unproductiveNeighbors + 2 < vertexCount && i % neighborInterval == 0) {
                for(int j = 0; j < unproductiveNeighbors; ++j) {
                    int randVert =  rand() % vertexCount;
                    if(i + 1 != randVert && !graph.hasEdge(i, randVert))
                        graph.addEdge(i, randVert % vertexCount);
                    else --j;
                }
            }

            graph.addEdge(i, (i + 1) % vertexCount);
        }

        {
        // add random edges to the graph
        // srand(time(0));
        // for (int i = 0; i < vertexCount * 2; ++i) { 
        //     int u = rand() % vertexCount;
        //     int v = rand() % vertexCount;
        //     if (u != v) {
        //         graph.addEdge(u, v);
        //     }
        // }
        }

        return graph;
    }
};

int main() {
    Graph g = Graph::generateHamiltonianCycle(100000, 0, 1);

    cout << "generated\n";
    int startVertex = 0;

    timeit.startTimer();
    g.findHamiltonianCycle(startVertex);
    timeit.endTimer();

    cout << "st\n";
    timeit.startTimer();
    g.findHamiltonianCycleST(startVertex);
    timeit.endTimer();

    return 0;
}

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <exception>

using namespace std;

#define MATRIX1_ROWS 1024
#define MATRIX1_COLS 512
#define MATRIX2_ROWS 512
#define MATRIX2_COLS 1024

#define TASK_COUNT 8
#define DEBUG_PRINT false

#define THREAD_POOL_SIZE 8

// Asynchronous output https://stackoverflow.com/a/45046349
struct Acout
{
    mutex object_lock;

    template <typename T>
    Acout &operator<<(const T &_t)
    {
        if(DEBUG_PRINT){
            object_lock.lock();
            cout << _t << flush;
            object_lock.unlock();
        }
        return *this;
    }

    Acout &operator<<(ostream &(*fp)(ostream &))
    {
        if(DEBUG_PRINT){
            object_lock.lock();
            cout << fp << flush;
            object_lock.unlock();
        }
        return *this;
    }
} acout;

class ThreadPool {
private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::list<std::function<void()> > m_queue;
    bool m_end;
    std::vector<std::thread> m_threads;

public:
    explicit ThreadPool(size_t nrThreads)
        :m_end(false)
    {
        m_threads.reserve(nrThreads);
        for (size_t i = 0; i < nrThreads; ++i) {
            m_threads.emplace_back([this]() {this->run(); });
        }
    }

    ~ThreadPool() {
        close();
        for (std::thread& t : m_threads) {
            t.join();
        }
    }

    void close() {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_end = true;
        m_cond.notify_all();
    }

    void enqueue(std::function<void()> func) {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_queue.push_back(std::move(func));
        m_cond.notify_one();
    }

private:
    void run() {
        while (true) {
            std::function<void()> toExec;
            {
                std::unique_lock<std::mutex> lck(m_mutex);
                while (m_queue.empty() && !m_end) {
                    m_cond.wait(lck);
                }
                if (m_queue.empty()) {
                    return;
                }
                toExec = std::move(m_queue.front());
                m_queue.pop_front();
            }
            toExec();
        }
    }
};

int computeElement(int row, int column, vector<vector<int>> &matrix1, vector<vector<int>> &matrix2) {
    int result = 0;
    int row_size = matrix1[0].size();

    for(int i = 0; i < row_size; ++i)
        result += matrix1[row][i] * matrix2[i][column];

    return result;
}

void generateMatrices(vector<vector<int>> &matrix1, vector<vector<int>> &matrix2) {
    for(int i = 0; i < MATRIX1_ROWS; ++i) {
        matrix1.push_back(vector<int>());
        for(int j = 0; j < MATRIX1_COLS; ++j)
            matrix1[i].push_back(i * j + 1);
    }

    for(int i = 0; i < MATRIX2_ROWS; ++i) {
        matrix2.push_back(vector<int>());
        for(int j = 0; j < MATRIX2_COLS; ++j)
            matrix2[i].push_back(i * j + 1);
    }
}

void printMatrix(vector<vector<int>> &matrix) {
    int rows = matrix.size();
    int columns = matrix[0].size();

    for(int i = 0; i < rows; ++i) {
        for(int j = 0; j < columns; ++j)
            acout << matrix[i][j] << " ";

        acout << "\n";
    }

    acout << "\n";
}

void threadWorkRowByRow(vector<vector<int>> &matrix1, vector<vector<int>> &matrix2, vector<vector<int>> &result, int start_row, int start_column, int element_num) {
    auto tid = this_thread::get_id();

    for(int i = start_row; i < MATRIX1_ROWS && element_num > 0; ++i) {
        for(int j = start_column; j < MATRIX2_COLS && element_num > 0; ++j, --element_num) {
            result[i][j] = computeElement(i, j, ref(matrix1), ref(matrix2));
            start_column = 0;

            acout << "[T" << tid << "] Computed element on row " << i << " column " << j << ", value " << result[i][j] << "\n";
        }
    }
}

void threadWorkColumnByColumn(vector<vector<int>> &matrix1, vector<vector<int>> &matrix2, vector<vector<int>> &result, int start_row, int start_column, int element_num) {
    auto tid = this_thread::get_id();

    for(int j = start_column; j < MATRIX2_COLS && element_num > 0; ++j) {
        for(int i = start_row; i < MATRIX1_ROWS && element_num > 0; ++i, --element_num) {
            result[i][j] = computeElement(i, j, ref(matrix1), ref(matrix2));
            start_row = 0;

            acout << "[T" << tid << "] Computed element on row " << i << " column " << j << ", value " << result[i][j] << "\n";
        }
    }
}

void threadWorkKth(vector<vector<int>> &matrix1, vector<vector<int>> &matrix2, vector<vector<int>> &result, int order) {
    auto tid = this_thread::get_id();

    for(int i = 0; i < MATRIX1_ROWS; ++i) {
        for(int j = 0; j < MATRIX2_COLS; ++j) {
            if((i + j) % TASK_COUNT == order) {
                result[i][j] = computeElement(i, j, ref(matrix1), ref(matrix2));

                acout << "[T" << tid << "] Computed element on row " << i << " column " << j << ", value " << result[i][j] << "\n";
            }
        }
    }
}

void verifyResult(vector<vector<int>> &correct, vector<vector<int>> &result) {
    int rows = correct.size(), columns = correct[0].size();

    for(int i = 0; i < rows; ++i)
        for(int j = 0; j < columns; ++j)
            if(correct[i][j] != result[i][j]) {
                cout << "Result is incorrect\n";
                throw -1;
            }
}

// run all matrix multiplication functions using the low-level thread mechanism
void threadTest(vector<vector<int>> &matrix1, vector<vector<int>> &matrix2, vector<vector<int>> &correct_result) {
    int index;

    // Threads - row by row method
    {
        vector<vector<int>> thread_result;
        for (index = 0; index < MATRIX1_ROWS; ++index) {
            thread_result.push_back(vector<int>(MATRIX2_COLS));
        }

        vector<thread> children;
        int additional_elements = MATRIX1_ROWS * MATRIX2_COLS % TASK_COUNT;
        int elements_computed = 0, element_count;
        int elements_per_thread = MATRIX1_ROWS * MATRIX2_COLS / TASK_COUNT;
        int row_start, column_start;

        auto start = std::chrono::system_clock::now();
        auto start_time = std::chrono::system_clock::to_time_t(start);

        for (index = 0; index < TASK_COUNT; ++index) {
            element_count = elements_per_thread;

            if(additional_elements > 0) {
                ++element_count;
                --additional_elements;
            }

            row_start = elements_computed / MATRIX2_COLS;
            column_start = elements_computed % MATRIX2_COLS;

            children.push_back(thread(threadWorkRowByRow, ref(matrix1), ref(matrix2), ref(thread_result), 
                row_start, column_start, element_count));

            elements_computed += element_count;
        }

        for (thread &child : children) {
            child.join();
        }

        auto end = std::chrono::system_clock::now();
        auto end_time = std::chrono::system_clock::to_time_t(end);
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        verifyResult(ref(correct_result), ref(thread_result));
        cout << "Threads row-by-row multiplication finished\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n\n";
    }

    // Threads - column by column method
    {
        vector<vector<int>> thread_result;
        for (index = 0; index < MATRIX1_ROWS; ++index) {
            thread_result.push_back(vector<int>(MATRIX2_COLS));
        }

        vector<thread> children;
        int additional_elements = MATRIX1_ROWS * MATRIX2_COLS % TASK_COUNT;
        int elements_computed = 0, element_count;
        int elements_per_thread = MATRIX1_ROWS * MATRIX2_COLS / TASK_COUNT;
        int row_start, column_start;

        auto start = std::chrono::system_clock::now();
        auto start_time = std::chrono::system_clock::to_time_t(start);

        for (index = 0; index < TASK_COUNT; ++index) {
            element_count = elements_per_thread;

            if(additional_elements > 0) {
                ++element_count;
                --additional_elements;
            }

            row_start = elements_computed / MATRIX1_ROWS;
            column_start = elements_computed % MATRIX1_ROWS;

            children.push_back(thread(threadWorkColumnByColumn, ref(matrix1), ref(matrix2), ref(thread_result), 
                column_start, row_start, element_count));

            elements_computed += element_count;
        }

        for (thread &child : children) {
            child.join();
        }

        auto end = std::chrono::system_clock::now();
        auto end_time = std::chrono::system_clock::to_time_t(end);
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        verifyResult(ref(correct_result), ref(thread_result));
        cout << "Threads column-by-column multiplication finished\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n\n";
    }

    // Threads - kth element method
    {
        vector<vector<int>> thread_result;
        for (index = 0; index < MATRIX1_ROWS; ++index) {
            thread_result.push_back(vector<int>(MATRIX2_COLS));
        }

        vector<thread> children;

        auto start = std::chrono::system_clock::now();
        auto start_time = std::chrono::system_clock::to_time_t(start);

        for (index = 0; index < TASK_COUNT; ++index) {
            children.push_back(thread(threadWorkKth, ref(matrix1), ref(matrix2), ref(thread_result), index));
        }

        for (thread &child : children) {
            child.join();
        }

        auto end = std::chrono::system_clock::now();
        auto end_time = std::chrono::system_clock::to_time_t(end);
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        verifyResult(ref(correct_result), ref(thread_result));
        cout << "Threads k-th element multiplication finished\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n\n";
    }
}

// run all matrix multiplication functions using a thread pool
void threadPoolTest(vector<vector<int>> &matrix1, vector<vector<int>> &matrix2, vector<vector<int>> &correct_result) {
    int index;

    // Thread pool - row by row method
    {
        vector<vector<int>> thread_result;
        for (index = 0; index < MATRIX1_ROWS; ++index) {
            thread_result.push_back(vector<int>(MATRIX2_COLS));
        }

        int additional_elements = MATRIX1_ROWS * MATRIX2_COLS % TASK_COUNT;
        int elements_computed = 0, element_count;
        int elements_per_thread = MATRIX1_ROWS * MATRIX2_COLS / TASK_COUNT;
        int row_start, column_start;

        auto start = std::chrono::system_clock::now();
        auto start_time = std::chrono::system_clock::to_time_t(start);

        {
            ThreadPool pool(THREAD_POOL_SIZE);
        
            for (index = 0; index < TASK_COUNT; ++index) {
                element_count = elements_per_thread;

                if(additional_elements > 0) {
                    ++element_count;
                    --additional_elements;
                }

                row_start = elements_computed / MATRIX2_COLS;
                column_start = elements_computed % MATRIX2_COLS;

                pool.enqueue([=, &matrix1, &matrix2, &thread_result]()
                {
                    threadWorkRowByRow(ref(matrix1), ref(matrix2), ref(thread_result), row_start, column_start, element_count);
                });

                elements_computed += element_count;
            }
        }

        auto end = std::chrono::system_clock::now();
        auto end_time = std::chrono::system_clock::to_time_t(end);
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        verifyResult(ref(correct_result), ref(thread_result));
        cout << "Thread pool row-by-row multiplication finished\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n\n";
    }

    // Thread pool - column by column method
    {
        vector<vector<int>> thread_result;
        for (index = 0; index < MATRIX1_ROWS; ++index) {
            thread_result.push_back(vector<int>(MATRIX2_COLS));
        }

        int additional_elements = MATRIX1_ROWS * MATRIX2_COLS % TASK_COUNT;
        int elements_computed = 0, element_count;
        int elements_per_thread = MATRIX1_ROWS * MATRIX2_COLS / TASK_COUNT;
        int row_start, column_start;

        auto start = std::chrono::system_clock::now();
        auto start_time = std::chrono::system_clock::to_time_t(start);
        {
            ThreadPool pool(THREAD_POOL_SIZE);

            for (index = 0; index < TASK_COUNT; ++index) {
                element_count = elements_per_thread;

                if(additional_elements > 0) {
                    ++element_count;
                    --additional_elements;
                }

                row_start = elements_computed / MATRIX1_ROWS;
                column_start = elements_computed % MATRIX1_ROWS;

                pool.enqueue([=, &matrix1, &matrix2, &thread_result]()
                {
                    threadWorkColumnByColumn(ref(matrix1), ref(matrix2), ref(thread_result), column_start, row_start, element_count);
                });

                elements_computed += element_count;
            }
        }

        auto end = std::chrono::system_clock::now();
        auto end_time = std::chrono::system_clock::to_time_t(end);
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        verifyResult(ref(correct_result), ref(thread_result));
        cout << "Thread pool column-by-column multiplication finished\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n\n";
    }

    // Thread pool - kth element method
    {
        vector<vector<int>> thread_result;
        for (index = 0; index < MATRIX1_ROWS; ++index) {
            thread_result.push_back(vector<int>(MATRIX2_COLS));
        }

        auto start = std::chrono::system_clock::now();
        auto start_time = std::chrono::system_clock::to_time_t(start);

        {
            ThreadPool pool(THREAD_POOL_SIZE);
        
            for (index = 0; index < TASK_COUNT; ++index) {
                pool.enqueue([=, &matrix1, &matrix2, &thread_result]()
                {
                    threadWorkKth(ref(matrix1), ref(matrix2), ref(thread_result), index);
                });
            }
        }
        
        auto end = std::chrono::system_clock::now();
        auto end_time = std::chrono::system_clock::to_time_t(end);
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        verifyResult(ref(correct_result), ref(thread_result));
        cout << "Thread pool k-th element multiplication finished\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";
        cout << "====================================================\n";
    }
}

int main() {
    vector<vector<int>> matrix1;
    vector<vector<int>> matrix2;
    vector<vector<int>> correct_result;
    int index;

    if(MATRIX1_COLS != MATRIX2_ROWS) {
        cout << "Invalid matrix sizes\n";
        return -1;
    }

    generateMatrices(ref(matrix1), ref(matrix2));

    acout << "Matrix 1\n";
    printMatrix(matrix1);
    acout << "Matrix 2\n";
    printMatrix(matrix2);

    for (index = 0; index < MATRIX1_ROWS; ++index) {
        correct_result.push_back(vector<int>(MATRIX2_COLS));
    }

    auto start = std::chrono::system_clock::now();
    auto start_time = std::chrono::system_clock::to_time_t(start);

    for(int i = 0; i < MATRIX1_ROWS; ++i) {
        for(int j = 0; j < MATRIX2_COLS; ++j)
            correct_result[i][j] = computeElement(i, j, ref(matrix1), ref(matrix2));
    }

    auto end = std::chrono::system_clock::now();
    auto end_time = std::chrono::system_clock::to_time_t(end);
    auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "====================================================\n";
    cout << "Matrix 1: " << MATRIX1_ROWS << "x" << MATRIX1_COLS << "\n"; 
    cout << "Matrix 2: " << MATRIX2_ROWS << "x" << MATRIX2_COLS << "\n";
    cout << "Task count: " << TASK_COUNT << "\n";
    cout << "Thread pool size: " << THREAD_POOL_SIZE << "\n\n";

    cout << "Single thread multiplication finished\n";
    cout << "Elapsed time: " << elapsed_seconds << "ms\n\n";

    threadTest(ref(matrix1), ref(matrix2), ref(correct_result));
    threadPoolTest(ref(matrix1), ref(matrix2), ref(correct_result));

    acout << "Result matrix\n";
    printMatrix(correct_result);
    
    return 0;
}
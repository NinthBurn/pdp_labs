#include <iostream>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>

using namespace std;

#define ELEMENT_COUNT 600000

mutex m;
condition_variable cv;
atomic_bool productReady = false;
atomic_bool processed = true;
atomic_int productValue = 0;

class ValueQueue{
private:
    queue<int> values;
    mutex object_lock;

public:
    int pop() {
        unique_lock lk(object_lock);
        int value = this->values.front();
        this->values.pop();
        return value;
    }

    void push(int value) {
        unique_lock lk(object_lock);
        this->values.push(value);
    }

    bool empty() {
        unique_lock lk(object_lock);
        return this->values.empty();
    }
};

void producerThreadQueue(vector<int> &v1, vector<int> &v2, int &size, ValueQueue &value_queue) {
    for (int i = 0; i < size; ++i) {
        unique_lock lk(m);
        value_queue.push(v1[i] * v2[i]);
        cv.notify_all();
    }
}

void consumerThreadQueue(int &size, int &result, ValueQueue &value_queue) {
    result = 0;

    for (int i = 0; i < size; ++i) {
        unique_lock<mutex> lk(m);

        cv.wait(lk, [&]
                { return value_queue.empty() == false; });

        result += value_queue.pop();
    }
}

void producerThread(vector<int> &v1, vector<int> &v2, int &size) {
    for (int i = 0; i < size; ++i) {
        unique_lock lk(m);

        cv.wait(lk, []
            { return processed == true; });

        productValue = v1[i] * v2[i];
        processed = false;
        productReady = true;

        cv.notify_all();
    }
}

void consumerThread(int &size, int &result)
{
    result = 0;

    for (int i = 0; i < size; ++i) {
        unique_lock<mutex> lk(m);

        cv.wait(lk, []
            { return productReady == true; });

        result += productValue;
        productReady = false;
        processed = true;

        cv.notify_all();
    }
}

int main() {
    int i, size = ELEMENT_COUNT, result, result_queue;
    int expectedResult = 0;
    
    vector<int> v1;
    vector<int> v2;

    for (i = 0; i < size; ++i){
        v1.push_back(i + 1);
        v2.push_back(size - i);
    }

    auto start = std::chrono::system_clock::now();
    auto start_time = std::chrono::system_clock::to_time_t(start);

    for (i = 0; i < size; ++i){
        expectedResult += v1[i] * v2[i];
    }

    auto end = std::chrono::system_clock::now();
    auto end_time = std::chrono::system_clock::to_time_t(end);
    auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "=========================================\n";
    cout << "Single thread result result: " << expectedResult << "\n";
    cout << "\tTotal elapsed time: " << elapsed_seconds << "ms\n\n";

    start = std::chrono::system_clock::now();
    start_time = std::chrono::system_clock::to_time_t(start);

    thread producer(producerThread, ref(v1), ref(v2), ref(size));
    thread consumer(consumerThread, ref(size), ref(result));

    producer.join();
    consumer.join();

    end = std::chrono::system_clock::now();
    end_time = std::chrono::system_clock::to_time_t(end);
    elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Threads result: " << result << "\n";
    cout << "\tTotal elapsed time: " << elapsed_seconds << "ms\n\n";
    
    ValueQueue queue;

    start = std::chrono::system_clock::now();
    start_time = std::chrono::system_clock::to_time_t(start);
    
    thread producerQueue(producerThreadQueue, ref(v1), ref(v2), ref(size), ref(queue));
    thread consumerQueue(consumerThreadQueue, ref(size), ref(result_queue), ref(queue));

    producerQueue.join();
    consumerQueue.join();

    end = std::chrono::system_clock::now();
    end_time = std::chrono::system_clock::to_time_t(end);
    elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    
    cout << "Threads + queue result: " << result_queue << "\n";
    cout << "\tTotal elapsed time: " << elapsed_seconds << "ms\n";
    cout << "=========================================\n";

    return 0;
}
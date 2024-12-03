#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <condition_variable>
#include <list>
#include <functional>
#include <future>
#include <random>
#include <queue>
#include <span>

using namespace std;

#define POLY1_SIZE 10000
#define POLY2_SIZE 10000
#define THREAD_POOL_SIZE 16

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
}tpool(THREAD_POOL_SIZE);

class Polynomial
{
private:
    int degree;

public:
    vector<int> coefficients;

    Polynomial() {}
    Polynomial(int degree) : coefficients(vector<int>(degree + 1, 0)) {}
    Polynomial(vector<int> coefficients) : degree(coefficients.size() - 1), coefficients(coefficients) {};

    int getDegree() const { return this->degree; }
    vector<int> getCoefficients() const { return this->coefficients; }

    void setDegree(int degree) {
        this->degree = degree;
    }

    Polynomial(Polynomial const &other)
    {
        this->degree = other.getDegree();
        this->coefficients = other.getCoefficients();
    }

    Polynomial &operator=(Polynomial const &other)
    {
        this->degree = other.getDegree();
        this->coefficients = other.getCoefficients();
        return *this;
    }

    Polynomial operator+(Polynomial const &other) const
    {
        int max_degree = max(this->degree, other.getDegree()) + 1;
        int pol2_size = other.getDegree();
        vector<int> result_coefficients(max_degree);

        for (int d1 = 0; d1 <= this->degree; ++d1)
        {
            result_coefficients[d1] += this->coefficients[d1];
        }

        for (int d2 = 0; d2 <= pol2_size; ++d2)
        {
            result_coefficients[d2] += other.coefficients[d2];
        }

        return Polynomial(result_coefficients);
    }

    Polynomial operator-(Polynomial const &other) const
    {
        int max_degree = max(this->degree, other.getDegree()) + 1;
        int pol2_size = other.getDegree();
        vector<int> result_coefficients(max_degree);

        for (int d1 = 0; d1 <= this->degree; ++d1)
        {
            result_coefficients[d1] += this->coefficients[d1];
        }

        for (int d2 = 0; d2 <= pol2_size; ++d2)
        {
            result_coefficients[d2] -= other.coefficients[d2];
        }

        return Polynomial(result_coefficients);
    }

    Polynomial operator*(Polynomial const &other) const
    {
        int max_degree = this->degree + other.getDegree() + 1;
        int pol2_size = other.getDegree();
        vector<int> result_coefficients(max_degree);

        for (int d1 = 0; d1 <= this->degree; ++d1)
        {
            for (int d2 = 0; d2 <= pol2_size; ++d2)
            {
                result_coefficients[d1 + d2] += this->coefficients[d1] * other.coefficients[d2];
            }
        }

        return Polynomial(result_coefficients);
    }

    bool operator==(Polynomial const &other) const
    {
        int size_other = other.getDegree();

        if (this->degree != size_other)
            return false;

        for (int i = 0; i <= this->degree; ++i)
            if (this->coefficients[i] != other.coefficients[i])
                return false;

        return true;
    }

    bool operator!=(Polynomial const &other) const
    {
        return !(*this == other);
    }

    void padToSize(int size)
    {
        size -= this->coefficients.size();
        for(int i = 0; i < size; ++i)
            this->coefficients.push_back(0);
    }

    static Polynomial generatePolynomial(int max_degree)
    {
        vector<int> coefficients;

        for (int i = 0; i <= max_degree; ++i)
            coefficients.push_back(rand() % 1000);

        return Polynomial(coefficients);
    }

    string toString() const
    {
        string result = to_string(coefficients[degree]) + "x^" + to_string(degree);

        for (int i = degree - 1; i > 0; --i)
        {
            if (coefficients[i] == 0)
                continue;
            if (coefficients[i] < 0)
                result += " - ";
            else
                result += " + ";
            result += to_string(abs(coefficients[i])) + "x^" + to_string(i);
        }

        if (coefficients[0] > 0)
            result += " + " + to_string(abs(coefficients[0]));
        else if (coefficients[0] < 0)
            result += " - " + to_string(abs(coefficients[0]));

        return result;
    }

    ~Polynomial() {}
};

vector<int> karatsuba(span<int> A, span<int> B)
{
    int n = A.size();

    if (n < 65) {
        vector<int> result(n * 2 - 1, 0);
        for (int i = 0; i < A.size(); ++i) {
            for (int j = 0; j < B.size(); ++j) {
                result[i + j] += A[i] * B[j];
            }
        }
        return result;
    }

    int mid = n / 2;

    span<int> A0(A.begin(), A.begin() + mid);
    span<int> A1(A.begin() + mid, A.end());
    span<int> B0(B.begin(), B.begin() + mid);
    span<int> B1(B.begin() + mid, B.end());

    vector<int> A0A1(max(A0.size(), A1.size()), 0);
    vector<int> B0B1(max(B0.size(), B1.size()), 0);

    for (int i = 0; i < A0.size(); ++i) {
        A0A1[i] += A0[i];
    }
    for (int i = 0; i < A1.size(); ++i) {
        A0A1[i] += A1[i];
    }

    for (int i = 0; i < B0.size(); ++i) {
        B0B1[i] += B0[i];
    }
    for (int i = 0; i < B1.size(); ++i) {
        B0B1[i] += B1[i];
    }

    vector<int> z0 = karatsuba(A0, B0);
    vector<int> z1 = karatsuba(A1, B1);
    vector<int> z2 = karatsuba(A0A1, B0B1);

    // Combine results (z2 - z1 - z0)
    vector<int> z4 = z2;
    for (int i = 0; i < z1.size(); ++i) {
        z4[i] -= z1[i];
    }
    for (int i = 0; i < z0.size(); ++i) {
        z4[i] -= z0[i];
    }

    vector<int> result(2 * n - 1, 0);

    for (int i = 0; i < z0.size(); ++i) {
        result[i] += z0[i];
    }

    for (int i = 0; i < z4.size(); ++i) {
        result[i + mid] += z4[i]; 
    }

    for (int i = 0; i < z1.size(); ++i) {
        result[i + 2 * mid] += z1[i];  
    }

    return result;
}

vector<int> karatsuba_mt(span<int> A, span<int> B)
{
    int n = A.size();

    if (n < 65) {
        vector<int> result(n * 2 - 1, 0);
        for (int i = 0; i < A.size(); ++i) {
            for (int j = 0; j < B.size(); ++j) {
                result[i + j] += A[i] * B[j];
            }
        }
        return result;
    }

    int mid = n / 2;

    span<int> A0(A.begin(), A.begin() + mid);
    span<int> A1(A.begin() + mid, A.end());
    span<int> B0(B.begin(), B.begin() + mid);
    span<int> B1(B.begin() + mid, B.end());

    vector<int> A0A1(max(A0.size(), A1.size()), 0);
    vector<int> B0B1(max(B0.size(), B1.size()), 0);

    for (int i = 0; i < A0.size(); ++i) {
        A0A1[i] += A0[i];
    }
    for (int i = 0; i < A1.size(); ++i) {
        A0A1[i] += A1[i];
    }

    for (int i = 0; i < B0.size(); ++i) {
        B0B1[i] += B0[i];
    }
    for (int i = 0; i < B1.size(); ++i) {
        B0B1[i] += B1[i];
    }

    vector<int> z0, z1;

    future<vector<int>> z0f, z1f;
    bool uses_threads = false;
    
    if (tpool.isBusy()) {
        z0 = karatsuba(A0, B0);
        z1 = karatsuba(A1, B1);
    } else {
        uses_threads = true;
        z0f = tpool.enqueue([&] { return karatsuba_mt(A0, B0); });
        z1f = tpool.enqueue([&] { return karatsuba_mt(A1, B1); });
    }

    vector<int> z2 = karatsuba(A0A1, B0B1);
    
    if (uses_threads) {
        z0 = z0f.get();
        z1 = z1f.get();
    }

    // Combine results (z2 - z1 - z0)
    vector<int> z4 = z2;
    for (int i = 0; i < z1.size(); ++i) {
        z4[i] -= z1[i];
    }
    for (int i = 0; i < z0.size(); ++i) {
        z4[i] -= z0[i];
    }

    vector<int> result(2 * n - 1, 0);

    for (int i = 0; i < z0.size(); ++i) {
        result[i] += z0[i];
    }

    for (int i = 0; i < z4.size(); ++i) {
        result[i + mid] += z4[i]; 
    }

    for (int i = 0; i < z1.size(); ++i) {
        result[i + 2 * mid] += z1[i];  
    }

    return result;
}

Polynomial karatsuba_mult(Polynomial &p1, Polynomial &p2) {
    int p1_deg = p1.getDegree() + 1, p2_deg = p2.getDegree() + 1;
    int x = max(p1_deg, p2_deg);
    // int size = pow(2, ceil(log2(max(p1_deg, p2_deg))));

    p1.padToSize(x + 1);
    p2.padToSize(x + 1);
    Polynomial res = Polynomial(karatsuba(span<int>(p1.coefficients), span<int>(p2.coefficients)));
    res.setDegree(res.getDegree() - 2);
    return res;
}

Polynomial karatsuba_mult_mt(Polynomial &p1, Polynomial &p2) {
    int p1_deg = p1.getDegree() + 1, p2_deg = p2.getDegree() + 1;
    int x = max(p1_deg, p2_deg);

    p1.padToSize(x + 1);
    p2.padToSize(x + 1);
    return Polynomial(karatsuba_mt(span<int>(p1.coefficients), span<int>(p2.coefficients)));
}

bool segmentMultiplication(Polynomial &p1, Polynomial &p2, vector<int> &result_coefficients, int min_pos, int max_pos) { 
    int pol1_size = p1.getDegree(), pol2_size = p2.getDegree();

    for (int d1 = 0; d1 <= pol1_size && d1 < max_pos; ++d1)
    {
        for (int d2 = 0; d2 <= pol2_size && d1 + d2 < max_pos; ++d2)
        {
            if(d1 + d2 >= min_pos)
                result_coefficients[d1 + d2] = (result_coefficients[d1 + d2] + p1.coefficients[d1] * p2.coefficients[d2]) ;
            else d2 = min_pos - d1 - 1;
        }
    }

    return true;
}

Polynomial polynomialMultiplicationMT(Polynomial &p1, Polynomial &p2) { 
    int max_degree = p1.getDegree() + p2.getDegree() + 1;
    int pol1_size = p1.getDegree(), pol2_size = p2.getDegree();
    vector<int> result_coefficients(max_degree);
    vector<future<bool>> futures;

    int increment = max_degree * 3 / 2 / THREAD_POOL_SIZE;
    increment = increment > 0 ? increment : 4;

    for (int start = 0; start <= max_degree; start += increment) 
        futures.push_back(tpool.enqueue([=, &p1, &p2, &result_coefficients]()
            { 
                return segmentMultiplication(ref(p1), ref(p2), ref(result_coefficients), start, start + increment); 
            }));

    int len = futures.size();
    for(int i = 0; i < len; ++i)
        futures[i].get();

    return Polynomial(result_coefficients);
}

int main()
{
    try{
        Polynomial test1 = Polynomial::generatePolynomial(POLY1_SIZE);
        Polynomial test2 = Polynomial::generatePolynomial(POLY2_SIZE);

        auto start = chrono::system_clock::now();
        auto start_time = chrono::system_clock::to_time_t(start);

        Polynomial correct_result = (test1 * test2);
        Polynomial result;

        auto end = chrono::system_clock::now();
        auto end_time = chrono::system_clock::to_time_t(end);
        auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Single-threaded polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";



        start = chrono::system_clock::now();
        start_time = chrono::system_clock::to_time_t(start);

        result = polynomialMultiplicationMT(test1, test2);
        
        end = chrono::system_clock::now();
        end_time = chrono::system_clock::to_time_t(end);
        elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Multi-threaded O(n^2) polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";

        if(result != correct_result)
            throw "Incorrect multiplication - polynomial multiplication multi";



        start = chrono::system_clock::now();
        start_time = chrono::system_clock::to_time_t(start);

        result = karatsuba_mult(test1, test2);
        
        end = chrono::system_clock::now();
        end_time = chrono::system_clock::to_time_t(end);
        elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Single-threaded Karatsuba polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";
        
        // cout << correct_result.toString() << "\n" << result.toString() << "\n";
        if(result != correct_result)
            throw "Incorrect multiplication - Karatsuba single";

        start = chrono::system_clock::now();
        start_time = chrono::system_clock::to_time_t(start);

        result = karatsuba_mult_mt(test1, test2);
        
        end = chrono::system_clock::now();
        end_time = chrono::system_clock::to_time_t(end);
        elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Multi-threaded Karatsuba polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";

        if(result != correct_result)
            throw "Incorrect multiplication - Karatsuba multi";

    }catch(const char* message) {
        cout << message << "\n";
    }

    return 0;
}
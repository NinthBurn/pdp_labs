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
#include <chrono>

using namespace std;

#define POLY1_SIZE 20000
#define POLY2_SIZE 20000
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
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
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
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

    template<typename F>
    auto enqueue(F&& f) -> std::future<typename std::result_of<F()>::type> {
        using return_type = typename std::result_of<F()>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task] { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    bool isBusy() {
        return activeTasks.load() >= workers.size();
    }
}tpool(THREAD_POOL_SIZE);

class Timer{
private:
    chrono::system_clock::time_point start;
    chrono::system_clock::time_point end;

public:
    Timer() {}

    void startTimer() {
        start = chrono::system_clock::now();
    }

    int64_t endTimer() {
        end = chrono::system_clock::now();
        return chrono::duration_cast<chrono::milliseconds>(end - start).count();
    }
};

class Polynomial
{
private:
    int degree;

public:
    vector<int> coefficients;

    Polynomial() {}
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

Polynomial karatsuba(Polynomial &p1, Polynomial &p2)
{
    int n = p1.coefficients.size();

    if (n < 65)
        return p1 * p2;

    int mid = n / 2;

    vector<int> A0(p1.coefficients.begin(), p1.coefficients.begin() + mid);
    vector<int> A1(p1.coefficients.begin() + mid, p1.coefficients.end());
    vector<int> B0(p2.coefficients.begin(), p2.coefficients.begin() + mid);
    vector<int> B1(p2.coefficients.begin() + mid, p2.coefficients.end());

    Polynomial high1(A1), high2(B1),
        low1(A0), low2(B0);

    Polynomial l1h1 = low1 + high1;
    Polynomial l2h2 = low2 + high2;
    
    Polynomial z0 = karatsuba(low1, low2);
    Polynomial z1 = karatsuba(high1, high2);
    Polynomial z2 = karatsuba(l1h1, l2h2);

    Polynomial z4 = (z2 - z1 - z0);

    vector<int> result(2 * n - 1, 0);

    // Z0 contributes to the lower half
    for (int i = 0; i < z0.coefficients.size(); ++i) {
        result[i] += z0.coefficients[i];
    }

    // Middle part contributes to the middle of the result
    for (int i = 0; i < z4.coefficients.size(); ++i) {
        result[i + mid] += z4.coefficients[i];
    }

    // Z1 contributes to the upper half
    for (int i = 0; i < z1.coefficients.size(); ++i) {
        result[i + 2 * mid] += z1.coefficients[i];
    }

    Polynomial result_polynomial = Polynomial(result);
    result_polynomial.setDegree(p1.getDegree() + p2.getDegree());
    return result_polynomial;
}

Polynomial karatsuba_mt(Polynomial &p1, Polynomial &p2)
{
    int n = p1.coefficients.size();

    if (n < 65)
        return p1 * p2;

    int mid = n / 2;

    vector<int> A0(p1.coefficients.begin(), p1.coefficients.begin() + mid);
    vector<int> A1(p1.coefficients.begin() + mid, p1.coefficients.end());
    vector<int> B0(p2.coefficients.begin(), p2.coefficients.begin() + mid);
    vector<int> B1(p2.coefficients.begin() + mid, p2.coefficients.end());

    Polynomial high1(A1), high2(B1),
        low1(A0), low2(B0);

    Polynomial z0, z1, z2;
    future<Polynomial> z0f, z1f;
    bool z0_uses_threads = false, z1_uses_threads = false;
    
    // auto z0Future = pool.enqueue([&] { return karatsuba(A_low, B_low, pool); });
    if (tpool.isBusy()) {
        z0 = karatsuba(low1, low2);
    } else {
        z0_uses_threads = true;
        z0f = tpool.enqueue([&] { return karatsuba_mt(low1, low2); });
    }

    if (tpool.isBusy()) {
        z1 = karatsuba(high1, high2); 
    } else {
        z1_uses_threads = true;
        z1f = tpool.enqueue([&] { return karatsuba_mt(high1, high2); });
    }

    Polynomial l1h1 = low1 + high1;
    Polynomial l2h2 = low2 + high2;
    
    z2 = karatsuba_mt(l1h1, l2h2);
    
    if (z0_uses_threads) {
        z0 = z0f.get();
    }

    if (z1_uses_threads) {
        z1 = z1f.get();
    }

    Polynomial z4 = (z2 - z1 - z0);

    vector<int> result(2 * n - 1, 0);

    for (int i = 0; i < z0.coefficients.size(); ++i) {
        result[i] += z0.coefficients[i];
    }

    for (int i = 0; i < z4.coefficients.size(); ++i) {
        result[i + mid] += z4.coefficients[i];
    }

    for (int i = 0; i < z1.coefficients.size(); ++i) {
        result[i + 2 * mid] += z1.coefficients[i];
    }

    Polynomial result_polynomial = Polynomial(result);
    result_polynomial.setDegree(p1.getDegree() + p2.getDegree());

    return result_polynomial;
}

Polynomial karatsuba_mult(Polynomial &p1, Polynomial &p2) {
    int p1_deg = p1.getDegree() + 1, p2_deg = p2.getDegree() + 1;
    int x = max(p1_deg, p2_deg);
    // int size = pow(2, ceil(log2(max(p1_deg, p2_deg))));

    p1.padToSize(x + 1);
    p2.padToSize(x + 1);
    return karatsuba(p1, p2);
}

Polynomial karatsuba_mult_mt(Polynomial &p1, Polynomial &p2) {
    int p1_deg = p1.getDegree() + 1, p2_deg = p2.getDegree() + 1;
    int x = max(p1_deg, p2_deg);

    p1.padToSize(x + 1);
    p2.padToSize(x + 1);
    return karatsuba_mt(p1, p2);
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
        Polynomial result;

        Timer timer;
        timer.startTimer();
        Polynomial correct_result = (test1 * test2);
        auto elapsed_seconds = timer.endTimer();

        cout << "Single-threaded polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";

        
        timer.startTimer();
        result = polynomialMultiplicationMT(test1, test2);
        elapsed_seconds = timer.endTimer();

        cout << "Multi-threaded O(n^2) polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";

        if(result != correct_result)
            throw "Incorrect multiplication - polynomial multiplication multi";


        timer.startTimer();
        result = karatsuba_mult(test1, test2);
        elapsed_seconds = timer.endTimer();

        cout << "Single-threaded Karatsuba polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";
        
        if(result != correct_result)
            throw "Incorrect multiplication - Karatsuba single";


        timer.startTimer();
        result = karatsuba_mult_mt(test1, test2);
        elapsed_seconds = timer.endTimer();

        cout << "Multi-threaded Karatsuba polynomial multiplication\n";
        cout << "Elapsed time: " << elapsed_seconds << "ms\n";

        if(result != correct_result)
            throw "Incorrect multiplication - Karatsuba multi";

    }catch(const char* message) {
        cout << message << "\n";
    }

    return 0;
}
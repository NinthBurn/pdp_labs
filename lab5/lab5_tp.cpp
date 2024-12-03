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

using namespace std;

#define POLY1_SIZE 500
#define POLY2_SIZE 1500
#define THREAD_POOL_SIZE 8

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
}tpool(5);

class Polynomial
{
private:
    vector<int> coefficients;
    int degree;

public:
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
        vector<int> pol2_coefficients = other.getCoefficients();
        vector<int> result_coefficients;

        for (int i = 0; i < max_degree; ++i)
            result_coefficients.push_back(0);

        for (int d1 = 0; d1 <= this->degree; ++d1)
        {
            result_coefficients[d1] += this->coefficients[d1];
        }

        for (int d2 = 0; d2 <= pol2_size; ++d2)
        {
            result_coefficients[d2] += pol2_coefficients[d2];
        }

        return Polynomial(result_coefficients);
    }

    Polynomial operator-(Polynomial const &other) const
    {
        int max_degree = max(this->degree, other.getDegree()) + 1;
        int pol2_size = other.getDegree();
        vector<int> pol2_coefficients = other.getCoefficients();
        vector<int> result_coefficients;

        for (int i = 0; i < max_degree; ++i)
            result_coefficients.push_back(0);

        for (int d1 = 0; d1 <= this->degree; ++d1)
        {
            result_coefficients[d1] += this->coefficients[d1];
        }

        for (int d2 = 0; d2 <= pol2_size; ++d2)
        {
            result_coefficients[d2] -= pol2_coefficients[d2];
        }

        return Polynomial(result_coefficients);
    }

    Polynomial operator*(Polynomial const &other) const
    {
        int max_degree = this->degree + other.getDegree() + 1;
        int pol2_size = other.getDegree();
        vector<int> pol2_coefficients = other.getCoefficients();
        vector<int> result_coefficients;

        for (int i = 0; i < max_degree; ++i)
            result_coefficients.push_back(0);

        for (int d1 = 0; d1 <= this->degree; ++d1)
        {
            for (int d2 = 0; d2 <= pol2_size; ++d2)
            {
                result_coefficients[d1 + d2] += this->coefficients[d1] * pol2_coefficients[d2];
            }
        }

        return Polynomial(result_coefficients);
    }

    bool operator==(Polynomial const &other) const
    {
        int size_other = other.getDegree();
        vector<int> coefficients_other = other.getCoefficients();

        if (this->degree != size_other)
            return false;

        for (int i = 0; i <= this->degree; ++i)
            if (this->coefficients[i] != coefficients_other[i])
                return false;

        return true;
    }

    void padToSize(int size)
    {
        size -= this->coefficients.size();
        for(int i = 0; i < size; ++i)
            this->coefficients.push_back(0);
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

Polynomial karatsuba(Polynomial const &p1, Polynomial const &p2)
{
    vector<int> p1_coef = p1.getCoefficients(), p2_coef = p2.getCoefficients();
    int n = p1_coef.size();

    if (n < 64)
        return p1 * p2;

    int mid = n / 2;

    vector<int> A0(p1_coef.begin(), p1_coef.begin() + mid);
    vector<int> A1(p1_coef.begin() + mid, p1_coef.end());
    vector<int> B0(p2_coef.begin(), p2_coef.begin() + mid);
    vector<int> B1(p2_coef.begin() + mid, p2_coef.end());

    Polynomial high1(A1), high2(B1),
        low1(A0), low2(B0);

    Polynomial z0 = karatsuba(low1, low2);
    Polynomial z1 = karatsuba(high1, high2);
    Polynomial z2 = karatsuba(low1 + high1, low2 + high2);

    Polynomial z4 = (z2 - z1 - z0);

    vector<int> result(2 * n - 1, 0);
    vector<int> P0 = z0.getCoefficients();                
    vector<int> P1 = z1.getCoefficients();              
    vector<int> P2 = z2.getCoefficients();
    vector<int> middle = z4.getCoefficients();

    // P0 contributes to the lower half
    for (int i = 0; i < P0.size(); ++i) {
        result[i] += P0[i];
    }

    // Middle part contributes to the middle of the result
    for (int i = 0; i < middle.size(); ++i) {
        result[i + mid] += middle[i];
    }

    // P1 contributes to the upper half
    for (int i = 0; i < P1.size(); ++i) {
        result[i + 2 * mid] += P1[i];
    }

    Polynomial result_polynomial = Polynomial(result);
    result_polynomial.setDegree(p1.getDegree() + p2.getDegree());
    return result_polynomial;
}

Polynomial karatsuba_mt(Polynomial const &p1, Polynomial const &p2)
{
    vector<int> p1_coef = p1.getCoefficients(), p2_coef = p2.getCoefficients();
    int n = p1_coef.size();

    if (n < 64)
        return p1 * p2;

    int mid = n / 2;

    vector<int> A0(p1_coef.begin(), p1_coef.begin() + mid);
    vector<int> A1(p1_coef.begin() + mid, p1_coef.end());
    vector<int> B0(p2_coef.begin(), p2_coef.begin() + mid);
    vector<int> B1(p2_coef.begin() + mid, p2_coef.end());

    Polynomial high1(A1), high2(B1),
        low1(A0), low2(B0);

    Polynomial z0, z1, z2;
    future<Polynomial> z0f, z1f;
    bool uses_threads = false;
    
    // auto z0Future = pool.enqueue([&] { return karatsuba(A_low, B_low, pool); });
    if (tpool.isBusy()) {
        z0 = karatsuba(low1, low2);
        z1 = karatsuba(high1, high2); 
    } else {
        uses_threads = true;
        z0f = tpool.enqueue([&] { return karatsuba_mt(low1, low2); });
        z1f = tpool.enqueue([&] { return karatsuba_mt(high1, high2); });
    }

    z2 = karatsuba(low1 + high1, low2 + high2);
    if (uses_threads) {
        z0 = z0f.get();
        z1 = z1f.get();
    }

    Polynomial z4 = (z2 - z1 - z0);

    vector<int> result(2 * n - 1, 0);
    vector<int> P0 = z0.getCoefficients();                
    vector<int> P1 = z1.getCoefficients();              
    vector<int> P2 = z2.getCoefficients();
    vector<int> middle = z4.getCoefficients();

    // P0 contributes to the lower half
    for (int i = 0; i < P0.size(); ++i) {
        result[i] += P0[i];
    }

    // Middle part contributes to the middle of the result
    for (int i = 0; i < middle.size(); ++i) {
        result[i + mid] += middle[i];
    }

    // P1 contributes to the upper half
    for (int i = 0; i < P1.size(); ++i) {
        result[i + 2 * mid] += P1[i];
    }

    Polynomial result_polynomial = Polynomial(result);
    result_polynomial.setDegree(p1.getDegree() + p2.getDegree());

    return result_polynomial;
}

Polynomial generatePolynomial(int max_degree)
{
    vector<int> coefficients;

    for (int i = 0; i <= max_degree; ++i)
        coefficients.push_back(rand() % 1000);

    return Polynomial(coefficients);
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
    // int size = pow(2, ceil(log2(max(p1_deg, p2_deg))));

    p1.padToSize(x + 1);
    p2.padToSize(x + 1);
    return karatsuba_mt(p1, p2);
}

int main()
{
    // Polynomial test1(vector<int>({2, 0, 3, 2, 2, 2, 2, 2,}));
    // Polynomial test2(vector<int>({1, 4}));
    Polynomial test1 = generatePolynomial(10000);
    Polynomial test2 = generatePolynomial(10000);
    // cout << test2.toString() << "\n";
    // cout << test1.toString() << "\n";
    
    auto start = std::chrono::system_clock::now();
    auto start_time = std::chrono::system_clock::to_time_t(start);

    (test1 * test2);

    auto end = std::chrono::system_clock::now();
    auto end_time = std::chrono::system_clock::to_time_t(end);
    auto elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Elapsed time: " << elapsed_seconds << "ms\n";

    start = std::chrono::system_clock::now();
    start_time = std::chrono::system_clock::to_time_t(start);

     karatsuba_mult(test1, test2);
    
    end = std::chrono::system_clock::now();
    end_time = std::chrono::system_clock::to_time_t(end);
    elapsed_seconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Elapsed time: " << elapsed_seconds << "ms\n";

    cout << (karatsuba_mult_mt(test2, test1) == (test1 * test2)) << "\n";
    return 0;
}
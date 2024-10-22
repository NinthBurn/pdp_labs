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

using namespace std;

#define POLY1_SIZE 500
#define POLY2_SIZE 1500

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

class Polynomial {
private:
    vector<int> coefficients;
    int degree;

public:
    Polynomial() {}
    Polynomial(vector<int> coefficients) : degree(coefficients.size() - 1), coefficients(coefficients){};

    int getDegree() const { return this->degree; }
    vector<int> getCoefficients() const { return this->coefficients; }

    Polynomial(Polynomial const &other) {
        this->degree = other.getDegree();
        this->coefficients = other.getCoefficients();
    }

    Polynomial& operator=(Polynomial const &other) {
        this->degree = other.getDegree();
        this->coefficients = other.getCoefficients();
        return *this;
    }

    Polynomial operator+(Polynomial const &other) const {
        int max_degree = max(this->degree, other.getDegree()) + 1;
        int pol2_size = other.getDegree();
        vector<int> pol2_coefficients = other.getCoefficients();
        vector<int> result_coefficients;

        for(int i = 0; i < max_degree; ++i)
            result_coefficients.push_back(0);

        for(int d1 = 0; d1 <= this->degree; ++d1) {
            result_coefficients[d1] += this->coefficients[d1];
        }

        for(int d2 = 0; d2 <= pol2_size; ++d2) {
            result_coefficients[d2] += pol2_coefficients[d2];
        }

        return Polynomial(result_coefficients);
    }

    Polynomial operator-(Polynomial const &other) const {
        int max_degree = max(this->degree, other.getDegree()) + 1;
        int pol2_size = other.getDegree();
        vector<int> pol2_coefficients = other.getCoefficients();
        vector<int> result_coefficients;

        for(int i = 0; i < max_degree; ++i)
            result_coefficients.push_back(0);

        for(int d1 = 0; d1 <= this->degree; ++d1) {
            result_coefficients[d1] += this->coefficients[d1];
        }

        for(int d2 = 0; d2 <= pol2_size; ++d2) {
            result_coefficients[d2] -= pol2_coefficients[d2];
        }

        return Polynomial(result_coefficients);
    }

    Polynomial operator*(Polynomial const &other) const {
        int max_degree = this->degree + other.getDegree() + 1;
        int pol2_size = other.getDegree();
        vector<int> pol2_coefficients = other.getCoefficients();
        vector<int> result_coefficients;

        for(int i = 0; i < max_degree; ++i)
            result_coefficients.push_back(0);

        for(int d1 = 0; d1 <= this->degree; ++d1) {
            for(int d2 = 0; d2 <= pol2_size; ++d2) {
                result_coefficients[d1 + d2] += this->coefficients[d1] * pol2_coefficients[d2];
            }
        }

        return Polynomial(result_coefficients);
    }

    bool operator==(Polynomial const &other) const {
        int size_other = other.getDegree();
        vector<int> coefficients_other = other.getCoefficients();

        if(this->degree != size_other)
            return false;

        for(int i = 0; i <= this->degree; ++i)
            if(this->coefficients[i] != coefficients_other[i])
                return false;

        return true;
    }

    void shiftOrder(int amount) {
        this->degree += amount;
        for(int i = 0; i < amount; ++i)
            this->coefficients.insert(this->coefficients.begin(), 0);
    }

    string toString() const {
        string result = to_string(coefficients[degree]) + "x^" + to_string(degree);

        for(int i = degree - 1; i > 0; --i) {
            if(coefficients[i] == 0) continue;
            if(coefficients[i] < 0) result += " - ";
            else result += " + ";
            result += to_string(abs(coefficients[i])) + "x^" + to_string(i);
        }

        if(coefficients[0] > 0)
            result += " + " + to_string(abs(coefficients[0]));
        else if(coefficients[0] < 0)
            result += " - " + to_string(abs(coefficients[0]));

        return result;
    }

    ~Polynomial() {}
};

Polynomial karatsuba(Polynomial const &p1, Polynomial const &p2) {
    int i, p1_deg = p1.getDegree(), p2_deg = p2.getDegree();
    
    vector<int> p1_coef = p1.getCoefficients(), p2_coef = p2.getCoefficients();

    if(p1_deg < 2 || p2_deg < 2)
        return p1 * p2;

    int n = max(p1_deg, p2_deg);
    int middle = n / 2;

    vector<int> v1, v2, v3, v4;
    for(int i = 0; i < middle && i < p1_deg; ++i) {
        v1.push_back(p1_coef[i]);
    }

    for(int i = 0; i < middle && i < p2_deg; ++i) {
        v3.push_back(p2_coef[i]);
    }

    for(int i = min(middle, p1_deg); i <= p1_deg; ++i) {
        v2.push_back(p1_coef[i]);
    }

    for(int i = min(middle, p2_deg); i <= p2_deg; ++i) {
        v4.push_back(p2_coef[i]);
    }

    Polynomial high1(v1), high2(v3), 
        low1(v2), low2(v4);

    Polynomial z2 = karatsuba(low1, low2);
    Polynomial z1 = karatsuba(low1 + high1, low2 + high2);
    Polynomial z0 = karatsuba(high1, high2);

    Polynomial z4 = (z1 - z2 - z0);

    z2.shiftOrder(middle * 2);
    z4.shiftOrder(middle);
    
    return z2 + z4 + z0;
}

future<Polynomial> karatsuba_mt(Polynomial const &p1, Polynomial const &p2) {
    int i, p1_deg = p1.getDegree(), p2_deg = p2.getDegree();
    vector<int> p1_coef = p1.getCoefficients(), p2_coef = p2.getCoefficients();

    if(p1_deg < 2 || p2_deg < 2)
        return async(launch::async, [=]() {return p1 * p2;});

    int n = max(p1_deg, p2_deg);
    int middle = n / 2;
    
    vector<int> v1{p1_coef.begin(), p1_coef.begin() + middle}, v2{p1_coef.begin() + middle, p1_coef.end()}, v3{p2_coef.begin(), p2_coef.begin() + middle}, v4{p2_coef.begin() + middle, p2_coef.end()};
    Polynomial high1(v1), high2(v3), 
        low1(v2), low2(v4);

    Polynomial z2 = karatsuba_mt(low1, low2).get();
    Polynomial z1 = karatsuba_mt(low1 + high1, low2 + high2).get();
    Polynomial z0 = karatsuba_mt(high1, high2).get();

    Polynomial z4 = (z1 - z2 - z0);

    z2.shiftOrder(middle * 2);
    z4.shiftOrder(middle);
    
    return async(launch::async, [=]() {return z2 + z4 + z0;});
}

Polynomial generatePolynomial(int max_degree) {
    vector<int> coefficients;

    for(int i = 0; i <= max_degree; ++i)
        coefficients.push_back(rand() % 1000);

    return Polynomial(coefficients);
}

int main() {
    // Polynomial test1(vector<int>({1, 1, 2, -3, 4, 5}));
    // Polynomial test2(vector<int>({1, 1, 2, -3, 15}));
    Polynomial test1 = generatePolynomial(900);
    Polynomial test2 = generatePolynomial(600);
    // cout << test1.toString() << "\n";
    // cout << test2.toString() << "\n";
    cout << (karatsuba(test1, test2) == (test1 * test2)) << "\n";
    return 0;
}
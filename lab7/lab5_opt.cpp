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
#include <mpi.h>
using namespace std;

#define POLY1_SIZE 10000
#define POLY2_SIZE 10000

class Timer{
private:
    chrono::_V2::system_clock::time_point start;
    chrono::_V2::system_clock::time_point end;

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
        this->degree = other.degree;
        this->coefficients = other.coefficients;
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

bool segmentMultiplication(Polynomial &p1, Polynomial &p2, vector<int> &result_coefficients, int min_pos, int max_pos) { 
    int pol1_size = p1.getDegree(), pol2_size = p2.getDegree();
    
    for (int d1 = 0; d1 <= pol1_size && d1 <= max_pos; ++d1)
    {
        for (int d2 = 0; d2 <= pol2_size && d1 + d2 <= max_pos; ++d2)
        {
            if(d1 + d2 >= min_pos)
                result_coefficients[d1 + d2] = (result_coefficients[d1 + d2] + p1.coefficients[d1] * p2.coefficients[d2]) ;
            else d2 = min_pos - d1 - 1;
        }
    }

    return true;
}

Polynomial polynomialMultiplicationMT(MPI_Comm comm, int rank, int size, Polynomial &p1, Polynomial &p2) { 
    int max_degree = p1.getDegree() + p2.getDegree() + 1;
    int pol1_size = p1.getDegree(), pol2_size = p2.getDegree();
    vector<int> result_coefficients(max_degree, 0);
    vector<int> buffer(max_degree);

    int increment = max_degree / size;
    increment = increment > 0 ? increment : 1;

    if(rank == 0) {
        for (int currentRank = 1, start = increment; start <= max_degree && currentRank < size; start += increment, ++currentRank) {
            int start_point = start;
            MPI_Ssend(&start_point, 1, MPI_INT, currentRank, currentRank * 100, comm);
        }

        segmentMultiplication(p1, p2, result_coefficients, 0, increment);

        for (int currentRank = 1, start = increment; start <= max_degree && currentRank < size; start += increment, ++currentRank) {
            MPI_Recv(buffer.data(), buffer.size(), MPI_INT, currentRank, currentRank * 10000, comm, MPI_STATUS_IGNORE);
            
            for (int d1 = 0; d1 <= pol1_size && d1 <= start + increment; ++d1)
            {
                for (int d2 = 0; d2 <= pol2_size && d1 + d2 <= start + increment; ++d2)
                {
                    if(d1 + d2 >= start)
                        result_coefficients[d1 + d2] = buffer[d1 + d2];
                    else d2 = start - d1 - 1;
                }
            }
        }

    } else {
        int start;
        MPI_Recv(&start, 1, MPI_INT, 0, rank * 100, comm, MPI_STATUS_IGNORE);
        segmentMultiplication(p1, p2, result_coefficients, start, start + increment);
        MPI_Ssend(result_coefficients.data(), result_coefficients.size(), MPI_INT, 0, 10000 * rank, comm);
    }
    
    return Polynomial(result_coefficients);
}

Polynomial karatsuba(MPI_Comm comm, int rank, int size, Polynomial &p1, Polynomial &p2) {
    int n = max(p1.getDegree(), p2.getDegree());
    if (n < 2) {
        return p1 * p2;
    }

    p1.padToSize(n + 1);
    p2.padToSize(n + 1);
    int mid = n / 2;

    vector<int> A0(p1.coefficients.begin(), p1.coefficients.begin() + mid);
    vector<int> A1(p1.coefficients.begin() + mid, p1.coefficients.end());
    vector<int> B0(p2.coefficients.begin(), p2.coefficients.begin() + mid);
    vector<int> B1(p2.coefficients.begin() + mid, p2.coefficients.end());

    Polynomial low1(A0), high1(A1), low2(B0), high2(B1);

    Polynomial l1h1 = low1 + high1;
    Polynomial l2h2 = low2 + high2;

    Polynomial z0, z1, z2;
    if (rank == 0) {
        int low1_size = low1.coefficients.size();
        int low2_size = low2.coefficients.size();
        int high1_size = high1.coefficients.size();
        int high2_size = high2.coefficients.size();
        int l1h1_size = l1h1.coefficients.size();
        int l2h2_size = l2h2.coefficients.size();

        MPI_Send(&low1_size, 1, MPI_INT, 1, 0, comm);
        MPI_Send(&low2_size, 1, MPI_INT, 1, 1, comm);
        MPI_Send(&high1_size, 1, MPI_INT, 2, 2, comm);
        MPI_Send(&high2_size, 1, MPI_INT, 2, 3, comm);
        MPI_Send(&l1h1_size, 1, MPI_INT, 3, 4, comm);
        MPI_Send(&l2h2_size, 1, MPI_INT, 3, 5, comm);

        MPI_Send(low1.coefficients.data(), low1_size, MPI_INT, 1, 6, comm);
        MPI_Send(low2.coefficients.data(), low2_size, MPI_INT, 1, 7, comm);
        MPI_Send(high1.coefficients.data(), high1_size, MPI_INT, 2, 8, comm);
        MPI_Send(high2.coefficients.data(), high2_size, MPI_INT, 2, 9, comm);
        MPI_Send(l1h1.coefficients.data(), l1h1_size, MPI_INT, 3, 10, comm);
        MPI_Send(l2h2.coefficients.data(), l2h2_size, MPI_INT, 3, 11, comm);

        z0.coefficients.resize(low1_size + low2_size - 1);
        z0.setDegree(low1_size + low2_size - 2);
        MPI_Recv(z0.coefficients.data(), z0.coefficients.size(), MPI_INT, 1, 12, comm, MPI_STATUS_IGNORE);

        z1.coefficients.resize(high1_size + high2_size - 1);
        z1.setDegree(high1_size + high2_size - 2);
        MPI_Recv(z1.coefficients.data(), z1.coefficients.size(), MPI_INT, 2, 13, comm, MPI_STATUS_IGNORE);

        z2.coefficients.resize(l1h1_size + l2h2_size - 1);
        z2.setDegree(l1h1_size + l2h2_size - 2);
        MPI_Recv(z2.coefficients.data(), z2.coefficients.size(), MPI_INT, 3, 14, comm, MPI_STATUS_IGNORE);

    } else if (rank == 1) {
        int low1_size, low2_size;
        MPI_Recv(&low1_size, 1, MPI_INT, 0, 0, comm, MPI_STATUS_IGNORE);
        MPI_Recv(&low2_size, 1, MPI_INT, 0, 1, comm, MPI_STATUS_IGNORE);

        Polynomial low1_recv, low2_recv;
        low1_recv.coefficients.resize(low1_size);
        low1_recv.setDegree(low1_size - 1);
        low2_recv.coefficients.resize(low2_size);
        low2_recv.setDegree(low2_size - 1);
        
        MPI_Recv(low1_recv.coefficients.data(), low1_size, MPI_INT, 0, 6, comm, MPI_STATUS_IGNORE);
        MPI_Recv(low2_recv.coefficients.data(), low2_size, MPI_INT, 0, 7, comm, MPI_STATUS_IGNORE);

        Polynomial z0 = low1_recv * low2_recv;
        MPI_Send(z0.coefficients.data(), z0.coefficients.size(), MPI_INT, 0, 12, comm);

    } else if (rank == 2) {
        int high1_size, high2_size;
        MPI_Recv(&high1_size, 1, MPI_INT, 0, 2, comm, MPI_STATUS_IGNORE);
        MPI_Recv(&high2_size, 1, MPI_INT, 0, 3, comm, MPI_STATUS_IGNORE);

        Polynomial high1_recv, high2_recv;
        high1_recv.coefficients.resize(high1_size);
        high1_recv.setDegree(high1_size - 1);
        high2_recv.coefficients.resize(high2_size);
        high2_recv.setDegree(high2_size - 1);
        
        MPI_Recv(high1_recv.coefficients.data(), high1_size, MPI_INT, 0, 8, comm, MPI_STATUS_IGNORE);
        MPI_Recv(high2_recv.coefficients.data(), high2_size, MPI_INT, 0, 9, comm, MPI_STATUS_IGNORE);

        Polynomial z1 = high1_recv * high2_recv;
        
        MPI_Send(z1.coefficients.data(), z1.coefficients.size(), MPI_INT, 0, 13, comm);

    } else if (rank == 3) {
        int l1h1_size, l2h2_size;
        MPI_Recv(&l1h1_size, 1, MPI_INT, 0, 4, comm, MPI_STATUS_IGNORE);
        MPI_Recv(&l2h2_size, 1, MPI_INT, 0, 5, comm, MPI_STATUS_IGNORE);

        Polynomial l1h1_recv, l2h2_recv;
        l1h1_recv.coefficients.resize(l1h1_size);
        l1h1_recv.setDegree(l1h1_size - 1);
        l2h2_recv.coefficients.resize(l2h2_size);
        l2h2_recv.setDegree(l2h2_size - 1);
        
        MPI_Recv(l1h1_recv.coefficients.data(), l1h1_size, MPI_INT, 0, 10, comm, MPI_STATUS_IGNORE);
        MPI_Recv(l2h2_recv.coefficients.data(), l2h2_size, MPI_INT, 0, 11, comm, MPI_STATUS_IGNORE);

        Polynomial z2 = l1h1_recv * l2h2_recv;

        MPI_Send(z2.coefficients.data(), z2.coefficients.size(), MPI_INT, 0, 14, comm);
    }

    MPI_Barrier(comm);

    if(rank == 0) {
        Polynomial z4 = z2 - z1 - z0;

        vector<int> result(2 * n + 1, 0);
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
    
    return Polynomial();
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    try{
        Polynomial test1;
        Polynomial test2;
        Polynomial result;
        Timer timer;
        Polynomial correct_result;

        test1 = Polynomial::generatePolynomial(POLY1_SIZE);
        test2 = Polynomial::generatePolynomial(POLY2_SIZE);
        correct_result = (test1 * test2);

        if(rank == 0) {
            timer.startTimer();
        }
        
        result = polynomialMultiplicationMT(MPI_COMM_WORLD, rank, 4, test1, test2);

        if(rank == 0) {
            auto elapsed_seconds = timer.endTimer();

            cout << "Polynomial multiplication\n";
            cout << "Elapsed time: " << elapsed_seconds << "ms\n";

            // cout << "this is the buggiest piece of junk ive used yet it's amazingly horrible hope i never touch this again\n";
            if(result != correct_result)
                throw "Incorrect multiplication - Karatsuba single";

            timer.startTimer();
        }
        
        MPI_Barrier(MPI_COMM_WORLD);

        result = karatsuba(MPI_COMM_WORLD, rank, size, test1, test2);
        
        if(rank == 0) {
            auto elapsed_seconds = timer.endTimer();

            cout << "Karatsuba polynomial multiplication\n";
            cout << "Elapsed time: " << elapsed_seconds << "ms\n";
            
            if(result != correct_result)
                throw "Incorrect multiplication - Karatsuba single";
        }

    }catch(const char* message) {
        cout << message << "\n";
    }

    MPI_Finalize();
    return 0;
}
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

#define POLY1_SIZE 200
#define POLY2_SIZE 200

class Timer
{
private:
    chrono::system_clock::time_point start;
    chrono::system_clock::time_point end;

public:
    Timer() {}

    void startTimer()
    {
        start = chrono::system_clock::now();
    }

    int64_t endTimer()
    {
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

    void setDegree(int degree)
    {
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
        for (int i = 0; i < size; ++i)
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

bool segmentMultiplication(Polynomial &p1, Polynomial &p2, vector<int> &result_coefficients, int min_pos, int max_pos)
{
    int pol1_size = p1.getDegree(), pol2_size = p2.getDegree();

    for (int d1 = 0; d1 <= pol1_size && d1 <= max_pos; ++d1)
    {
        for (int d2 = 0; d2 <= pol2_size && d1 + d2 <= max_pos; ++d2)
        {
            if (d1 + d2 >= min_pos)
                result_coefficients[d1 + d2] = (result_coefficients[d1 + d2] + p1.coefficients[d1] * p2.coefficients[d2]);
            else
                d2 = min_pos - d1 - 1;
        }
    }

    return true;
}

Polynomial polynomialMultiplicationMT(MPI_Comm comm, int rank, int size, Polynomial &p1, Polynomial &p2)
{
    int max_degree = p1.getDegree() + p2.getDegree() + 1;
    int pol1_size = p1.getDegree(), pol2_size = p2.getDegree();
    vector<int> result_coefficients(max_degree, 0);
    pol1_size++, pol2_size++;
    int increment = max_degree / size;
    increment = increment > 0 ? increment : 1;
    vector<int> buffer(increment * 2);

    if (rank == 0)
    {
        for (int currentRank = 1, start = increment; start <= max_degree && currentRank < size; start += increment, ++currentRank)
        {
            pol1_size = p1.coefficients.size();
            MPI_Ssend(&pol1_size, 1, MPI_INT, currentRank, 11, comm);
            MPI_Ssend(p1.coefficients.data(), p1.coefficients.size(), MPI_INT, currentRank, 12, comm);

            MPI_Ssend(&pol2_size, 1, MPI_INT, currentRank, 13, comm);
            MPI_Ssend(p2.coefficients.data(), pol2_size, MPI_INT, currentRank, 14, comm);

            int start_point = start;
            MPI_Ssend(&start_point, 1, MPI_INT, currentRank, currentRank * 100, comm);
        }

        segmentMultiplication(p1, p2, result_coefficients, 0, increment);

        for (int currentRank = 1, start = increment; start <= max_degree && currentRank < size; start += increment, ++currentRank)
        {
            MPI_Recv(buffer.data(), buffer.size(), MPI_INT, currentRank, currentRank * 10000, comm, MPI_STATUS_IGNORE);

            if(currentRank < size - 1)
                for (int i = 0; i <= increment && start + i < result_coefficients.size(); ++i)
                    result_coefficients[start + i] = buffer[i];
            else for (int i = 0; start + i < result_coefficients.size(); ++i)
                    result_coefficients[start + i] = buffer[i];
        }
    }
    else
    {
        int start, p1_size, p2_size;

        MPI_Recv(&p1_size, 1, MPI_INT, 0, 11, comm, MPI_STATUS_IGNORE);
        vector<int> p1_data(p1_size);
        MPI_Recv(p1_data.data(), p1_size, MPI_INT, 0, 12, comm, MPI_STATUS_IGNORE);

        MPI_Recv(&p2_size, 1, MPI_INT, 0, 13, comm, MPI_STATUS_IGNORE);
        vector<int> p2_data(p2_size);
        MPI_Recv(p2_data.data(), p2_size, MPI_INT, 0, 14, comm, MPI_STATUS_IGNORE);
        
        Polynomial p1_recv(p1_data), p2_recv(p2_data);

        MPI_Recv(&start, 1, MPI_INT, 0, rank * 100, comm, MPI_STATUS_IGNORE);

        if(rank == size - 1) { 
            segmentMultiplication(p1_recv, p2_recv, result_coefficients, start, result_coefficients.size() - 1);
            vector<int> result(result_coefficients.begin() + start, result_coefficients.end());
            MPI_Ssend(result.data(), result.size(), MPI_INT, 0, 10000 * rank, comm);
        } else {
            segmentMultiplication(p1_recv, p2_recv, result_coefficients, start, start + increment);
            vector<int> result(result_coefficients.begin() + start, result_coefficients.begin() + start + increment + 1);
            MPI_Ssend(result.data(), result.size(), MPI_INT, 0, 10000 * rank, comm);
        }
    }

    return Polynomial(result_coefficients);
}

Polynomial karatsuba_rec(Polynomial &p1, Polynomial &p2)
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

    Polynomial z0 = karatsuba_rec(low1, low2);
    Polynomial z1 = karatsuba_rec(high1, high2);
    Polynomial z2 = karatsuba_rec(l1h1, l2h2);

    Polynomial z4 = (z2 - z1 - z0);

    vector<int> result(2 * n - 1, 0);

    for (int i = 0; i < z0.coefficients.size(); ++i)
    {
        result[i] += z0.coefficients[i];
    }

    for (int i = 0; i < z4.coefficients.size(); ++i)
    {
        result[i + mid] += z4.coefficients[i];
    }

    for (int i = 0; i < z1.coefficients.size(); ++i)
    {
        result[i + 2 * mid] += z1.coefficients[i];
    }

    Polynomial result_polynomial = Polynomial(result);
    result_polynomial.setDegree(p1.getDegree() + p2.getDegree());
    return result_polynomial;
}

Polynomial karatsuba(MPI_Comm comm, int rank, int size, Polynomial &p1, Polynomial &p2)
{
    int n = max(p1.getDegree(), p2.getDegree());
    if (n < 65)
    {
        return p1 * p2;
    }

    Polynomial z0, z1, z2;
	if(rank == 0) {
        int size = pow(2, ceil(log2(max(p1.getDegree(), p2.getDegree()) + 1)));
		p1.padToSize(size);
		p2.padToSize(size);
        n = size;
    }
	
    int mid = n / 2;
	int first_child = rank * 2 + 1;
	int parent = (rank - 1) / 2;
	
	if(rank != 0) {
		int p1_size, p2_size;
        MPI_Recv(&p1_size, 1, MPI_INT, parent, 0, comm, MPI_STATUS_IGNORE);
        MPI_Recv(&p2_size, 1, MPI_INT, parent, 1, comm, MPI_STATUS_IGNORE);

        Polynomial p1_recv, p2_recv;
        p1_recv.coefficients.resize(p1_size);
        p1_recv.setDegree(p1_size - 1);
        p2_recv.coefficients.resize(p2_size);
        p2_recv.setDegree(p2_size - 1);
        
        int n = max(p1_recv.getDegree(), p2_recv.getDegree()) + 1;

        MPI_Recv(p1_recv.coefficients.data(), p1_size, MPI_INT, parent, 2, comm, MPI_STATUS_IGNORE);
        MPI_Recv(p2_recv.coefficients.data(), p2_size, MPI_INT, parent, 3, comm, MPI_STATUS_IGNORE);

		int mid = n / 2;

		vector<int> A0;
		vector<int> A1;
		vector<int> B0;
		vector<int> B1;
		
		if(first_child < size) {
			int child = first_child;
			A0 = vector<int>(p1_recv.coefficients.begin(), p1_recv.coefficients.begin() + mid);
			B0 = vector<int>(p2_recv.coefficients.begin(), p2_recv.coefficients.begin() + mid);
			
			int low1_size = A0.size();
			int low2_size = B0.size();

            MPI_Send(&low1_size, 1, MPI_INT, child, 0, comm);
			MPI_Send(&low2_size, 1, MPI_INT, child, 1, comm);

			MPI_Send(A0.data(), low1_size, MPI_INT, child, 2, comm);		
			MPI_Send(B0.data(), low2_size, MPI_INT, child, 3, comm);
		
		} else {
			Polynomial result = karatsuba_rec(p1_recv, p2_recv);
			MPI_Send(result.coefficients.data(), result.coefficients.size(), MPI_INT, parent, 12, comm);
			return Polynomial();
		}
		
		int second_child = first_child + 1;

		A1 = vector<int>(p1_recv.coefficients.begin() + mid, p1_recv.coefficients.end());
		B1 = vector<int>(p2_recv.coefficients.begin() + mid, p2_recv.coefficients.end());
			
		int low1_size = A0.size();
		int low2_size = B0.size();
		int high1_size = A1.size();
		int high2_size = B1.size();
		
		if(second_child < size) {
			int child = second_child;

			A1 = vector<int>(p1_recv.coefficients.begin() + mid, p1_recv.coefficients.end());
			B1 = vector<int>(p2_recv.coefficients.begin() + mid, p2_recv.coefficients.end());
			
			int high1_size = A1.size();
			int high2_size = B1.size();
	
			MPI_Send(&high1_size, 1, MPI_INT, child, 0, comm);
			MPI_Send(&high2_size, 1, MPI_INT, child, 1, comm);

			MPI_Send(A1.data(), high1_size, MPI_INT, child, 2, comm);
			MPI_Send(B1.data(), high2_size, MPI_INT, child, 3, comm);
			
		} else {
            Polynomial low1(A0), low2(B0);
			Polynomial high1(A1), high2(B1);

			z1 = karatsuba_rec(high1, high2);
            Polynomial l1h1 = low1 + high1, l2h2 = low2 + high2;
            z2 = karatsuba_rec(l1h1, l2h2);

            // receive low part from first child
			z0.coefficients.resize(low1_size + low2_size - 1);
			z0.setDegree(low1_size + low2_size - 2);
			MPI_Recv(z0.coefficients.data(), z0.coefficients.size(), MPI_INT, first_child, 12, comm, MPI_STATUS_IGNORE);

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

            MPI_Send(result.data(), result.size(), MPI_INT, parent, 12, comm);
			return Polynomial();
		}
        
        Polynomial low1(A0), low2(B0);
		Polynomial high1(A1), high2(B1);
        Polynomial l1h1 = low1 + high1, l2h2 = low2 + high2;

		z2 = karatsuba_rec(l1h1, l2h2);

		// receive low part from first child
		z0.coefficients.resize(low1_size + low2_size - 1);
		z0.setDegree(low1_size + low2_size - 2);
		MPI_Recv(z0.coefficients.data(), z0.coefficients.size(), MPI_INT, first_child, 12, comm, MPI_STATUS_IGNORE);

		// receive high part from second child		
		z1.coefficients.resize(high1_size + high2_size - 1);
		z1.setDegree(high1_size + high2_size - 2);
		MPI_Recv(z1.coefficients.data(), z1.coefficients.size(), MPI_INT, second_child, 12, comm, MPI_STATUS_IGNORE);
			
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
			
		MPI_Send(result.data(), result.size(), MPI_INT, parent, 12, comm);
		return Polynomial();

	} else {
		vector<int> A0(p1.coefficients.begin(), p1.coefficients.begin() + mid);
		vector<int> A1(p1.coefficients.begin() + mid, p1.coefficients.end());
		vector<int> B0(p2.coefficients.begin(), p2.coefficients.begin() + mid);
		vector<int> B1(p2.coefficients.begin() + mid, p2.coefficients.end());

		Polynomial low1(A0), high1(A1), low2(B0), high2(B1);
		Polynomial l1h1 = low1 + high1;
		Polynomial l2h2 = low2 + high2;

		int low1_size = low1.coefficients.size();
		int low2_size = low2.coefficients.size();
		int high1_size = high1.coefficients.size();
		int high2_size = high2.coefficients.size();
		int l1h1_size = l1h1.coefficients.size();
		int l2h2_size = l2h2.coefficients.size();

        int first_child = 1, second_child = 2;
        if(first_child < size) {
            MPI_Send(&low1_size, 1, MPI_INT, first_child, 0, comm);
            MPI_Send(&low2_size, 1, MPI_INT, first_child, 1, comm);

            MPI_Send(low1.coefficients.data(), low1_size, MPI_INT, first_child, 2, comm);
            MPI_Send(low2.coefficients.data(), low2_size, MPI_INT, first_child, 3, comm);
        }

        if(second_child < size) {
            MPI_Send(&high1_size, 1, MPI_INT, second_child, 0, comm);
            MPI_Send(&high2_size, 1, MPI_INT, second_child, 1, comm);
            
            MPI_Send(high1.coefficients.data(), high1_size, MPI_INT, second_child, 2, comm);
            MPI_Send(high2.coefficients.data(), high2_size, MPI_INT, second_child, 3, comm);
        }

        z2 = karatsuba_rec(l1h1, l2h2);
   
        if(second_child < size) {
            z1.coefficients.resize(high1_size + high2_size - 1);
            z1.setDegree(high1_size + high2_size - 2);
            MPI_Recv(z1.coefficients.data(), z1.coefficients.size(), MPI_INT, second_child, 12, comm, MPI_STATUS_IGNORE);
        } else {
            z1 = karatsuba_rec(high1, high2);
        }

        if(first_child < size) {
            z0.coefficients.resize(low1_size + low2_size - 1);
            z0.setDegree(low1_size + low2_size - 2);
            MPI_Recv(z0.coefficients.data(), z0.coefficients.size(), MPI_INT, first_child, 12, comm, MPI_STATUS_IGNORE);
        } else {
            z0 = karatsuba_rec(low1, low2);
        }

        Polynomial z4 = z2 - z1 - z0;

        vector<int> result(2 * n + 1, 0);
        for (int i = 0; i < z0.coefficients.size(); ++i)
        {
            result[i] += z0.coefficients[i];
        }

        for (int i = 0; i < z4.coefficients.size(); ++i)
        {
            result[i + mid] += z4.coefficients[i];
        }

        for (int i = 0; i < z1.coefficients.size(); ++i)
        {
            result[i + 2 * mid] += z1.coefficients[i];
        }

        Polynomial result_polynomial = Polynomial(result);
        result_polynomial.setDegree(p1.getDegree() + p2.getDegree());
        return result_polynomial;
    }

    return Polynomial();
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    try
    {
        Polynomial test1;
        Polynomial test2;
        Polynomial result;
        Timer timer;
        Polynomial correct_result;

        test1 = Polynomial::generatePolynomial(POLY1_SIZE);
        test2 = Polynomial::generatePolynomial(POLY2_SIZE);
        correct_result = (test1 * test2);

        if (rank == 0)
        {
            timer.startTimer();
        }

        result = polynomialMultiplicationMT(MPI_COMM_WORLD, rank, size, test1, test2);

        if (rank == 0)
        {
            auto elapsed_seconds = timer.endTimer();

            cout << "Polynomial multiplication\n";
            cout << "Elapsed time: " << elapsed_seconds << "ms\n";

            if (result != correct_result)
            {
                cout << result.toString() << endl;
                cout << "Incorrect multiplication - On2 single" << endl;
            }

            timer.startTimer();
        }

        MPI_Barrier(MPI_COMM_WORLD);

        result = karatsuba(MPI_COMM_WORLD, rank, size, test1, test2);

        if (rank == 0)
        {
            auto elapsed_seconds = timer.endTimer();

            cout << "Karatsuba polynomial multiplication\n";
            cout << "Elapsed time: " << elapsed_seconds << "ms\n";

            if (result != correct_result)
            {
                cout << result.toString() << endl;
                cout << "Incorrect multiplication - On2 single" << endl;
            }
        }
    }
    catch (const char *message)
    {
        cout << message << "\n";
    }

    MPI_Finalize();
    return 0;
}
#include <iostream>
#include <thread>
#include <mpi.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <set>
#include <string>

using namespace std;

atomic<bool> thread_finished = false;

class SharedVariable {
private:
    atomic<int> value;

public:
    set<int> subcribers;

    SharedVariable() : value(0) {}

    SharedVariable(int initial_value) : value(initial_value) {}

    SharedVariable(const SharedVariable& other) 
        : value(other.value.load()), subcribers(other.subcribers) {}

    SharedVariable(SharedVariable&& other) noexcept
        : value(other.value.load()), subcribers(std::move(other.subcribers)) {}

    SharedVariable& operator=(const SharedVariable& other) {
        if (this != &other) {
            value.store(other.value.load());
            subcribers = other.subcribers;
        }
        return *this;
    }

    // Move assignment operator
    SharedVariable& operator=(SharedVariable&& other) noexcept {
        if (this != &other) {
            value = other.value.load();
            subcribers = std::move(other.subcribers);
        }
        return *this;
    }

    enum MessageCode
    {
        CLOSE_CONNECTION,
        WRITE_VALUE,
        COMPARE_AND_EXCHANGE,
        REGISTER_SUBSCRIBER,
        NEW_SUBSCRIBER,
        REMOVE_SUBSCRIBER,
    };

    int getValue() {
        return value.load();
    }

    void writeValue(int new_value) {
        value.store(new_value);
    }

    void compareAndExchange(int compare_to_value, int new_value) {
        value.compare_exchange_strong(compare_to_value, new_value);
    }

    void addSubscriber(int node_id) {
        subcribers.insert(node_id);
    }
};

class DSM {
private:
    vector<SharedVariable> variables;
    int rank;
    int size;

public:
    DSM(int rank, int size, int variable_count) {
        this->rank = rank;
        this->size = size;

        for (int i = 0; i < variable_count; ++i)
            variables.push_back(SharedVariable());
    }

    void write(int variable_index, int new_value) {
        variables[variable_index].writeValue(new_value);
        int status_code = SharedVariable::MessageCode::WRITE_VALUE;
        MPI_Send(&status_code, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        MPI_Send(&variable_index, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&new_value, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
    }

    void compareAndExchange(int variable_index, int new_value, int compare_to_value) {
        variables[variable_index].compareAndExchange(compare_to_value, new_value);
        int status_code = SharedVariable::MessageCode::COMPARE_AND_EXCHANGE;
        MPI_Send(&status_code, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        MPI_Send(&variable_index, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&new_value, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Send(&compare_to_value, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
    }

    int load(int variable_index) {
        return variables[variable_index].getValue();
    }

    void close() {
        int status_code = SharedVariable::MessageCode::CLOSE_CONNECTION;

        if(rank > 0)
            MPI_Send(&status_code, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    }

    void subscribe(int variable_index, int rank) {
        if(variable_index < 0 || variable_index >= variables.size())
            cout << "[" << rank << "] Invalid variable index " << variable_index << endl;
            
        else {
            variables[variable_index].addSubscriber(rank);
            int status_code = SharedVariable::MessageCode::REGISTER_SUBSCRIBER;
            MPI_Send(&status_code, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(&variable_index, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            MPI_Send(&rank, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        }
    }

    string inspect() {
        string result = "DSM {\n";

        for (int i = 0; i < variables.size(); ++i)
        {
            SharedVariable& variable = variables[i];
            if(variable.subcribers.find(rank) != variable.subcribers.end() || rank == 0) {
                result += "var[" + to_string(i);
                result += "] with value " + to_string(variable.getValue());
                result += ", subscribers: {";

                set<int>::iterator itr;
                for (itr = variable.subcribers.begin(); itr != variable.subcribers.end(); itr++) {
                    result += " " + to_string(*itr);                
                }
                
                result += " }\n";
            }
        }

        return result + "}";
    }

    void listenForChanges() {
        cout << "[" << rank << "] Listener started" << endl;
        int connection_closed = false;

        int status_code;
        int variable_index = 0;
        int i;
        int subscriber, new_value, compare_to_value;
        int closed_connections = 0;

        while(!connection_closed) {
            MPI_Recv(&status_code, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            switch(status_code) {
                case SharedVariable::MessageCode::CLOSE_CONNECTION:
                    closed_connections++;
                    if(rank == 0) {
                        if(closed_connections == size - 1) {
                            status_code = SharedVariable::MessageCode::CLOSE_CONNECTION;

                            for (int i = 1; i < size; ++i)
                                MPI_Send(&status_code, 1, MPI_INT, i, 1, MPI_COMM_WORLD);

                            connection_closed = true;
                        }
                    
                    } else
                        connection_closed = true;

                    break;

                case SharedVariable::MessageCode::WRITE_VALUE:
                    MPI_Recv(&variable_index, 1, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&new_value, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    variables[variable_index].writeValue(new_value);

                    if(rank == 0) {
                        set<int>::iterator itr;
                        for (itr = variables[variable_index].subcribers.begin(); itr != variables[variable_index].subcribers.end(); itr++) {
                            int i = *itr;
                            status_code = SharedVariable::MessageCode::WRITE_VALUE;
                            MPI_Send(&status_code, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
                            MPI_Send(&variable_index, 1, MPI_INT, i, 2, MPI_COMM_WORLD);
                            MPI_Send(&new_value, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
                            cout << "[" << rank << "] Sending to [" << i << "]: write value " << new_value << " to variable " << variable_index << endl; 
                        }
                    } else 
                        cout << "[" << rank << "] Writing value " << new_value << " to variable " << variable_index << endl; 

                    break;

                case SharedVariable::MessageCode::COMPARE_AND_EXCHANGE:
                    MPI_Recv(&variable_index, 1, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&new_value, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&compare_to_value, 1, MPI_INT, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    variables[variable_index].compareAndExchange(compare_to_value, new_value);

                    if(rank == 0) {
                        set<int>::iterator itr;
                        for (itr = variables[variable_index].subcribers.begin(); itr != variables[variable_index].subcribers.end(); itr++) {
                            int i = *itr;

                            status_code = SharedVariable::MessageCode::COMPARE_AND_EXCHANGE;
                            MPI_Send(&status_code, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
                            MPI_Send(&variable_index, 1, MPI_INT, i, 2, MPI_COMM_WORLD);
                            MPI_Send(&new_value, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
                            MPI_Send(&compare_to_value, 1, MPI_INT, i, 4, MPI_COMM_WORLD);

                            cout << "[" << rank << "] Sending to [" << i << "]: compare and exchange for variable " << variable_index << ", expecting value " << compare_to_value << " to be replaced with " << new_value << endl;    
                        }
                    
                    } else 
                        cout << "[" << rank << "] Compare and exchange for variable " << variable_index << ", expecting value " << compare_to_value << " to be replaced with " << new_value << endl;    


                    break;

                case SharedVariable::MessageCode::NEW_SUBSCRIBER:
                    MPI_Recv(&variable_index, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&subscriber, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    if(variable_index < 0 || variable_index >= variables.size())
                        cout << "[" << rank << "] Invalid variable index " << variable_index << endl;
                    else {
                        variables[variable_index].addSubscriber(subscriber);
                        cout << "[" << rank << "] New subscriber [" << subscriber << "] for variable " << variable_index << endl;    
                    }
        
                    break;

                case SharedVariable::MessageCode::REGISTER_SUBSCRIBER:
                    MPI_Recv(&variable_index, 1, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&subscriber, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    for (i = 1; i < size; ++i) {
                            variables[variable_index].addSubscriber(subscriber);
                            status_code = SharedVariable::MessageCode::NEW_SUBSCRIBER;
                            MPI_Send(&status_code, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
                            MPI_Send(&variable_index, 1, MPI_INT, i, 2, MPI_COMM_WORLD);
                            MPI_Send(&subscriber, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
                        }

                        cout << "[" << rank << "] Notification of new subscriber [" << subscriber << "] for variable " << variable_index << endl;    


                    break;

                default:
                    cout << "[" << rank << "] Unknown status code " << status_code << endl;
                    connection_closed = true;
                    break;
            }
        }

        cout << "[" << rank << "] Listener finished execution" << endl;
        thread_finished = true;
    }
};

void startDSM(int rank, int size) {
    DSM dsm(rank, size, 4);

    thread listener = thread([&](){ dsm.listenForChanges(); });

    switch(rank) {
        case 1:
            dsm.subscribe(0, rank);
            dsm.subscribe(1, rank);
            dsm.subscribe(2, rank);
            dsm.subscribe(3, rank);
            break;
        
        case 2:
            dsm.subscribe(0, rank);
            dsm.subscribe(1, rank);
            break;

        case 3:
            dsm.subscribe(0, rank);
            dsm.subscribe(3, rank);
            break;

        case 4:
            dsm.subscribe(2, rank);
            dsm.subscribe(3, rank);

        default:
            break;
    }

    this_thread::sleep_for(chrono::milliseconds(2000));

    int iteration = 0;
    while (!thread_finished)
    {
        switch(rank) {
            case 1:
                switch(iteration) {
                    case 0:
                        dsm.write(0, -5);
                        dsm.write(1, -10);
                        dsm.write(3, -15);
                        break;
                    case 1:
                        dsm.write(1, 5);
                        break;
                    case 2:
                        dsm.write(2, 100);
                        break;
                }

                break;
            
            case 2:
                switch(iteration) {
                    case 0:
                        dsm.compareAndExchange(0, 62, -5);
                        dsm.compareAndExchange(1, 72, -10);
                        break;

                    case 1:
                        dsm.write(1, 27);
                        break;
                    
                    case 2:
                        dsm.write(1, 100);
                        break;
                }

                break;

            case 3:
                switch(iteration) {
                    case 1:
                        dsm.compareAndExchange(3, 15, -15);
                        break;
                    
                    case 2:
                        dsm.compareAndExchange(0, 99, 62);
                        dsm.compareAndExchange(0, 999, -5);
                        break;
                }

                break;

            default:
                break;
        }

        if(iteration > 2)
            dsm.close();

        cout << "[" << rank << "] Iteration[" << iteration << "] " << dsm.inspect() << "\n" << endl;

        ++iteration;
        this_thread::sleep_for(chrono::milliseconds(400));
    }

    listener.join();
}

int main(int argc, char** argv) {
    int provided = MPI_THREAD_SERIALIZED;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    startDSM(rank, size);

    MPI_Finalize();
    return 0;
}
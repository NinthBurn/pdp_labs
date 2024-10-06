#include <iostream>
#include <thread>
#include <utility>
#include <cstdlib>
#include <ctime>
#include "entities.h"

using namespace std;

bool debugPrint = false;

#define THREAD_COUNT 4

// minimum number of operations per thread
#define THREAD_OPERATIONS 400000

// randomise number of operations: THREAD_OPERATIONS * rand(1, 5)
#define THREAD_RANDOMIZE_COUNT false

// delay between each integrity check
#define INTEGRITY_CHECK_DELAY_MS 10

// number of products to generate 
#define PRODUCT_COUNT 20000

// run the slower integrity check method
#define SLOW_CHECK false

// Asynchronous output https://stackoverflow.com/a/45046349
struct Acout
{
    mutex object_lock;

    template <typename T>
    Acout &operator<<(const T &_t)
    {
        object_lock.lock();
        cout << _t;
        object_lock.unlock();
        return *this;
    }

    Acout &operator<<(ostream &(*fp)(ostream &))
    {
        object_lock.lock();
        cout << fp;
        object_lock.unlock();
        return *this;
    }
} acout;


/// @brief Runs an inventory check to make sure current data is consistent with all registered bills.
/// @param product_database Database of products, pair of id & associated product
/// @param bills List with all the bills
void inventoryCheck(map<int, shared_ptr<Product>> &product_database, shared_ptr<BillList> bills)
{
    long long int moneyFromDatabase = 0, moneyFromBills = 0;

    for (const auto &[id, val] : product_database)
    {
        // lock operations for this product while performing the check
        val->lock();

        int quantity_db, price = val->getPrice();
        quantity_db = val->getInitialQuantity() - val->getQuantity();
        moneyFromDatabase = price * quantity_db;

        int quantity_bills = bills->getTotalQuantityFor(id);
        moneyFromBills = price * quantity_bills;

        val->unlock();

        if (moneyFromDatabase != moneyFromBills)
        {
            acout << "==============================================\n";
            acout << "  Consistency check failed\n";
            acout << "  Product ID: " << id << "\n";
            acout << "  Database quantity: " << quantity_db << "\n";
            acout << "  Bill quantity: " << quantity_bills << "\n";
            acout << "  Database amount: " << moneyFromDatabase << "\n";
            acout << "  Bill amount: " << moneyFromBills << "\n";
            acout << "==============================================\n";
            return;
        }
    }

    if(debugPrint) {
        acout << "==============================================\n";
        acout << "      Consistency check is successful\n";
        acout << "==============================================\n";
    }else acout << "    - - - - Consistency check is successful - - - -\n";
}

/// @brief Runs an inventory check to make sure current data is consistent with all registered bills. (slower version - locks all data first, then checks)
/// @param product_database Database of products, pair of id & associated product
/// @param bills List with all the bills
void inventoryCheckSlow(map<int, shared_ptr<Product>> &product_database, shared_ptr<BillList> bills)
{
    long long int moneyFromDatabase = 0, moneyFromBills = 0;

    for (const auto &[id, val] : product_database)
        val->lock();

    for (const auto &[id, val] : product_database)
    {
        int quantity_db, price = val->getPrice();
        quantity_db = val->getInitialQuantity() - val->getQuantity();
        moneyFromDatabase = price * quantity_db;

        int quantity_bills = bills->getTotalQuantityFor(id);
        moneyFromBills = price * quantity_bills;

        if (moneyFromDatabase != moneyFromBills)
        {
            acout << "==============================================\n";
            acout << "  Consistency check failed\n";
            acout << "  Product ID: " << id << "\n";
            acout << "  Database quantity: " << quantity_db << "\n";
            acout << "  Bill quantity: " << quantity_bills << "\n";
            acout << "  Database amount: " << moneyFromDatabase << "\n";
            acout << "  Bill amount: " << moneyFromBills << "\n";
            acout << "==============================================\n";
            return;
        }
    }

    for (const auto &[id, val] : product_database)
        val->unlock();

    acout << "==============================================\n";
    acout << "      Consistency check is successful\n";
    acout << "==============================================\n";
}

/// @brief Method for thread to run an inventory check.
/// @param product_database Database of products, pair of id & associated product
/// @param bills List with all the bills
/// @param still_executing Thread will keep checking the data until this variable is set to false by the main thread.
void inventoryCheckThread(map<int, shared_ptr<Product>> &product_database, shared_ptr<BillList> bills, bool &still_executing)
{
    void (*inventoryCheckFunc)(map<int, shared_ptr<Product>>&, shared_ptr<BillList>);

    if(SLOW_CHECK)
        inventoryCheckFunc = inventoryCheckSlow;
    else inventoryCheckFunc = inventoryCheck;

    while (still_executing)
    {
        this_thread::sleep_for(chrono::milliseconds(INTEGRITY_CHECK_DELAY_MS));
        inventoryCheckFunc(ref(product_database), bills);
    }
}

/// @brief Method for threads that run sale operations. Will do THREAD_OPERATIONS * rand(1, 5) transactions until shutdown, buying a random number of a product for each transaction. 
/// @param product_database Database of products, pair of id & associated product
/// @param bills List with all the bills
/// @param shop_account Does nothing
/// @param seed Seed used for random number generation
void threadWork(map<int, shared_ptr<Product>> const &product_database, shared_ptr<BillList> bills, shared_ptr<ShopBankAccount> shop_account, int seed)
{
    auto tid = this_thread::get_id();
    acout << "[T" << tid << "] " << "Starting execution\n";
    srand(seed);

    int size = product_database.size();
    int key, count;
    Product *product;
    int amount;

    shared_ptr<Bill> bill(new Bill());
    bills->registerBill(bill);

    if(debugPrint)
        acout << "[T" << tid << "] " << "Will be purchasing products " << count << " times\n";
    
    if(THREAD_RANDOMIZE_COUNT)
        count = THREAD_OPERATIONS * (rand() % 5 + 1);
    else count = THREAD_OPERATIONS;

    while (count > 0)
    {
        count--;
        key = rand() % size + 1;

        product = product_database.at(key).get();

        product->lock();
        amount = product->purchase(rand() % 100 + 1);
        shop_account->registerTransaction(amount * product->getPrice());
        bill->addProduct(key, amount, product->getPrice());
        product->unlock();

        if(debugPrint)
            acout << "[T" << tid << "] " << "Purchased " << amount << " of product " << key << "\n";
    }

    acout << "[T" << tid << "] Finished execution\n";
}

/// @brief Method for generating test data
map<int, shared_ptr<Product>> getProducts()
{
    map<int, shared_ptr<Product>> map;

    for (int index = 1; index < PRODUCT_COUNT + 1; ++index)
        map.insert(pair(
            index,
            shared_ptr<Product>(new Product(index, rand() % 100 + 1, 100 * index, "Product" + to_string(index)))));

    return map;
}

int main()
{
    acout << "Main thread " << this_thread::get_id() << "\n";

    bool still_executing = true;
    vector<thread> children;

    map<int, shared_ptr<Product>> product_database = getProducts();

    shared_ptr<BillList> bills(new BillList());
    shared_ptr<ShopBankAccount> shop_account(new ShopBankAccount());

    acout << "[MAIN] Starting more threads\n";
    auto start = std::chrono::system_clock::now();
    auto start_time = std::chrono::system_clock::to_time_t(start);
    acout << "[MAIN] Start time: " << ctime(&start_time) << "\n";

    for (int index = 0; index < THREAD_COUNT; ++index)
        children.push_back(thread(threadWork, ref(product_database), bills, shop_account, rand() % 20000));

    thread stateValidator(inventoryCheckThread, ref(product_database), bills, ref(still_executing));

    for (thread &child : children)
    {
        child.join();
    }

    auto end = std::chrono::system_clock::now();
    auto end_time = std::chrono::system_clock::to_time_t(end);
    std::chrono::duration<double> elapsed_seconds = end-start;
    acout << "[MAIN] Sales finish time: " << ctime(&end_time) << "\n";
    acout << "[MAIN] Sales elapsed time: " << elapsed_seconds.count() << "\n";

    still_executing = false;
    stateValidator.join();
    inventoryCheck(ref(product_database), bills);

    end = std::chrono::system_clock::now();
    end_time = std::chrono::system_clock::to_time_t(end);
    elapsed_seconds = end-start;
    acout << "[MAIN] Finish time: " << ctime(&end_time) << "\n";
    acout << "[MAIN] Total elapsed time: " << elapsed_seconds.count() << "\n";

    return 0;
}
#include <vector>
#include <string>
#include <mutex>
#include <map>
#include <memory>

class Product
{
private:
    int price;
    int id;
    int quantity;
    int initial_quantity;
    std::string name;
    std::mutex object_lock;

    static inline int last_available_id = 1;

public:
    Product()
    {
        this->id = Product::last_available_id++;
        this->price = 0;
        this->quantity = 0;
        this->initial_quantity = 0;
        this->name = "";
    }

    Product(int price, int quantity, std::string name) : price{price},
                                                    quantity{quantity}, initial_quantity{quantity}, name{name}
    {
        this->id = Product::last_available_id++;
    }

    Product(int id, int price, int quantity, std::string name) : price{price},
                                                            quantity{quantity}, initial_quantity{quantity}, name{name}, id{id} {};

    int purchase(int amount)
    {
        this->quantity -= amount;
        if (this->quantity < 0)
        {
            amount = amount + quantity;
            this->quantity = 0;
        }

        return amount;
    }

    int getId()
    {
        return this->id;
    }

    int getPrice()
    {
        return this->price;
    }

    std::string getName()
    {
        return this->name;
    }

    int getQuantity()
    {
        return this->quantity;
    }

    int getInitialQuantity()
    {
        return this->initial_quantity;
    }

    bool tryLock()
    {
        return this->object_lock.try_lock();
    }

    void lock()
    {
        this->object_lock.lock();
    }

    void unlock()
    {
        this->object_lock.unlock();
    }

    std::string toString()
    {
        return "{ id: " + std::to_string(this->id) +
               ", name: " + this->getName() +
               ", price: " + std::to_string(this->price) +
               ", quantity: " + std::to_string(this->quantity) +
               ", initial quantity: " + std::to_string(this->quantity) +
               " }";
    }
};

class ShopBankAccount
{
private:
    long long int total = 0;
    std::mutex object_lock;

public:
    void registerTransaction(int amount)
    {
        this->object_lock.lock();
        this->total += amount;
        this->object_lock.unlock();
    }

    long long int getTotal()
    {
        long long int amount;

        this->object_lock.lock();
        amount = this->total;
        this->object_lock.unlock();

        return amount;
    }
};

class Bill
{
private:
    std::mutex object_lock;

    // <product id, purchase count>
    std::map<int, int> products;

    // bill value
    int total_amount = 0;

public:
    std::map<int, int> getBills()
    {
        this->object_lock.lock();

        auto result = this->products;

        this->object_lock.unlock();

        return result;
    }

    void addProduct(int id, int quantity, int price)
    {
        this->object_lock.lock();

        if (this->products.count(id) == 1)
            this->products[id] += quantity;
        else
            this->products[id] = quantity;

        this->total_amount += price;

        this->object_lock.unlock();
    }

    int getQuantityFor(int id)
    {
        int result = 0;

        this->object_lock.lock();

        if (this->products.count(id) == 1)
            result = this->products[id];

        this->object_lock.unlock();

        return result;
    }

    int getTotal()
    {
        int result = 0;

        this->object_lock.lock();

        result = this->total_amount;

        this->object_lock.unlock();

        return result;
    }
};

class BillList
{
private:
    std::mutex object_lock;
    std::vector<std::shared_ptr<Bill>> bills;

public:
    void registerBill(std::shared_ptr<Bill> bill)
    {
        object_lock.lock();

        bills.push_back(bill);

        object_lock.unlock();
    }

    long long int getTotalQuantityFor(int id)
    {
        long long int total = 0;

        object_lock.lock();

        for (const auto &bill : bills)
        {
            total += bill->getQuantityFor(id);
        }

        object_lock.unlock();

        return total;
    }

    long long int getTotalAmount()
    {
        long long int total = 0;

        object_lock.lock();

        for (const auto &bill : bills)
        {
            total += bill->getTotal();
        }

        object_lock.unlock();

        return total;
    }
};

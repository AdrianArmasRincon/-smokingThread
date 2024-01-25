#include <QCoreApplication>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>

std::mutex coutMutex;

// Provider class to manage ingredients on the table
class Provider {
public:
    Provider();
    void placeIngredients();
    void notifyFinishedSmoking();
    bool getIngredientsAvailable() const;
    int getIngredientOnTable() const;
    std::mutex& getMtx();
    std::condition_variable& getCv();
    std::string ingredient;

private:
    bool ingredientsAvailable;
    int ingredientOnTable;
    std::mutex mtx;
    std::condition_variable cv;
};

Provider::Provider() : ingredientsAvailable(false), ingredientOnTable(-1) {
}

// Function to randomly place two ingredients on the table
void Provider::placeIngredients() {
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(1, 3);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        int chosenIngredient = distr(eng);

        {
            std::lock_guard<std::mutex> lock(mtx);
            ingredientOnTable = chosenIngredient;
            ingredientsAvailable = true;

            // Convert the numeric representation of ingredients to strings for better readability
            if (chosenIngredient == 1) {
                ingredient = "paper";
            } else if (chosenIngredient == 2) {
                ingredient = "tobacco";
            } else if (chosenIngredient == 3) {
                ingredient = "matches";
            }

            std::cout << "Provider: Places two ingredients on the table, excluding: " << ingredient << "(" << chosenIngredient << ")" << std::endl;
        }

        cv.notify_all();

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return !ingredientsAvailable; });
        }
    }
}

// Function to notify that smoking is finished
void Provider::notifyFinishedSmoking() {
    std::lock_guard<std::mutex> lock(mtx);
    ingredientsAvailable = false;
    ingredientOnTable = -1;
    cv.notify_all();
}

// Getter functions for mutex, condition variable, and ingredient status
std::mutex& Provider::getMtx() {
    return mtx;
}

std::condition_variable& Provider::getCv() {
    return cv;
}

bool Provider::getIngredientsAvailable() const {
    return ingredientsAvailable;
}

int Provider::getIngredientOnTable() const {
    return ingredientOnTable;
}

// Smoker class to represent individual smokers
class Smoker {
public:
    Smoker(int id, Provider& provider);
    void waitForIngredientsAndSmoke();

private:
    int smokerID;
    Provider& providerRef;
};

Smoker::Smoker(int id, Provider& provider) : smokerID(id), providerRef(provider) {
}

// Function for smokers to wait for their required ingredients and then smoke
void Smoker::waitForIngredientsAndSmoke() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(providerRef.getMtx());
            providerRef.getCv().wait(lock, [this] {
                return providerRef.getIngredientsAvailable() && providerRef.getIngredientOnTable() == smokerID;
            });

            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Smoker " << smokerID << ": Picked up the ingredients, rolls a cigarette, and smokes." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));

        providerRef.notifyFinishedSmoking();
    }
}

// Main function to create instances of Provider and Smoker, and run threads
int main() {
    Provider provider;
    Smoker smoker1(1, provider), smoker2(2, provider), smoker3(3, provider);

    std::thread providerThread(&Provider::placeIngredients, &provider);
    std::thread smoker1Thread(&Smoker::waitForIngredientsAndSmoke, &smoker1);
    std::thread smoker2Thread(&Smoker::waitForIngredientsAndSmoke, &smoker2);
    std::thread smoker3Thread(&Smoker::waitForIngredientsAndSmoke, &smoker3);

    providerThread.join();
    smoker1Thread.join();
    smoker2Thread.join();
    smoker3Thread.join();

    return 0;
}

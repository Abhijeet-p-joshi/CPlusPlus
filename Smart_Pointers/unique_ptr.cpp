/* ----------------------------------------------------- */
// #include <iostream>
// #include <memory>

// class Player {
//     public: 
//         Player(std::string name) : name_(name) {
//             std::cout << "Player " << name_ << " created.\n";
//         }
//         ~Player() {
//             std::cout << "Player " << name_ << " destroyed.\n";
//         }
//         void player_name() {
//             std::cout << "Player name: " << name_ << std::endl;
//         }
//     private: 
//         std::string name_;
// };

// int main() {
//     std::unique_ptr<Player> p1 = std::make_unique<Player>("Puks");
//     std::unique_ptr<Player> p2 = std::make_unique<Player>("Akshu");

//     p1->player_name();
//     p2->player_name();

//     p1 = std::move(p2);

//     p1->player_name();
//     p2->player_name();

//     return 0;
// }

/*------------------------------------------------------ */
// #include <iostream>
// #include <memory>

// int main() {
//     std::unique_ptr<int> p1 = std::make_unique<int>(42);
//     std::unique_ptr<int> p2 = std::make_unique<int>(30);

//     std::cout << &p1 << std::endl;
//     std::cout << *p1 << std::endl;

//     std::cout << &p2 << std::endl;
//     std::cout << *p2 << std::endl;

//     p2 = std::move(p1);

//     std::cout << &p2 << std::endl;
//     std::cout << *p2 << std::endl;

//     return 0;
// }

/* ----------------------------------------------------- */

#include <iostream>
#include <memory>

void processData(std::unique_ptr<int> data) {
    std::cout << "Processing value: " << *data << "\n";
    // Data is destroyed here as the function scope ends
}

int main() {
    auto myData = std::make_unique<int>(42);

    // This would fail: processData(myData); 
    // You MUST move it because unique_ptr cannot be copied.
    processData(std::move(myData));

    if (!myData) {
        std::cout << "myData is now null. Ownership was moved to the function.\n";
    }

    return 0;
}
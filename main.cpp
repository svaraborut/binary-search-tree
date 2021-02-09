#include <iostream>

#include "bst.cpp"

int main() {
    std::cout << "This is an example code, not meant to run." << std::endl;

    bst<int, std::string> m{};
    m.insert(std::pair{1, "Hello"});
    m.has(1);
    m.erase(1);

    return 0;
}
// stack_vector_test.cpp : Defines the entry point for the application.
//
#include "stack_vector_test.h"
#include <iostream>
#include <string>

int main() {
    std::string           output = "";
    stack_vector::stack_vector<int, 5> test;
    test.emplace_back(0);
    stack_vector::stack_vector<int, 5> test_1;

    output += std::to_string((uintptr_t)&test) + "\n";
    assert(test.begin() == &test[0] && "begin iterator != begin ptr");
    assert(test.end() == ((&test[0]) + test.size()) && "end iterator != end ptr");
    assert(test.max_size() == test.capacity() && "max size != capacity");
    /*
    for (size_t i = 1; i < test.max_size(); i++)
        test.emplace_back(i);
        */
    for (size_t i = 1; i < test.max_size(); i++) {
        test.shove_back(i);
    }
    for (size_t i = 0; i < test.capacity(); i++) {
        assert(test[i] == test.begin()[i] && "iterator value != ptr value");
        assert(test[i] == i && "iterator value != assigned value!");
    }
    assert(test.size() == test.capacity() && "size != capacity");
        
    for (const auto &int_val : test) {
        output += std::to_string(int_val) + "\n";
    }
    test.pop_back();
    test.pop_back();
    assert(test.size() == (test.capacity() - 2) && "pop back did not remove element");
    output += "___\n";

    for (const auto &int_val : test) {
        output += std::to_string(int_val) + "\n";
    }

    output += "___\n";
    test_1 = {5, 6, 7, 8, 9};
    assert(test_1.size() == (5) && "intializer list constructor failed");
    for (size_t i = 0; i < test_1.size(); i++) {
        assert(test_1[i] == test_1.begin()[i] && "iterator value != ptr value");
        assert(test_1[i] == (i + 5) && "iterator value != assigned value");
    }
    for (const auto &int_val : test_1) {
        output += std::to_string(int_val) + "\n";
    }
    output += "___\n";
    std::swap(test, test_1);
    for (size_t i = 0; i < test_1.size(); i++) {
        assert(test_1[i] == test_1.begin()[i] && "iterator value != ptr value");
    }
    for (const auto &int_val : test_1) {
        output += std::to_string(int_val) + "\n";
    }
    output += "___\n";
    test_1.insert(test_1.begin() + 2, {8, 9});
    for (size_t i = 2; i < 4; i++) {
        assert(test_1[i] == (i + 6) && "iterator value != ptr value");
    }
    for (const auto &int_val : test_1) {
        output += std::to_string(int_val) + "\n";
    }
    output += "___\n";
    test_1.pop_back();
    test_1.insert(test_1.begin() + 2, 7);
    assert(test_1[2] == 7 && "iterator value != ptr value");

    for (const auto &int_val : test_1) {
        output += std::to_string(int_val) + "\n";
    }

    std::cout << output;

    return 0;
}

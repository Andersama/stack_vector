// stack_vector_test.cpp : Defines the entry point for the application.
//
#include "stack_vector.h"
//#include "sbo_vector.h"
#include <iostream>
#include <string>

int main() {
    std::string                        output = "";
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

    output += "___\n";

    stack_vector::stack_vector<int, 10> append_test;
    append_test.append(test.begin(), test.end());
    append_test.append(test_1.begin(), test_1.end());
    size_t m = 0;
    for (size_t i = 0; i < test.size(); i++) {
        assert(append_test[m] == test[i] && "iterator value != ptr value");
        m++;
    }
    for (size_t i = 0; i < test_1.size(); i++) {
        assert(append_test[m] == test_1[i] && "iterator value != ptr value");
        m++;
    }

    for (const auto &int_val : append_test) {
        output += std::to_string(int_val) + "\n";
    }
    output += "___\n";
    stack_vector::stack_vector<int, 10> erase_test = append_test;
    erase_test.erase(erase_test.begin() + 2, erase_test.begin() + 4);
    assert(erase_test.size() == append_test.size() - 2 && "erase range test failed!");
    erase_test.erase(erase_test.begin() + 2);
    assert(erase_test.size() == append_test.size() - 3 && "erase test failed!");

    for (const auto &int_val : erase_test) {
        output += std::to_string(int_val) + "\n";
    }

    output += "___\n";
    size_t removed_count = std::erase(erase_test, 1);
    assert(removed_count == 1 && "std::erase test failed!");
    assert(erase_test.size() == append_test.size() - 4 && "std::erase test failed!");

    for (const auto &int_val : erase_test) {
        output += std::to_string(int_val) + "\n";
    }

    output += "___\n";

    auto append_test_1 = ::stack_vector::append(append_test, erase_test);
    assert(append_test_1.size() == (append_test.size() + erase_test.size()) &&
           "top level append size check failed!");
    assert(append_test_1.capacity() == (append_test.capacity() + erase_test.capacity()) &&
           "top level append capacity check failed!");

    m = 0;
    for (size_t i = 0; i < append_test.size(); i++) {
        assert(append_test_1[m] == append_test[i] && "iterator value != ptr value");
        m++;
    }
    for (size_t i = 0; i < erase_test.size(); i++) {
        assert(append_test_1[m] == erase_test[i] && "iterator value != ptr value");
        m++;
    }

    for (const auto &int_val : append_test_1) {
        output += std::to_string(int_val) + "\n";
    }

    output += "___\n";

    constexpr stack_vector::stack_vector<int, 5> constexpr_test = {0, 1, 2};
    for (const auto &int_val : constexpr_test) {
        output += std::to_string(int_val) + "\n";
    }
    constexpr auto  constexpr_insert_test_func = [](const stack_vector::stack_vector<int, 5> insert_me) {
        stack_vector::stack_vector<int, 2>  blank{};
        blank.insert(blank.begin(),2ULL,1);
        blank.resize(0);
        blank.resize(1);
        blank.resize(2,2);
        stack_vector::stack_vector<int, 10> constexpr_insert_test = {1};
        stack_vector::stack_vector<int, 5>  assign_test           = {2};
        stack_vector::stack_vector<int, 10> swap_test = {3, 4};
        constexpr_insert_test                                     = assign_test;
        ::std::swap(constexpr_insert_test, swap_test);

        constexpr_insert_test.insert(constexpr_insert_test.begin()+1, insert_me.begin(), insert_me.end());
        constexpr_insert_test.insert(constexpr_insert_test.begin()+1, 5ULL, 5);
        constexpr_insert_test.erase(constexpr_insert_test.begin()+1);


        return blank;
    }(constexpr_test);

    output += "___\n";
    for (const auto &int_val : constexpr_insert_test_func) {
        output += std::to_string(int_val) + "\n";
    }

    std::cout << output;

    return 0;
}

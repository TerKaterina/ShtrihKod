#include <iostream>
#include <sstream>
#include "Tests.h"
#include "Sum.h"

void Tests_headers::Sum_header() {
    std::cout << "Test Sum..." << std::endl;
    
    // Хз че это, но так надо
    std::streambuf* original_cin = std::cin.rdbuf();
    std::streambuf* original_cout = std::cout.rdbuf();
    
    // Тест 1: 10 + 20 = 30
    {
        std::stringstream input("10 20\nq\n");
        std::stringstream output;
        
        std::cin.rdbuf(input.rdbuf());
        std::cout.rdbuf(output.rdbuf());
        
        Sum test;
        test.input();
        test.sum();
        test.output();
        
        if (output.str().find("30") != std::string::npos) {
            std::cout.rdbuf(original_cout);
            std::cout << "Test 1 completed: 10 + 20 = 30" << std::endl;
            std::cout.rdbuf(output.rdbuf());
        } else {
            std::cout.rdbuf(original_cout);
            std::cout << "Test 1 failed" << std::endl;
        }
    }
    
    // Тест 2: -5 + 3 = -2
    {
        std::stringstream input("-5 3\nq\n");
        std::stringstream output;
        
        std::cin.rdbuf(input.rdbuf());
        std::cout.rdbuf(output.rdbuf());
        
        Sum test;
        test.input();
        test.sum();
        test.output();
        
        if (output.str().find("-2") != std::string::npos) {
            std::cout.rdbuf(original_cout);
            std::cout << "Test 2 completed: -5 + 3 = -2" << std::endl;
            std::cout.rdbuf(output.rdbuf());
        } else {
            std::cout.rdbuf(original_cout);
            std::cout << "Test 2 failed" << std::endl;
        }
    }
    
    // Тест 3: 0 + 0 = 0
    {
        std::stringstream input("0 0\nq\n");
        std::stringstream output;
        
        std::cin.rdbuf(input.rdbuf());
        std::cout.rdbuf(output.rdbuf());
        
        Sum test;
        test.input();
        test.sum();
        test.output();
        
        if (output.str().find("0") != std::string::npos) {
            std::cout.rdbuf(original_cout);
            std::cout << "Test 3 completed: 0 + 0 = 0" << std::endl;
            std::cout.rdbuf(output.rdbuf());
        } else {
            std::cout.rdbuf(original_cout);
            std::cout << "Test 3 failed" << std::endl;
        }
    }
    
    // Хз че это, но так надо
    std::cin.rdbuf(original_cin);
    std::cout.rdbuf(original_cout);
    
    std::cout << std::endl << "Test is finished" << std::endl;
}
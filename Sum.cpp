#include <iostream>
#include "Sum.h"
	
void Sum::print_result(std::string phrase, int a, int b) {
	std::cout << phrase << std::endl;
	std::cout << "Sum numbers:"<< std::endl << a + b << std::endl;
}
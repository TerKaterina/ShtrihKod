#include <iostream>
#include "Sum.h"

void Sum::input() {
	std::cout << "Enter 2 numbers:" << std::endl;
	std::cin >> a >> b;
}
	
void Sum::sum() {
	c = a + b;
}
	
void Sum::output() {
	std::cout << "Sum numbers:"<< std::endl << c << std::endl;
	std::cout << "Enter something:" << std::endl;
	std::cin >> ch;
}
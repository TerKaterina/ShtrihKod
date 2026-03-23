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
	std::cout << "Sum of numbers: "<< std::endl << c << std::endl;
	std::cout << "Press any button to close: " << std::endl;
	std::cin >> ch;
}
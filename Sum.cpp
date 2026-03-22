#include <iostream>
#include "2.h"

void Sum::input() {
	std::cout << "Введите 2 числа:\n";
	std::cin >> a >> b;
}
	
void Sum::sum() {
	c = a + b;
}
	
void Sum::output() {
	std::cout << "Сумма чисел:\n" << c;
	std::cout << "\nВведите что-нибудь, чтобы закрыть: ";
	std::cin >> ch;
}
#include "2.h"

int main() {
	setlocale(LC_ALL, "RU");
	Sum calc;
	calc.input();
	calc.sum();
	calc.output();

	return 0;
}
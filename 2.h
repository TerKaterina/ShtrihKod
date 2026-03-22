#ifndef H_2
#define H_2

#include <iostream>

class Sum {
private:
	int a;
	int b;
	int c;

public:
	void input() {
		std::cout << "Enter two numbers:\n";
		std::cin >> a >> b;
	}
	void sum() {
		c = a + b;
	}
	void output() {
		std::cout << "Sum of numbers:\n" << c;
	}
};

#endif // H_2

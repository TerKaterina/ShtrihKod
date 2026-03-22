#include <iostream>
#include <clocale>
#include "Sum.h"
#include "Tests.h"

#ifdef _WIN32
    #include <windows.h>
#endif

void setRussianLocale() {
    #ifdef _WIN32
        // Для Windows
        SetConsoleCP(1251);
        SetConsoleOutputCP(1251);
        setlocale(LC_ALL, "Russian");
    #else
        // Для Linux/Mac
        setlocale(LC_ALL, "ru_RU.UTF-8");
    #endif
}

int main() {
    setRussianLocale();
    
    Tests_headers Test_Sum_Header;
	Test_Sum_Header.Sum_header();
    
	char a;
	std::cin >> a;
    return 0;
}
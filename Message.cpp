#include "Message.h"

void Message::input() {
    std::cout << "Enter your message: " << std::endl;
    std::getline(std::cin, text);
}

void Message::show() {
    std::cout << "Your message: " << std::endl;
    std::cout << text << std::endl;
}
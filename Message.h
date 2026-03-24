#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <string>

class Message {
private:
    std::string text;

public:
    void input();
    void show();
};

#endif // MESSAGE_H
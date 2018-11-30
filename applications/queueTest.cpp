//
// Created by anne on 30.11.18.
//

#include "../include/CircularQueue.h"

int main()
{
    CircularQueue<int> q(5);
    q.push(1, "str1");
    q.push(2, "str2");
    q.push(3, "str3");
    q.push(4, "str4");
    q.push(5, "str5");
    std::cout << q << std::endl;
    q.push(6, "str6");
    q.push(7, "str7");
    std::cout << q << std::endl;

    q.remove();
    std::cout << q << std::endl;
    q.remove();
    std::cout << q << std::endl;
    q.remove();
    std::cout << q << std::endl;
    q.remove();
    std::cout << q << std::endl;
    q.remove();
    std::cout << q << std::endl;


    return 0;
}
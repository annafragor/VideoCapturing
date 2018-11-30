//
// Created by anne on 30.11.18.
//

#ifndef TEST_CIRCULARQUEUE_H
#define TEST_CIRCULARQUEUE_H

#include <iostream>
#include <memory>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

template <typename T>
class CircularQueue
{
public:
    struct Node
    {
        std::shared_ptr<Node> prev;
        std::shared_ptr<Node> next;
        T frame;
        std::string name;

        Node(T frame_, std::string name_, std::shared_ptr<Node> prev_) :
                frame(frame_), name(name_), next(nullptr), prev(prev_) {}

        ~Node()
        {
            prev = nullptr;
            next = nullptr;
        }
    };

    CircularQueue(size_t capacity_) : head(nullptr), tail(nullptr), _size(0), _capacity(capacity_) {}

    auto size() const noexcept -> size_t;
    auto capacity() const noexcept -> size_t;
    auto empty() const noexcept -> bool;
    auto full() const noexcept -> bool;
    auto push(const T& frame, std::string name) noexcept -> bool;
    auto remove() noexcept -> bool;
    auto save(const std::string path) noexcept -> bool;

    friend auto operator << (std::ostream& out, const CircularQueue<T>& queue) -> std::ostream&
    {
        if (!queue.head)
            out << "empty\n";
        std::shared_ptr<Node> thisNode = queue.head;
        for(int i = 0; i < queue.size(); ++i)
        {
            out << thisNode->name << std::endl;
            thisNode = thisNode->next;
        }
        return  out;
    }

    ~CircularQueue();

    size_t _size;
    size_t _capacity;
    std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;
};

template <typename T>
auto CircularQueue<T>::size() const noexcept -> size_t { return _size; }


template <typename T>
auto CircularQueue<T>::capacity() const noexcept -> size_t { return _capacity; }


template <typename T>
auto CircularQueue<T>::empty() const noexcept -> bool
{
    if (size())
        return false;
    else
        return true;
}


template <typename T>
auto CircularQueue<T>::full() const noexcept -> bool
{
    if (size() == capacity())
        return true;
    else
        return false;
}


template <typename T>
auto CircularQueue<T>::push(const T &frame, std::string name) noexcept -> bool
{
    if (full())
    {
        head = head->next;
        tail->next = std::make_shared<Node>(frame, name, tail);
        tail->next->next = head;
        tail = tail->next;
        head->prev = nullptr;
    }
    else if (size() == capacity() - 1)
    {
        tail->next = std::make_shared<Node>(frame, name, tail);
        tail->next->next = head;
        tail = tail->next;
        head->prev = tail;
        _size++;
    }
    else if (!empty())
    {
        tail->next = std::make_shared<Node>(frame, name, tail);
        tail = tail->next;
        _size++;
    }
    else if (empty())
    {
        head = std::make_shared<Node>(frame, name, nullptr);
        tail = head;
        _size++;
    }
    return true;
}


template <typename T>
auto CircularQueue<T>::remove() noexcept -> bool
{
    if (empty())
        return false;

    if (tail == head)
    {
        head = nullptr;
        tail = nullptr;
    }
    else
    {
        if (tail->next == head)
            tail->next = nullptr;
        head = head->next;
        head->prev = nullptr;
    }

    _size -= 1;
    return true;
}

template <typename T>
auto CircularQueue<T>::save(const std::string path) noexcept -> bool
{
    if (empty())
        return false;

    for (int i = 0; i < size(); ++i)
    {
        if (empty())
            break;

        imwrite(path + head->name, head->frame);
        remove();
    }

    return true;
}


template <typename T>
CircularQueue<T>::~CircularQueue()
{
    head = nullptr;
    tail = nullptr;
    _size = 0;
    _capacity = 0;
}

#endif //TEST_CIRCULARQUEUE_H

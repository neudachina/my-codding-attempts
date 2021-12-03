#pragma once

#include <atomic>
#include <optional>
#include <stdexcept>
#include <utility>

template <class T>
class MPSCStack {
public:
    // Push adds one element to stack top.
    //
    // Safe to call from multiple threads.
    void Push(const T& value) {
        auto node = new Node;
        node->value = value;
        while (true) {
            node->next = head_.load();
            auto tmp = node->next;
            if (head_.compare_exchange_weak(tmp, node)) {
                return;
            }
        }
    }

    // Pop removes top element from the stack.
    //
    // Not safe to call concurrently.
    std::optional<T> Pop() {
        if (head_.load() == nullptr) {
            return std::nullopt;
        } else {
            Node* current_head;
            while (true) {
                current_head = head_.load();
                if (head_.compare_exchange_weak(current_head, current_head->next)) {
                    break;
                }
            }
            Node* node = current_head;
            T res = node->value;
            delete node;
            return res;
        }
    }

    // DequeuedAll Pop's all elements from the stack and calls cb() for each.
    //
    // Not safe to call concurrently with Pop()
    template <class TFn>
    void DequeueAll(const TFn& cb) {
        while (head_) {
            cb(Pop().value());
        }
    }

    ~MPSCStack() {
        while (head_) {
            Pop();
        }
    }

private:
    struct Node {
        T value = T();
        Node* next = nullptr;
    };

    std::atomic<Node*> head_ = nullptr;
};

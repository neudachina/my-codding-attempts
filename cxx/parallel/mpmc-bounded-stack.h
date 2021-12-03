#pragma once

#include <mutex>
#include <queue>
#include <thread>

template <class T>
class MPMCBoundedQueue {
public:
    explicit MPMCBoundedQueue(int max_size) : data_(max_size), size_(max_size), head_(0), tail_(0) {
        for (uint64_t i = 0; i < size_; ++i) {
            data_[i].tag = i;
        }
    }

    bool Enqueue(const T& value) {
        uint64_t idx;
        while (true) {
            uint64_t current_head = head_.load();
            idx = current_head & (size_ - 1);
            if (data_[idx].tag != current_head) {
                if (current_head == tail_.load() + size_) {
                    return false;
                } else {
                    std::this_thread::yield();
                }
            } else {
                if (head_.compare_exchange_weak(current_head, current_head + 1)) {
                    break;
                } else {
                    std::this_thread::yield();
                    continue;
                }
            }
        }
        data_[idx].value = value;
        ++data_[idx].tag;
        return true;
    }

    bool Dequeue(T& data) {
        uint64_t idx;
        while (true) {
            uint64_t current_tail = tail_.load();
            idx = current_tail & (size_ - 1);
            if (data_[idx].tag != current_tail + 1) {
                if (current_tail == head_.load()) {
                    return false;
                } else {
                    std::this_thread::yield();
                }
            } else {
                if (tail_.compare_exchange_weak(current_tail, current_tail + 1)) {
                    break;
                } else {
                    std::this_thread::yield();
                    continue;
                }
            }
        }

        data = data_[idx].value;
        data_[idx].tag += size_ - 1;
        return true;
    }

private:
    struct Node {
        T value = T();
        std::atomic<uint64_t> tag = 0;
    };

    std::vector<Node> data_;
    uint64_t size_;
    std::atomic<uint64_t> head_, tail_;
};

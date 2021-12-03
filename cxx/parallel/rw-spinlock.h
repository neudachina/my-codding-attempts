#pragma once

#include <atomic>
#include <thread>

struct RWSpinLock {
    void LockRead() {
        while (true) {
            uint64_t expected = data_.load();
            if (expected & 1) {
                std::this_thread::yield();
                continue;
            }
            if (data_.compare_exchange_weak(expected, expected + 2)) {
                break;
            }
        }
    }

    void UnlockRead() {
        data_.fetch_sub(2);
    }

    void LockWrite() {
        while (true) {
            uint64_t expected = data_.load();
            if (expected & 1) {
                std::this_thread::yield();
                continue;
            }

            if (data_.compare_exchange_weak(expected, expected + 1)) {
                break;
            }
        }

        while (data_ != 1) {
            std::this_thread::yield();
        }
    }

    void UnlockWrite() {
        data_.store(0);
    }

    std::atomic<uint64_t> data_ = 0;
};

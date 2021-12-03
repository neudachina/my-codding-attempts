#pragma once

#include <utility>
#include <optional>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : max_size_(size), open_(true) {
    }

    void Send(const T& value) {
        if (!open_) {
            throw std::runtime_error("https://www.youtube.com/watch?v=0VTIeL9wmQE");
        }

        std::unique_lock<std::mutex> ul(m_);
        if (queue_.size() == max_size_) {
            cv_send_.wait(ul, [this]() -> bool { return queue_.size() < max_size_ || !open_; });
        }

        if (!open_) {
            throw std::runtime_error("https://www.youtube.com/watch?v=0VTIeL9wmQE");
        }

        queue_.push_back(value);
        cv_recv_.notify_one();
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> ul(m_);

        if (queue_.empty() && !open_) {
            return std::nullopt;
        }

        if (queue_.empty()) {
            cv_recv_.wait(ul, [this]() -> bool { return !queue_.empty() || !open_; });
        }

        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = queue_.front();
        queue_.pop_front();
        cv_send_.notify_one();
        return value;
    }

    void Close() {
        std::unique_lock<std::mutex> ul(m_);
        open_ = false;
        cv_recv_.notify_all();
        cv_send_.notify_all();
    }

private:
    std::deque<T> queue_;
    std::atomic<int> max_size_;
    std::mutex m_;
    std::atomic<bool> open_;

    std::condition_variable cv_send_;
    std::condition_variable cv_recv_;
};

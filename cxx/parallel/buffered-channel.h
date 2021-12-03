#pragma once

#include <utility>
#include <optional>
#include <condition_variable>
#include <mutex>

template <class T>
class UnbufferedChannel {
public:
    void Send(const T& value) {
        std::unique_lock<std::mutex> lock_writer(writer_);
        std::unique_lock<std::mutex> lock(mutex_);

        message_ = &value;

        cv_reader_.notify_one();

        if (message_) {
            cv_writer_.wait(lock, [this]() -> bool { return !message_ || !open_; });
        }

        if (!open_ && message_) {
            throw std::runtime_error("channel closed");
        }
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> lock_reader(reader_);
        std::unique_lock<std::mutex> lock(mutex_);

        if (!message_ && open_) {
            cv_reader_.wait(lock, [this]() -> bool { return !open_ || message_; });
        }

        if (!open_) {
            return std::nullopt;
        }

        T value = *message_;
        message_ = nullptr;
        cv_writer_.notify_one();
        return value;
    }

    void Close() {
        std::unique_lock<std::mutex> lock(mutex_);
        open_ = false;
        cv_writer_.notify_one();
        cv_reader_.notify_one();
    }

private:
    bool open_ = true;
    std::condition_variable cv_writer_, cv_reader_;
    std::mutex mutex_, writer_, reader_;
    T const* message_ = nullptr;
};

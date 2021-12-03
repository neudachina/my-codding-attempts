#include <iostream>
#include <memory>
#include <list>

template <typename T>
class List {
public:
    List() : head_(new Node), tail_(head_.get()), size_(0) {
        head_->next.reset();
        head_->prev = nullptr;
    }

    ~List() {
    }

    void push_back(const T& x) {
        Node* node = new Node;
        node->value = std::make_unique<T>(x);

        node->prev = tail_->prev;
        tail_->prev = node;

        std::unique_ptr<Node>& prev = node->prev ? node->prev->next : head_;

        node->next.reset(prev.release());
        prev.reset(node);
        ++size_;
    }

    void push_front(const T& x) {
        Node* node = new Node;
        node->value = std::make_unique<T>(x);
        node->prev = nullptr;
        head_->prev = node;
        node->next.reset(head_.release());
        head_.reset(node);
        ++size_;
    }

    void pop_back() {
        std::unique_ptr<Node>& prev = tail_->prev->prev ? tail_->prev->prev->next : head_;
        std::unique_ptr<Node> node(prev.release());
        prev.reset(node->next.release());
        tail_->prev = node->prev;
        --size_;
    }

    void pop_front() {
        head_.reset(head_->next.release());
        head_->prev = nullptr;
        --size_;
    }


    T& front() {
        return *head_->value;
    }

    T& back() {
        return *tail_->prev->value;
    }

    auto begin() {
        return Iterator(head_);
    }

    auto end() {
        return Iterator(tail_);
    }

    List& operator=(List& l) {
        while (size_ > 0)
            pop_back();
        for (auto it = l.begin(); it != l.end(); ++it)
            push_back(*it);
        return *this;
    }

    size_t size() {
        return size_;
    }

    struct Node {
        std::unique_ptr<Node> next;
        Node* prev;
        std::unique_ptr<T> value;
    };

private:
    std::unique_ptr<Node> head_;
    Node* tail_;
    size_t size_;

    class Iterator {
    public:
        Iterator() : node_(nullptr) {
        }

        Iterator(Node*& node) : node_(node) {
        }

        Iterator(std::unique_ptr<Node>& node) : node_(node.get()) {
        }

        T& operator*() {
            return *node_->value;
        }

        Iterator& operator++() {
            node_ = node_->next.get();
            return *this;
        }

        Iterator& operator--() {
            node_ = node_->prev;
            return *this;
        }

        bool operator==(Iterator other) const {
            return (node_ == other.node_);
        }

        bool operator!=(Iterator other) const {
            return (node_ != other.node_);
        }

    private:
        Node* node_;
    };
};

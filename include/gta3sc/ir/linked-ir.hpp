#pragma once
#include <gta3sc/arena-allocator.hpp>

namespace gta3sc
{
template<typename T>
struct LinkedIR
{
public:
    explicit LinkedIR() = default;

    static auto from_ir(arena_ptr<T> ir) -> LinkedIR
    {
        LinkedIR linked;
        linked.setup_first(ir);
        return linked;
    }

    LinkedIR(const LinkedIR&) = delete;

    LinkedIR(LinkedIR&& other) :
        front_(other.front_), back_(other.back_)
    {
        other.front_ = other.back_ = nullptr;
    }

    LinkedIR& operator=(const LinkedIR&) = delete;

    LinkedIR& operator=(LinkedIR&& other)
    {
        LinkedIR(std::move(other)).swap(*this);
        return *this;
    }

    void swap(LinkedIR& other)
    {
        std::swap(this->front_, other.front_);
        std::swap(this->back_, other.back_);
    }

    bool empty() const
    {
        return front_ == nullptr;
    }

    auto front() const -> arena_ptr<T>
    {
        assert(front_ != nullptr);
        return front_;
    }

    auto back() const -> arena_ptr<T>
    {
        assert(back_ != nullptr);
        return back_;
    }

    // O(1)
    void push_front(arena_ptr<T> ir)
    {
        assert(ir && !ir->prev && !ir->next);

        if(empty())
            return setup_first(ir);

        ir->set_next(this->front_);
        this->front_ = ir;
    }

    // O(1)
    void push_back(arena_ptr<T> ir)
    {
        assert(ir && !ir->prev && !ir->next);

        if(empty())
            return setup_first(ir);

        this->back_->set_next(ir);
        this->back_ = ir;
    }

    // O(1)
    void splice_back(LinkedIR&& other)
    {
        if(this->empty())
            return other.swap(*this);

        if(other.empty())
            return;

        this->back_->set_next(other.front_);
        this->back_ = other.back_;

        other.front_ = other.back_ = nullptr;
    }

private:
    static auto find_front(arena_ptr<T> ir) -> arena_ptr<T>
    {
        auto first = nullptr;
        while(ir)
        {
            first = ir;
            ir = ir->prev;
        }
        return first;
    }

    static auto find_back(arena_ptr<T> ir) -> arena_ptr<T>
    {
        auto last = nullptr;
        while(ir)
        {
            last = ir;
            ir = ir->next;
        }
        return last;
    }

    void setup_first(arena_ptr<T> ir)
    {
        assert(empty());

        this->front_ = ir;
        while(ir != nullptr)
        {
            this->back_ = ir;
            ir = ir->next;
        }
    }

private:
    arena_ptr<T> front_ = nullptr;
    arena_ptr<T> back_ = nullptr;
};
}

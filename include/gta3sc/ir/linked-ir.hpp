#pragma once
#include <gta3sc/arena-allocator.hpp>

// TODO bad things will happen if one manually adds things to a IR while it is
// inside a LinkedIR, document this
//
// e.g. LinkedIR has [A, B, C]
// then one manually changes B => A to be B => D => C
// then LinkedIR.size() is wrong

namespace gta3sc
{
template<typename T>
struct LinkedIR
{
public:
    // TODO this is all WIP

    // TODO this could be generalized to any T
    struct iterator
    {
        public:
            iterator& operator++()
            {
                assert(curr != nullptr);
                this->curr = curr->next;
                return *this;
            }

            T& operator*() { return *curr; }
            T* operator->() { return curr; }

            bool operator==(const iterator& rhs) const
            { return curr == rhs.curr; }

        protected:
            friend class LinkedIR<T>;

            explicit iterator() = default;
            explicit iterator(T* p) : curr(p)
            {}

        private:
            T* curr = nullptr;
    };

    auto begin() { return iterator(front_); }
    auto end() { return iterator(); }

    explicit LinkedIR() = default;

    static auto from_ir(arena_ptr<T> ir) -> LinkedIR
    {
        LinkedIR linked;
        linked.setup_first(ir);
        return linked;
    }

    auto into_ir() && -> arena_ptr<T>
    {
        auto result = this->front_;
        LinkedIR().swap(*this);
        return result;
    }

    LinkedIR(const LinkedIR&) = delete;

    LinkedIR(LinkedIR&& other) :
        front_(other.front_), back_(other.back_), size_(other.size_)
    {
        LinkedIR().swap(other);
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
        std::swap(this->size_, other.size_);
    }

    bool empty() const
    {
        return size_ == 0;
    }

    size_t size() const
    {
        return size_;
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
        this->size_ += 1;
    }

    // O(1)
    void push_back(arena_ptr<T> ir)
    {
        assert(ir && !ir->prev && !ir->next);

        if(empty())
            return setup_first(ir);

        this->back_->set_next(ir);
        this->back_ = ir;
        this->size_ += 1;
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
        this->size_ += other.size_;

        LinkedIR().swap(other);
    }

    // O(1)
    void splice_front(LinkedIR&& other)
    {
        other.splice_back(std::move(*this));
        *this = std::move(other);
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
        this->size_ = 0;

        while(ir != nullptr)
        {
            ++this->size_;
            this->back_ = ir;
            ir = ir->next;
        }
    }

private:
    arena_ptr<T> front_ = nullptr;
    arena_ptr<T> back_ = nullptr;
    size_t size_ = 0;
};
}

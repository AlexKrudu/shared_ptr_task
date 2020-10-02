#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <cstddef>
#include <type_traits>

struct control_block {
private:
    size_t ref_cnt = 1;
    size_t weak_cnt = 0;


public:
    virtual ~control_block() = default;

    virtual void delete_object() = 0;

    control_block() = default;

    void release_ref() {
        ref_cnt--;
        if (ref_cnt == 0) {
            delete_object();
        }
    };

    void release_weak() {
        weak_cnt--;
    };

    void add_ref() {
        ref_cnt++;
    };

    void add_weak() {
        weak_cnt++;
    };

    [[nodiscard]] size_t use_count() const noexcept {
        return ref_cnt;
    };

    [[nodiscard]] size_t weak_count() const noexcept {
        return weak_cnt;
    }
};


template<typename Y, typename D = std::default_delete<Y>>
struct cb_ptr : control_block, D {
private:
    Y *ptr;
public:
    ~cb_ptr() override = default;

    cb_ptr(Y *ptr, D deleter = std::default_delete<Y>()) : ptr(ptr), D(std::move(deleter)) {};

    void delete_object() override {
        static_cast<D &>(*this)(ptr);
    };
};

template<typename Y, typename D = std::default_delete<Y>>
struct cb_obj : control_block, D {
private:
    typename std::aligned_storage<sizeof(Y), alignof(Y)>::type data;
public:
    Y *get() noexcept {
        return reinterpret_cast<Y *>(&data);
    }

    template<typename ...Args>
    explicit cb_obj(Args &&...args) {
        new(&data) Y(std::forward<Args>(args)...);
    }

    void delete_object() override {
        reinterpret_cast<Y *>(&data)->~Y();
    }
};

template<typename T>
struct shared_ptr;

template<typename T>
struct weak_ptr {
    control_block *cb{};
    T *ptr{};
    void swap(weak_ptr &other) noexcept {
        std::swap(this->cb, other.cb);
        std::swap(this->ptr, other.ptr);
    };

    weak_ptr() : ptr(nullptr), cb(nullptr) {};

    ~weak_ptr() {
        if (cb != nullptr) {
            cb->release_weak();
            if (cb->use_count() == 0 && cb->weak_count() == 0) {
                delete cb;
            }
        }
    };


    template<typename Y>
    weak_ptr(shared_ptr<Y> const &r) {
        this->ptr = r.ptr;
        this->cb = r.cb;
        if (cb != nullptr) {
            cb->add_weak();
        }
    };

    shared_ptr<T> lock() {
        if (this->cb != nullptr) {
            if (this->cb->use_count() != 0) {
                return shared_ptr<T>(*this);
            }
        }
        return shared_ptr<T>();
    };


    weak_ptr(weak_ptr &&other) {
        this->ptr = other.ptr;
        this->cb = other.cb;
        other.ptr = nullptr;
        other.cb = nullptr;
    };

    weak_ptr(weak_ptr const &r) {
        this->ptr = r.ptr;
        this->cb = r.cb;
        if (cb != nullptr) {
            cb->add_weak();
        }
    };

    weak_ptr &operator=(const weak_ptr &other) {
        weak_ptr(other).swap(*this);
        return *this;
    };

    weak_ptr &operator=(weak_ptr &&other) {
        weak_ptr(std::move(other)).swap(*this);
        return *this;
    };


};

template<typename T>
struct shared_ptr {
    control_block *cb{};
    T *ptr{};

    shared_ptr() noexcept: ptr(nullptr), cb(nullptr) {};

    ~shared_ptr() {
        if (cb != nullptr) {
            cb->release_ref();
            if (cb->use_count() == 0 && cb->weak_count() == 0) {
                delete cb;
            }
        }
    }

    explicit shared_ptr(std::nullptr_t) noexcept: ptr(nullptr), cb(nullptr) {};

    template<typename Y, typename D = std::default_delete<Y>>
    explicit shared_ptr(Y *p, D deleter = D{}) {
        try {
            this->cb = new cb_ptr(p, deleter);
            this->ptr = p;
        } catch (...) {
            deleter(p);
            throw;
        }
    }

    T *get() const noexcept {
        return ptr;
    }

    void reset() noexcept {
        shared_ptr().swap(*this);
    }

    template<typename Y, typename D = std::default_delete<Y>>
    void reset(Y *point, D deleter = D{}) {
        shared_ptr(point, deleter).swap(*this);
    }

    T &operator*() const noexcept {
        return *ptr;
    }

    shared_ptr &operator=(const shared_ptr &other) noexcept {
        shared_ptr(other).swap(*this);
        return *this;
    }

    shared_ptr &operator=(shared_ptr &&other) noexcept {
        shared_ptr(std::move(other)).swap(*this);
        return *this;
    }

    T *operator->() const noexcept {
        return ptr;
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    [[nodiscard]] size_t use_count() const noexcept {
        if (cb != nullptr) {
            return cb->use_count();
        } else {
            return 0;
        }

    }

    template<typename D>
    shared_ptr(std::nullptr_t, D deleter) noexcept {};

    template<typename Y>
    shared_ptr(shared_ptr<Y> const &sp, T *ptr) noexcept {
        cb = sp.cb;
        ptr = ptr;
        if (cb != nullptr) {
            cb->add_ref();
        }
    };

    template<typename Y>
    shared_ptr(const shared_ptr<Y> &r) noexcept {
        this->ptr = r.ptr;
        this->cb = r.cb;
        if (cb != nullptr) {
            cb->add_ref();
        }
    }

    shared_ptr(const shared_ptr &r) noexcept {
        this->ptr = r.ptr;
        this->cb = r.cb;
        if (cb != nullptr) {
            cb->add_ref();
        }
    }

    template<typename Y>
    shared_ptr(const weak_ptr<Y> &r) noexcept {
        this->ptr = r.ptr;
        this->cb = r.cb;
        if (cb != nullptr) {
            cb->add_ref();
        }
    }


    template<typename Y>
    shared_ptr(shared_ptr<Y> &&r) noexcept {
        this->ptr = r.ptr;
        this->cb = r.cb;
        r.ptr = nullptr;
        r.cb = nullptr;
    }

    shared_ptr(shared_ptr &&r) noexcept {
        this->ptr = r.ptr;
        this->cb = r.cb;
        r.ptr = nullptr;
        r.cb = nullptr;
    }

    void swap(shared_ptr &other) noexcept {
        std::swap(this->cb, other.cb);
        std::swap(this->ptr, other.ptr);
    };


    template<typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args &&... args);
};


template<typename A, typename B>
bool operator==(const shared_ptr<A> &first, const shared_ptr<B> &second) {
    return first.get() == second.get();
}

template<typename B>
bool operator==(std::nullptr_t, const shared_ptr<B> &second) {
    return nullptr == second.get();
}

template<typename A>
bool operator==(const shared_ptr<A> &first, std::nullptr_t) {
    return first.get() == nullptr;
}

template<typename A, typename B>
bool operator!=(const shared_ptr<A> &first, const shared_ptr<B> &second) {
    return first.get() != second.get();
}

template<typename B>
bool operator!=(std::nullptr_t, const shared_ptr<B> &second) {
    return nullptr != second.get();
}

template<typename A>
bool operator!=(const shared_ptr<A> &first, std::nullptr_t) {
    return first.get() != nullptr;
}


template<typename U, typename... Args>
shared_ptr<U> make_shared(Args &&... args) {
    auto tmp = new cb_obj<U>(std::forward<U>(args)...);
    shared_ptr<U> sp;
    sp.cb = tmp;
    sp.ptr = tmp->get();
    return sp;
};

#endif

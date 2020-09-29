#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <cstddef>
#include <type_traits>

struct control_block {
    size_t ref_cnt;
    size_t weak_cnt;

    control_block();

    void release_ref();
    void release_weak();

    void add_ref();
    void add_weak();

    size_t use_count();

public:
    virtual ~control_block() = default;
    virtual void delete_object() = 0;
};

// type-erasure
// std::any, std::function

// erase Y, D
// -> empty base optimization

template <typename Y, typename D>
struct cb_ptr : control_block, D {
    Y* ptr;

    cb_ptr(Y* ptr, D deleter);

    void delete_object() override;
};

template <typename Y, typename D>
struct cb_obj : control_block, D {
    // std::aligned_storage
};

template <typename T>
struct shared_ptr {
    control_block* cb;
    T* ptr;

    // пустой shared_pointer :: cb == nullptr
    // нулевой shared_pointer :: ptr == nullptr

    // пустой и нулевой объект
    shared_ptr();

    // тоже пустой нулевой
    shared_ptr(std::nullptr_t);

    // shared_ptr(T*); -> T = Base => ~Base()
    // непустой объект
    template <typename Y>
    shared_ptr(Y* ptr)
            : shared_ptr(ptr, std::default_delete<Y>()) {}

    // Base, Derived : Base.
    // shared_ptr<Base>(new Derived()) -> Y = Derived

    template <typename Y, typename D>
    shared_ptr(Y* ptr, D deleter)
            : ptr(ptr), cb(new cb_ptr<Y, D>(ptr, deleter)) {}

    // aliasing constructor
    template <typename Y>
    shared_ptr(shared_ptr<Y> const& sp, T* ptr);

    template <typename Y, typename... Args>
    shared_ptr<Y> make_shared(Args&&... args);
};

// f(sp<int>(new int(1)), sp<int>(new int(2)))
// ==
// f(ms<int>(1), ms<int>(1))
//

// 1) create new cb
// 2) cb->ptr = new T(args)

// new cb(args...)

// a <- 10 sp
// a <- 10 wp

// 10 sp умирают => объект разрушен
//
// 1) 2 вида cb T, T*
// 2) Y -- тип объемлющего объекта,

// sp = shp<int>(new int(1))
// sp = shp<int>(new int(2)) 1 <-> sp.reset(new int(2)) 0.5
//

// cppreference
// constructors: except 6, 7, 12, 13
// operator=: except 3, 4
// reset: except 4
// Observers: не надо operator[], owner_before
// non-member только make_shared, ==, !=, swap

// weak_ptr
// default construcor
// weak_ptr(shared_ptr<T> const&)
// weak_ptr(weak_ptr&&)
// weak_ptr(weak_ptr const&)
// operator= (const&, &&)
// shared_ptr<T> lock();
// 1, 1

#endif // SHARED_PTR_H
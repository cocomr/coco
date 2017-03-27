/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#pragma once

#include <map>
#include <list>
#include <vector>
#include <memory>
#include <algorithm>
#include "coco/util/threading.h"
#include "coco/util/logging.h"
 

namespace coco
{
namespace util
{

/**
 * This class provides a pool of memory of objects of vector<T>

 */
template <class T>
class VectorPool
{
public:
    static std::shared_ptr<std::vector<T> > get(unsigned long k, bool power2 = false);
    static VectorPool<T> singleton;

private:
    VectorPool()
    {}

    ~VectorPool()
    {
        std::lock_guard<std::mutex> mlock(this->mutex_);
        for (auto & p : free_pool_)
            for (auto & l : p.second)
                delete l;
    }

    /// returns the singleton
    static VectorPool<T> &instance();

    /// implementation of acquisistion: GLOBAL lock
    std::shared_ptr<std::vector<T> > getImpl(unsigned long k, bool power2)
    {
        if(k < 16384)
        {
            return std::make_shared<std::vector<T> >(k);
        }
        std::lock_guard<std::mutex> mlock(this->mutex_);

        std::vector<T> * v_ptr = nullptr;

        auto pi = free_pool_.lower_bound(k);
        if (pi != free_pool_.end())
        {
            v_ptr = pi->second.front();
            v_ptr->resize(k);

            pi->second.pop_front();
            if (pi->second.size() == 0)
            {
                // remove from list
                free_pool_.erase(pi);
            }
        }
        if (!v_ptr)
        {
            v_ptr = new std::vector<T>(k);
        }

        /// create a descrutor that releases the pointer
        return std::shared_ptr<std::vector<T> >(v_ptr, [this] (std::vector<T>* vec)
            {
                std::lock_guard<std::mutex> mlock(this->mutex_);
                this->free_pool_[vec->capacity()].push_back(vec);
            });
    }

    /// free pool for given size
    std::map<unsigned long, std::list<std::vector<T> *> > free_pool_;

    std::mutex mutex_;
};

template<class T>
std::shared_ptr<std::vector<T> > VectorPool<T>::get(unsigned long k, bool power2)
{
    return instance().getImpl(k,power2);
}

template <class T>
VectorPool<T> VectorPool<T>::singleton;

template<class T>
VectorPool<T> & VectorPool<T>::instance()
{
    return singleton;
}

/**
 * Allocator based on the VectorPool that can be used for allocating std::vector
 *
 * In practice this provides the memory for the std::vector on request. Everytime
 * it is resizes the pool gets called
 */
template <class T>
class PoolAllocator
{
public:
    typedef T value_type;
    PoolAllocator(/*ctor args*/)
    {}
    template <class U>
    PoolAllocator(const PoolAllocator<U>& other)
    {}

    T* allocate(std::size_t n)
    {
        ptr_ = VectorPool<T>::get(n);
        return ptr_->data();
    }

    void deallocate(T* p, std::size_t n)
    {
        ptr_.reset();
    }
private:
    std::shared_ptr<std::vector<T> > ptr_;
};

template <class T, class U>
bool operator==(const PoolAllocator<T>& p1, const PoolAllocator<U>& p2)
{
    return typeid(T) == typeid(U);
    //return p1.ptr() == p2.ptr();
}
template <class T, class U>
bool operator!=(const PoolAllocator<T>& p1, const PoolAllocator<U>& p2)
{
    return typeid(T) != typeid(U);
    //return p1.ptr() != p2.ptr();
}

template<class T>
using Vector = typename std::vector<T, PoolAllocator<T> >;
template<class T>
using VectorPtr = typename std::shared_ptr<Vector<T> >;

template<class T>
inline Vector<T> toVector(const std::vector<T> &other)
{
    Vector<T> vec(other.begin(), other.end());
    return std::move(vec);
}
template<class T>
inline Vector<T> toVector(std::vector<T> &&other)
{
    Vector<T> vec(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
    return std::move(vec);
}

template<class T>
inline std::vector<T> toStdVector(const Vector<T> &other)
{
    std::vector<T> vec(other.begin(), other.end());
    return std::move(vec);

}
template<class T>
inline std::vector<T> toStdVector(Vector<T> &&other)
{
    std::vector<T> vec(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
    return std::move(vec);
}


}  // end of namespace util

}  // end of namespace coco

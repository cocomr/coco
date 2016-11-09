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
#include <mutex>
#include <algorithm>

namespace coco
{
namespace util
{

template <class T>
class VectorPool
{
public:
    static std::shared_ptr<std::vector<T> > get(unsigned long k);

private:
    VectorPool()
    {}

    ~VectorPool()
    {
        for (auto & p : free_pool_)
            for (auto & l : p.second)
                delete l;
    }

    static VectorPool<T> &instance();

    std::shared_ptr<std::vector<T> > getImpl(unsigned long k)
    {
        std::unique_lock<std::mutex> mlock(this->mutex_);

        std::vector<T> * v_ptr = nullptr;

        auto pi = free_pool_.lower_bound(k);
        if (pi != free_pool_.end())
        {
            v_ptr = pi->second.front();
            v_ptr->resize(k);

            pi->second.pop_front();
            if (pi->second.size() == 0)
                free_pool_.erase(pi);
        }
        if (!v_ptr)
            v_ptr = new std::vector<T>(k);

        return std::shared_ptr<std::vector<T> >(v_ptr, [this] (std::vector<T>* vec)
            {
                std::unique_lock<std::mutex> mlock(this->mutex_);
                this->free_pool_[vec->capacity()].push_back(vec);
            });
    }

    std::map<unsigned long, std::list<std::vector<T> *> > free_pool_;

    std::mutex mutex_;
};

template<class T>
std::shared_ptr<std::vector<T> > VectorPool<T>::get(unsigned long k)
{
    return instance().getImpl(k);
}

template<class T>
VectorPool<T> & VectorPool<T>::instance()
{
    static VectorPool<T> instance;
    return instance;
}

template<class T>
class Vector
{
public:
    Vector()
    { }
    Vector(unsigned long count)
    {
        buffer_ = coco::util::VectorPool<T>::get(count);
    }
    Vector(unsigned long count, const T& value)
    {
        buffer_ = coco::util::VectorPool<T>::get(count);

        buffer_->assign(count, value);
    }
    Vector(const std::vector<T> &vector)
    {
        buffer_ = coco::util::VectorPool<T>::get(vector.size());
        buffer_->assign(vector.begin(), vector.end());
    }
    Vector(std::vector<T> &&vector)
    {
        buffer_ = coco::util::VectorPool<T>::get(vector.size());
        *buffer_ = std::move(vector);
    }
    Vector(std::initializer_list<T> init)
    {
        buffer_ = coco::util::VectorPool<T>::get(init.size());
        buffer_->assign(init.begin(), init.end());
    }

    T* data()
    {
        return buffer_->data();
    }
    const T* data() const
    {
        return buffer_->data();
    }

    T& operator[](unsigned long idx)
    {
        return (*buffer_)[idx];
    }

    const T& operator[](unsigned long idx) const
    {
        return (*buffer_)[idx];
    }
    // If the buffer doesn't exists undefined behavior
    // Not this because I don't want user to modify the size of the vector.
    // std::vector<T>& get()
    // {
    //      return *buffer_;
    // }
    // If the buffer doesn't exists undefined behavior
    const std::vector<T>& get() const
    {
        return *buffer_;
    }

    /// Returns the number of elements of type T
    unsigned long size() const
    {
        return buffer_ ? buffer_->size() : 0;
    }

    void resize(unsigned long count)
    {
        auto new_buffer = coco::util::VectorPool<T>::get(count);
        std::copy_n(buffer_->begin(), std::min(count, size()), new_buffer->begin());
        buffer_ = new_buffer;
    }
    void resize(unsigned long count, const T& value)
    {
        auto new_buffer = coco::util::VectorPool<T>::get(count);
        auto it = std::copy_n(buffer_->begin(), std::min(count, size()), new_buffer->begin());
        std::for_each(it, new_buffer->end(), [value](T &n){ n = value;});
        buffer_ = new_buffer;
    }

    Vector clone() const
    {
        return Vector(*buffer_);
    }

    operator bool() const
    {
        if (buffer_)
            return buffer_->size() > 0;
        return false;
    }

private:
    std::shared_ptr<std::vector<T> > buffer_;
};


#if 0
template<class T>
class MemoryPool
{
public:
    static std::shared_ptr<T> get();

private:
    MemoryPool()
    {}

    ~MemoryPool()
    {
        for (auto & fp : free_pool_)
            delete fp;
    }

    static MemoryPool<T> &instance();

    std::shared_ptr<T> getImpl()
    {
        std::unique_lock<std::mutex> mlock(this->mutex_);
        T * obj;
        if (free_pool_.empty())
        {
            obj = new T();
        }
        else
        {
            obj = free_pool_.front();
            free_pool_.pop_front();
        }

        return getSharedPtr(obj);
    }

    inline std::shared_ptr<T> getSharedPtr(T * t)
    {
        return std::shared_ptr<T>(t, [this] (T* obj)
            {
                std::unique_lock<std::mutex> mlock(this->mutex_);
                this->free_pool_.push_back(obj);
            });
    }

    std::mutex mutex_;
    std::list<T *> free_pool_;
};

template<class T>
std::shared_ptr<T> MemoryPool<T>::get()
{
    return instance().getImpl();
}

template<class T>
MemoryPool<T> & MemoryPool<T>::instance()
{
    static MemoryPool<T> instance;
    return instance;
}
#endif




}  // end of namespace util

}  // end of namespace coco

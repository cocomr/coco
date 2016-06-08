#pragma once

#include <vector>
#include <memory>

namespace coco
{

namespace util
{

template <class T>
class VectorPool
{
public:
    static std::shared_ptr<std::vector<T> > get(unsigned int k);

private:
    VectorPool()
    {}

    ~VectorPool()
    {
        for(auto & p: free_pool_)
            for (auto & l : p.second)
                delete l;
    }

    static VectorPool<T> &instance();

    std::shared_ptr<std::vector<T> > getImpl(unsigned int k)
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

        return std::shared_ptr<std::vector<T> >(v_ptr, [this] (std::vector<T>* vec) {
            std::unique_lock<std::mutex> mlock(this->mutex_);
            this->free_pool_[vec->capacity()].push_back(vec);
        });
    }

    std::map<unsigned int, std::list<std::vector<T> *> > free_pool_;

    std::mutex mutex_;
};

template<class T>
std::shared_ptr<std::vector<T> > VectorPool<T>::get(unsigned int k)
{
    return instance().getImpl(k);
}

template<class T>
VectorPool<T> & VectorPool<T>::instance()
{
    static VectorPool<T> instance;
    return instance;
}

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
        return std::shared_ptr<T>(t, [this] (T* obj) {
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




} // End of namespace util

} // End of namespace coco

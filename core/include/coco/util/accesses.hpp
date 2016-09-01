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
#include <string>
#include <unordered_map>
#include <map>

#include "generics.hpp"

namespace coco
{
namespace util
{


class split_iterator
{
public:
    split_iterator(const std::string & ain, char ac)
        : ip(0), in(ain), c(ac)
    {
        auto newip = in.find(c);
        if (newip == std::string::npos)
        {
            np = in.size()-  ip;
        }
        else
        {
            np = newip - ip;
        }
    }

    split_iterator()
        : ip(std::string::npos), np(std::string::npos)
    {}

    std::string operator * () const
    {
        return in.substr(ip, np);
    }

    split_iterator &operator++()
    {
        if (ip == std::string::npos)
            return *this;
        else
        {
            ip += np+1;
            if (ip >= in.size())
            {
                ip = std::string::npos;
            }
            else
            {
                auto newip = in.find(c, ip);
                if (newip == std::string::npos)
                {
                    np = in.size() - ip;
                }
                else
                {
                    np = newip - ip;
                }
            }
        }
        return *this;
    }

    bool operator != (const split_iterator & s) const
    {
        return !(s == *this);
    }

    bool        operator == (const split_iterator & s) const
    {
        return s.ip == ip && ip == std::string::npos;
    }

    std::string::size_type ip, np;
    std::string in;
    char c;
};

class string_splitter
{
public:
    string_splitter(const std::string aa, char ac)
        : a(aa), c(ac)
    {}
    split_iterator begin() { return split_iterator(a, c); }
    split_iterator end() { return split_iterator(); }

    std::string a;
    char c;
};


template <class MapClass, class T>
struct map_access
{
    static_assert(check_container<MapClass>::value, "Container not valid, only std::map or std::unordered_map");
    static_assert(check_spec<T>::value, "No a Key or Value");

    using map_t = MapClass;

    struct iterator
    {
        using iterator_t = typename std::conditional<std::is_const<map_t>::value,
                                                     typename map_t::const_iterator,
                                                     typename map_t::iterator
                                                    >::type;

        using pre_value_t = typename std::conditional<std::is_same<T, Key>::value,
                                                      typename map_t::key_type,
                                                      typename map_t::mapped_type
                                                    >::type;

        using value_t = typename std::conditional<std::is_const<map_t>::value,
                                                  typename std::add_const<pre_value_t>::type,
                                                  pre_value_t
                                                 >::type;
        iterator_t base_iterator;

        explicit iterator(iterator_t itr)
            : base_iterator(itr)
        {}

        bool operator != (const iterator& o) const
        {
            return base_iterator != o.base_iterator;
        }

        template<class U = T>
        typename std::enable_if<std::is_same<Key, U>::value,
                                typename std::add_const<value_t>::type
                            >::type
        operator*()
        {
            return base_iterator->first;
        }
        template<class U = T>
        typename std::enable_if<!std::is_same<Key, U>::value,
                                typename std::add_lvalue_reference<value_t>::type
                               >::type operator*()
        {
            return base_iterator->second;
        }
        template<class U = T>
        typename std::enable_if<std::is_same<Key, U>::value,
                                typename std::add_pointer<
                                    typename std::add_const<value_t>::type>::type
                               >::type operator->()
        {
            return &base_iterator->first;
        }
        template<class U = T>
        typename std::enable_if<!std::is_same<Key, U>::value,
                                typename std::add_pointer<value_t>::type
                               >::type operator->()
        {
            return &base_iterator->second;
        }

        iterator & operator++()
        {
            ++base_iterator;
            return * this;
        }

        iterator operator++(int)
        {
            iterator itr(*this);
            ++base_iterator;
            return itr;
        }
    };

    explicit map_access(map_t & x)
        : x_(x)
    {}
    iterator begin() { return iterator(x_.begin()); }
    iterator end()   { return iterator(x_.end());   }
    std::size_t size() const { return x_.size(); }
private:
    map_t & x_;
};

template <class Map>
map_access<Map, Key> keys_iteration(Map& x)
{
    return map_access<Map, Key>(x);
}
template <class Map>
map_access<const Map, Key> keys_iteration(const Map& x)
{
    return map_access<const Map, Key>(x);
}
template <class Map>
map_access<Map, Value> values_iteration(Map& x)
{
    return map_access<Map, Value>(x);
}
template <class Map>
map_access<const Map, Value> values_iteration(const Map& x)
{
    return map_access<const Map, Value>(x);
}


}  // end of namespace util

}  // end of namespace coco

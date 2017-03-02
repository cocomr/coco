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
#include <memory>
#include <vector>
#include <list>
#include <type_traits>
#include <map>
#include <unordered_map>
#include "coco/util/threading.h"


namespace std
{
template<int>  // begin with 0 here!
struct placeholder_template
{};

template<int N>
struct is_placeholder<placeholder_template<N> >
    : integral_constant<int, N+1>  // the one is important
{};
}  // end of namespace std

namespace coco
{
namespace util
{
struct enum_hash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

template<std::size_t...>
struct int_sequence
{};

template<std::size_t N, std::size_t... Is>
struct make_int_sequence
    : make_int_sequence<N-1, N-1, Is...>
{};

template<std::size_t... Is>
struct make_int_sequence<0, Is...>
    : int_sequence<Is...>
{};

/// Used to iterate over the keys of a map
template < template <typename...> class Template, typename T >
struct is_specialization_of : std::false_type {};

template < template <typename...> class Template, typename... Args >
struct is_specialization_of< Template, Template<Args...> > : std::true_type {};

template < template <typename...> class Template, typename... Args >
struct is_specialization_of< Template, const Template<Args...> > : std::true_type {};

template<class A, class B>
struct or_ : std::integral_constant<bool, A::value || B::value>{};

template<class T>
struct check_container
  : or_<is_specialization_of<std::map, T>,
        is_specialization_of<std::unordered_map, T>
       >{};

struct Key {};
struct Value {};

template<class T>
struct check_spec
  : or_<std::is_same<Key, T>,
        std::is_same<Value, T>
       >{};



// template< class T>
// struct get_functioner
// {};

template<class T>
struct get_functioner
{
    using ft =  get_functioner<decltype(&T::operator())>;          
    using target = typename ft::target;
    using fx = typename ft::fx;
};

template< class R, class U, class...Args>
struct get_functioner<R (U::*)(Args...) >
{
    typedef std::function<R(Args...)> target;
    using fx = R(Args...);
};

template< class R, class U, class...Args>
struct get_functioner<R (U::*)(Args...) const>
{
    typedef std::function<R(Args...)> target;
    using fx = R(Args...);
};

template< class R,  class...Args>
struct get_functioner< std::function<R(Args...) > >
{
    typedef std::function<R(Args...)> target;
    using fx = R(Args...);
};

template< class R, class...Args>
struct get_functioner<R(Args...)>
{
    typedef std::function<R(Args...)> target;
    using fx = R(Args...);
};

template< class R, class...Args>
struct get_functioner<R(*)(Args...)>
{
    typedef std::function<R(Args...)> target;
    using fx = R(Args...);
};

// utility
template<class R, class U, class... Args, std::size_t... Is>
auto bind_this_sub(R (U::*p)(Args...), U * pp, int_sequence<Is...>)
        -> decltype(std::bind(p, pp, std::placeholder_template<Is>{}...))
{
    return std::bind(p, pp, std::placeholder_template<Is>{}...);
}

// binds a member function only for the this pointer using std::bind
template<class R, class U, class... Args>
auto bind_this(R (U::*p)(Args...), U * pp)
        -> decltype(bind_this_sub(p, pp, make_int_sequence< sizeof...(Args) >{}))
{
    return bind_this_sub(p, pp, make_int_sequence< sizeof...(Args) >{});
}

// utility
template<class R, class U, class... Args, std::size_t... Is>
auto bind_this_sub(R (U::*p)(Args...) const, U * pp, int_sequence<Is...>)
        -> decltype(std::bind(p, pp, std::placeholder_template<Is>{}...))
{
    return std::bind(p, pp, std::placeholder_template<Is>{}...);
}

// binds a member function only for the this pointer using std::bind
template<class R, class U, class... Args>
auto bind_this(R (U::*p)(Args...) const, U * pp)
        -> decltype(bindthissub(p, pp, make_int_sequence< sizeof...(Args) >{}))
{
    return bind_this_sub(p, pp, make_int_sequence< sizeof...(Args) >{});
}

template<int... Idx>
struct sequence
{};

template<int N, int... Idx>
struct sequence_generator : sequence_generator<N - 1, N - 1, Idx...>
{};

template<int... Idx>
struct sequence_generator<0, Idx...>
{
   using type = sequence<Idx...>;
};
//Pass-through function that can be used to expand a function call on all arguments of a parameter pack
template<class... Args>
inline void pass_through(Args&&... args)
{}


template<class T, typename = const std::string &>
struct has_name
    : std::false_type
{};
template<class T>
struct has_name<T, decltype(std::declval<T>().instantiationName())>
        : std::true_type
{};

template <class T>
typename std::enable_if<has_name<T>::value, std::string>::type
task_name(const T* obj)
{
    return obj->instantiationName();
}

template <class T>
typename std::enable_if<!has_name<T>::value, std::string>::type
task_name(const T* obj)
{
    return "";
}


}  // end of namespace util

}  // end of namespace coco

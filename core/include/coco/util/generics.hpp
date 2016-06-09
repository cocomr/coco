/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
*/

#pragma once
#include <memory>
#include <vector>
#include <list>
#include <type_traits>
#include <map>
#include <unordered_map>
#include <condition_variable>


namespace std
{
template<int> // begin with 0 here!
struct placeholder_template
{};

template<int N>
struct is_placeholder<placeholder_template<N> >
    : integral_constant<int, N+1> // the one is important
{};
}

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
} // End of namespace util




} // End of namespace coco

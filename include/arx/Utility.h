/* This file is part of ArXLib, a C++ ArX Primitives Library.
 *
 * Copyright (C) 2008-2010 Alexander Fokin <apfokin@gmail.com>
 *
 * ArXLib is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * ArXLib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ArXLib. If not, see <http://www.gnu.org/licenses/>. 
 * 
 * $Id$ */
#ifndef ARX_UTILITY_H
#define ARX_UTILITY_H

#include "config.h"
#include <cassert>
#include <boost/preprocessor/stringize.hpp>
#include <functional>  /* for std::less. */
#include <utility>     /* for std::pair. */


// -------------------------------------------------------------------------- //
// Some useful defines
// -------------------------------------------------------------------------- //
#ifdef ARX_MSVC
#  define ALWAYS_INLINE __forceinline
#elif defined(ARX_GCC)
#  define ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#  define ALWAYS_INLINE inline
#endif

#ifdef ARX_MSVC
#  define NEVER_INLINE __declspec(noinline)
#elif defined(ARX_GCC)
#  define NEVER_INLINE __attribute__((__noinline__))
#else
#  define NEVER_INLINE
#endif

#ifdef ARX_MSVC
#  define NORETURN __declspec(noreturn)
#elif defined(ARX_GCC)
#  define NORETURN __attribute((__noreturn__))
#else
#  define NORETURN
#endif

#ifdef ARX_MSVC
#  define ARX_UNREACHABLE_CODE() __assume(false)
#elif defined(ARX_GCC) && (ARX_GCC >= 40500)
#  define ARX_UNREACHABLE_CODE() __builtin_unreachable()
#else 
#  define ARX_UNREACHABLE_CODE() for(;;) {}
#endif

#define Unreachable() {                                                         \
    assert(!"Unreachable code executed.");                                      \
    ARX_UNREACHABLE_CODE();                                                     \
  }


namespace arx {
// -------------------------------------------------------------------------- //
// Some useful functions
// -------------------------------------------------------------------------- //
  /**
   * Square template
   */
  template<class T> 
  inline T sqr(T x) {
    return x * x;
  }


// -------------------------------------------------------------------------- //
// UnorderedPair
// -------------------------------------------------------------------------- //
  /**
   * UnorderedPair stores an unordered pair of two values of the same type.
   * Derives from Cmp for empty base optimization.
   */
  template<class T, class Cmp = std::less<T> > class UnorderedPair: private Cmp {
  public:
    T first;
    T second;

    UnorderedPair() {}

    UnorderedPair(const T& a, const T& b, const Cmp& c = Cmp()): 
      Cmp(c), first(Cmp::operator()(a, b) ? a : b), second(Cmp::operator()(a, b) ? b : a) {}

    bool operator<(const UnorderedPair& other) const {
      return Cmp::operator()(first, other.first) ||
        (!Cmp::operator()(other.first, first) && Cmp::operator()(second, other.second));
    }

    bool operator>(const UnorderedPair& other) const {
      return other < *this;
    }

    bool operator>=(const UnorderedPair& other) const {
      return !(*this < other);
    }

    bool operator<=(const UnorderedPair& other) const {
      return !(*this > other);
    }

    bool operator==(const UnorderedPair& other) const {
      return *this >= other && *this <= other;
    }

    bool operator!=(const UnorderedPair& other) const {
      return !(*this == other);
    }
  };

  template<class T, class Cmp> 
  UnorderedPair<T, Cmp> make_upair(const T& a, const T& b, const Cmp& c) {
    return UnorderedPair<T, Cmp>(a, b, c);
  }

  template<class T> 
  UnorderedPair<T> make_upair(const T& a, const T& b) {
    return UnorderedPair<T>(a, b);
  }

} // namespace arx

#endif // ARX_UTILITY_H
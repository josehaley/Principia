#pragma once

#include "geometry/pair.hpp"

namespace principia {
namespace geometry {

template<typename T1, typename T2>
Pair<T1, T2>::Pair(T1 const& t1, T2 const& t2)
    : t1_(t1),
      t2_(t2) {}

template<typename T1, typename T2>
typename VectorOf<Pair<T1, T2>>::type Pair<T1, T2>::operator-(Pair const& from) const {
  return typename VectorOf<Pair<T1, T2>>::type(t1_ - from.t1_, t2_ - from.t2_);
}

template<typename T1, typename T2>
Pair<T1, T2> Pair<T1, T2>::operator+(typename VectorOf<Pair>::type const& translation) const {
}

template<typename T1, typename T2>
Pair<T1, T2> Pair<T1, T2>::operator-(typename VectorOf<Pair>::type const& translation) const {
}

template<typename T1, typename T2>
Pair<T1, T2>& Pair<T1, T2>::operator+=(typename VectorOf<Pair>::type const& translation) {
}

template<typename T1, typename T2>
Pair<T1, T2>& Pair<T1, T2>::operator-=(typename VectorOf<Pair>::type const& translation) {
}

template<typename T1, typename T2>
bool Pair<T1, T2>::operator==(Pair const& right) const {
}

template<typename T1, typename T2>
bool Pair<T1, T2>::operator!=(Pair const& right) const {
}

}  // namespace geometry
}  // namespace principia

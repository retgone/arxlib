#ifndef __ARX_SEQUENCEPROXY_H__
#define __ARX_SEQUENCEPROXY_H__

#include "config.h"
#include <boost/range.hpp>  /* for boost::begin & boost::end. */
#include "ContainerProxy.h"

namespace arx {
// -------------------------------------------------------------------------- //
// SequenceProxy
// -------------------------------------------------------------------------- //
  template<class Derived, class Container>
  class SequenceProxy: public ContainerProxy<Derived, Container> {
  public:
    void assign(size_type count, const value_type& value) {
      container().assign(count, value);
    }

    template<class InputIterator>
    void assign(InputIterator first, InputIterator last) {
      container().assign(first, last);
    }

    reference back() {
      return container().back();
    }

    const_reference back() const {
      return container().back();
    }

    reference front() {
      return container().front();
    }

    const_reference front() const {
      return container().front();
    }

    void insert(iterator where, size_type count, const value_type& value) {
      return container().insert(where, count, value);
    }

    template<class InputIterator>
    void insert(iterator where, InputIterator first, InputIterator last) {
      return container().insert(where, first, last);
    }

    void resize(size_type newSize) {
      container().resize(newSize);
    }

    void resize(size_type newSize, const value_type& value) {
      container().resize(newSize, value);
    }

    /* Additional mutators. */

    template<class InputRange>
    void insert(iterator where, InputRange range) {
      insert(where, boost::begin(range), boost::end(range));
    }
  };

} // namespace arx

#endif // __ARX_SEQUENCEPROXY_H__

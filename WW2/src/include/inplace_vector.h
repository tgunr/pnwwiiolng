#pragma once

#ifndef __GNUC__
# include <stdexcept>
#endif

#include <vector>

// class template inplace_vector -*- C++ -*-
// derived from std::array
// WWII::inplace_vector

namespace WWII
{
  /**
   *  @brief A standard container for storing a fixed size sequence of elements.
   *
   *  @ingroup sequences
   *
   *  Sets support random access iterators.
   *
   *  @param  Tp  Type of element. Required to be a complete type.
   *  @param  N  Number of elements.
  */
  template<typename _Tp, size_t _Nm>
    struct inplace_vector
    {
      typedef _Tp 	    			      value_type;
      typedef value_type&                   	      reference;
      typedef const value_type&             	      const_reference;
      typedef value_type*          		      iterator;
      typedef const value_type*			      const_iterator;
      typedef inplace_vector<_Tp, _Nm>                 self;
      typedef std::size_t                    	      size_type;
      typedef std::ptrdiff_t                   	      difference_type;
      typedef std::reverse_iterator<iterator>	      reverse_iterator;
      typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

      // Support for zero-sized arrays mandatory.
      size_type _M_size ;
      value_type _M_instance[_Nm > 0 ? _Nm : 1];

      inplace_vector() : _M_size(0) { }
      inplace_vector(const self& rhs) : _M_size(rhs.size()) { memcpy(data(), rhs.data(), sizeof(value_type) * rhs.size()) ; }

      // No explicit construct/copy/destroy for aggregate type.

      void
      assign(const value_type& __u)
      { std::fill_n(begin(), _Nm, __u); _M_size = _Nm ; }

      void
      swap(inplace_vector& __other)
      { std::swap_ranges(begin(), end(), __other.begin()); }

      // Iterators.
      iterator
      begin()
      { return iterator(_M_instance); }

      const_iterator
      begin() const
      { return const_iterator(_M_instance); }

      iterator
      end()
      { return begin() + size() ; }

      const_iterator
      end() const
      { return begin() + size() ; }

      reverse_iterator
      rbegin()
      { return reverse_iterator(this->end()); }

      const_reverse_iterator
      rbegin() const
      { return const_reverse_iterator(this->end()); }

      reverse_iterator
      rend()
      { return reverse_iterator(begin()); }

      const_reverse_iterator
      rend() const
      { return const_reverse_iterator(begin()); }

      // Capacity.
      size_type
      size() const { return _M_size; }

      size_type
      max_size() const { return _Nm ; }

      size_type
      capacity() const { return _Nm; }

      bool
      empty() const { return size() == 0; }

      bool
      full() const { return size() >= capacity(); }

      void
      clear()
      { _M_size = 0 ; }

      // Element access.
      reference
      operator[](size_type __n)
      { return _M_instance[__n]; }

      const_reference
      operator[](size_type __n) const
      { return _M_instance[__n]; }

      reference
      at(size_type __n)
      {
#if defined(__GNUC__)
		if (__builtin_expect(__n >= size(), false))
			std::__throw_out_of_range(__N("inplace_vector::at"));
#else
		if(__n >= size())
			throw std::range_error("inplace_vector::at") ;
#endif
	return _M_instance[__n];
      }

      const_reference
      at(size_type __n) const
      {
#if defined(__GNUC__)
		  if (__builtin_expect(__n >= size(), false))
			  std::__throw_out_of_range(__N("inplace_vector::at"));
#else
		  if(__n >= size())
			  throw std::range_error("inplace_vector::at") ;
#endif
	return _M_instance[__n];
      }

      reference
      front()
      { return *begin(); }

      const_reference
      front() const
      { return *begin(); }

      reference
      back()
      { return size() ? *(end() - 1) : *end(); }

      const_reference
      back() const
      { return size() ? *(end() - 1) : *end(); }

      _Tp*
      data()
      { return &_M_instance[0]; }

      const _Tp*
      data() const
      { return &_M_instance[0]; }

      bool
      push_back(const value_type& __x)
      {
        if ( size() >= capacity() ) return false ;
        _M_instance[_M_size++] = __x ;
        return true ;
      }

      void
      pop_back()
      {
        if ( size() > 0 )
          --_M_size ;
      }

      void
      resize(size_type __n)
      {
#if defined(__GNUC__)
		if (__builtin_expect(__n > capacity(), false))
	  		std::__throw_out_of_range(__N("inplace_vector::resize"));
#else
		if (__n > capacity())
			throw std::range_error("inplace_vector::resize") ;
#endif
		_M_size = __n ;
      }

      // Fast, unordered erasure.
      // Invalidates iterators.
      void
      swap_to_back_and_pop(iterator pos)
      {
        // You can't erase 'end'
        if ( pos == end() ) return ;
        // ASSUMPTION: You have an iterator that isn't end so size must be > 0
        --_M_size ;
        // If you were deleting end - 1, it will now be end and the swap
        // would no-longer be necessary.
        if ( pos != end() )
          *pos = *end() ;
      }
    };

  // Array comparisons.
  template<typename _Tp, size_t _Nm>
    inline bool
    operator==(const inplace_vector<_Tp, _Nm>& __one, const inplace_vector<_Tp, _Nm>& __two)
    { return std::equal(__one.begin(), __one.end(), __two.begin()); }

  template<typename _Tp, size_t _Nm>
    inline bool
    operator!=(const inplace_vector<_Tp, _Nm>& __one, const inplace_vector<_Tp, _Nm>& __two)
    { return !(__one == __two); }

  template<typename _Tp, size_t _Nm>
    inline bool
    operator<(const inplace_vector<_Tp, _Nm>& __a, const inplace_vector<_Tp, _Nm>& __b)
    {
      return std::lexicographical_compare(__a.begin(), __a.end(),
					  __b.begin(), __b.end());
    }

  template<typename _Tp, size_t _Nm>
    inline bool
    operator>(const inplace_vector<_Tp, _Nm>& __one, const inplace_vector<_Tp, _Nm>& __two)
    { return __two < __one; }

  template<typename _Tp, size_t _Nm>
    inline bool
    operator<=(const inplace_vector<_Tp, _Nm>& __one, const inplace_vector<_Tp, _Nm>& __two)
    { return !(__one > __two); }

  template<typename _Tp, size_t _Nm>
    inline bool
    operator>=(const inplace_vector<_Tp, _Nm>& __one, const inplace_vector<_Tp, _Nm>& __two)
    { return !(__one < __two); }

  // Specialized algorithms [6.2.2.2].
  template<typename _Tp, size_t _Nm>
    inline void
    swap(inplace_vector<_Tp, _Nm>& __one, inplace_vector<_Tp, _Nm>& __two)
    { std::swap_ranges(__one.begin(), __one.end(), __two.begin()); }

  // Tuple interface to class template inplace_vector [6.2.2.5].

  /// tuple_size
  template<typename _Tp>
    struct tuple_size;

  /// tuple_element
  template<size_t _Int, typename _Tp>
    struct tuple_element;

  template<typename _Tp, size_t _Nm>
    struct tuple_size<inplace_vector<_Tp, _Nm> >
    { static const size_t value = _Nm; };

  template<typename _Tp, size_t _Nm>
    const size_t
    tuple_size<inplace_vector<_Tp, _Nm> >::value;

  template<size_t _Int, typename _Tp, size_t _Nm>
    struct tuple_element<_Int, inplace_vector<_Tp, _Nm> >
    { typedef _Tp type; };

  template<size_t _Int, typename _Tp, size_t _Nm>
    inline _Tp&
    get(inplace_vector<_Tp, _Nm>& __arr)
    { return __arr[_Int]; }

  template<size_t _Int, typename _Tp, size_t _Nm>
    inline const _Tp&
    get(const inplace_vector<_Tp, _Nm>& __arr)
    { return __arr[_Int]; }

}


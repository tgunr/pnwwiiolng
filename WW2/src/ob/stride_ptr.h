#pragma once

/************************************************************************************************
	stride_ptr
*************************************************************************************************/
// Strided Pointer Class

template<class T>
struct stride_ptr
{
	inline stride_ptr() 						{ ptr = 0; stride = 0; }
	inline stride_ptr(T* p, int s)				{ ptr = p; stride = s; }

	inline stride_ptr& operator++() 			{ ptr = stride_it( stride); return *this; }
	inline stride_ptr& operator--() 			{ ptr = stride_it(-stride); return *this; }

	inline stride_ptr& operator+=(int n)		{ ptr = stride_at( n); return *this; }
	inline stride_ptr& operator-=(int n)		{ ptr = stride_at(-n); return *this; }

	inline stride_ptr  operator++(int) 			{ stride_ptr tmp = *this;  ptr = stride_it( stride); return tmp; }
	inline stride_ptr  operator--(int) 			{ stride_ptr tmp = *this;  ptr = stride_it(-stride); return tmp; }

	inline stride_ptr  operator+ (int n) const	{ stride_ptr tmp = *this; return tmp += n; }
	inline stride_ptr  operator- (int n) const	{ stride_ptr tmp = *this; return tmp -= n; }

	inline T& operator[](int n) const			{ return *stride_at(n); }

	inline T* operator->() const				{ return  ptr; }
	inline T& operator* () const				{ return *ptr; }

	T* ptr;
	unsigned long stride;

	inline T* stride_it(int n) const			{ return (T*)((unsigned char*)ptr + n); }
	inline T* stride_at(int n) const			{ return (T*)((unsigned char*)ptr + n * stride); }
};

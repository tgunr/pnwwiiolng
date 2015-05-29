#ifndef PHUI_REFERENCED_H
#define PHUI_REFERENCED_H

#include <cassert>

namespace phui
{

/************************************************************************************************
	Referenced
*************************************************************************************************/

class Referenced
{
public:
	constexpr Referenced() : _refcount(0) {}
	
	void ref() noexcept { ++_refcount; }
	void unref() noexcept { if (--_refcount <= 0) delete this; }
	void Acquire() noexcept { ref(); }
	void Release() noexcept { unref(); }

protected:
	virtual ~Referenced() { assert(_refcount <= 0); }

private:
	int _refcount;
};

class UnownedReference : public Referenced
{
public:
	UnownedReference(void* d) : _data(d) {}
	void* _data;
};

template <class T>
class ref_ptr
{
public:
	using value_type = typename T;
	using self_type = typename ref_ptr<value_type>;

	constexpr ref_ptr() noexcept : _ref(nullptr) {}
	ref_ptr(T* ref) noexcept : _ref(ref) { if(_ref) _ref->ref(); }
	ref_ptr(const self_type& p) noexcept : ref_ptr(p._ref) {}
	ref_ptr(self_type&& p) noexcept : _ref(nullptr) { std::swap(_ref, p._ref); }
	~ref_ptr() noexcept { reset(nullptr); }
	
	ref_ptr& operator=(const self_type& p) noexcept { reset(p._ref); return *this; }
	ref_ptr& operator=(self_type&& p) noexcept { std::swap(_ref, p._ref); return *this; }

	void reset(T* p) noexcept
	{
		if (_ref != p) {
			std::swap(_ref, p);
			if (p)
				p->unref();
		}
	}
	
	constexpr bool operator==(const self_type& p) const noexcept { return (_ref == p._ref); }
	constexpr bool operator==(T* ref) const noexcept { return _ref == ref; }
	constexpr bool operator==(self_type&& p) const noexcept { return _pref == p._ref; }
	constexpr T* operator->() const noexcept { return _ref; }
	constexpr T* ptr() const noexcept { return _ref; }
	constexpr bool valid() const { return _ref != nullptr; }
	explicit constexpr operator bool() const noexcept { return _ref != nullptr; }

	friend void swap(self_type& lhs, self_type& rhs) noexcept { std::swap(lhs._ref, rhs._ref); }

private:
	T* _ref;
};

typedef ref_ptr<Referenced> referenced_ptr;

}

#endif // PHUI_REFERENCED_H

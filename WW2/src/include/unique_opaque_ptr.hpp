#pragma once

// RAII helpers for opaque pointers
template <typename TOpaque>
struct opaque_ptr_traits final {
	static inline void destroy(TOpaque value) noexcept; // wish this could error in compiler instead of linker
};
#define DEFINTE_OPAQUE_PTR_DELETER(ptrtype, deleterfunc) \
template <> struct opaque_ptr_traits<ptrtype> { \
	static inline void destroy(ptrtype value) noexcept { \
		if (value) { \
			deleterfunc(value); \
		} \
	} \
}

template <typename TOpaque>
class unique_opaque_ptr
{
	private:
	TOpaque __value;

	public:
	constexpr unique_opaque_ptr() noexcept : __value(nullptr) {}
	constexpr unique_opaque_ptr(nullptr_t) noexcept : __value(nullptr) {}
	unique_opaque_ptr(TOpaque value) noexcept : __value(value) {}
	~unique_opaque_ptr() { reset(nullptr); }

	// When moved, leaves previous object in destructible state.
	unique_opaque_ptr(unique_opaque_ptr &&rhs) noexcept : __value(std::exchange(rhs.__value, nullptr)) {}
	unique_opaque_ptr &operator=(unique_opaque_ptr &&rhs) {
		std::swap(rhs.__value, __value); // speculative destroy is probably safe here but just in case use the swap algorithm anyway
		return *this;
	}

	// In type-specific context, is the pointer
	operator TOpaque() const noexcept { return __value; }

	// In bool context, is whether the pointer is non-NULL
	explicit operator bool() const noexcept { return __value != nullptr; }

	// Change the value, destroying the old one
	inline void reset(TOpaque value = nullptr) noexcept {
		opaque_ptr_traits<TOpaque>::destroy(__value);
		__value = value;
	}

	// Return the raw pointer without releasing it
	inline TOpaque get() const noexcept { return __value; }

	// Return the raw pointer and release it from the object's ownership
	inline TOpaque release() noexcept { return std::exchange(__value, nullptr); }

	// In addressof context, a pointer to the raw pointer. Immediately destroys the existing value.
	inline TOpaque *operator&() noexcept {
		reset(nullptr);
		return &__value;
	}
};
#define DEFINE_UNIQUE_OPAQUE_PTR2(name, ptrtype) using name = unique_opaque_ptr<ptrtype>
#define DEFINE_UNIQUE_OPAQUE_PTR(name, ptrtype) DEFINE_UNIQUE_OPAQUE_PTR2(unique_ ## ptrtype, ptrtype)
#define DEFINE_UNIQUE_OPAQUE_PTR_WITH_DELETER2(name, ptrtype, deleterfunc) \
	DEFINTE_OPAQUE_PTR_DELETER(ptrtype, deleterfunc);	\
	DEFINE_UNIQUE_OPAQUE_PTR2(name, ptrtype)
#define DEFINE_UNIQUE_OPAQUE_PTR_WITH_DELETER(ptrtype, deleterfunc) DEFINE_UNIQUE_OPAQUE_PTR_WITH_DELETER2(unique_ ## ptrtype, ptrtype, deleterfunc)

// Define one of the above for HKEY
DEFINE_UNIQUE_OPAQUE_PTR_WITH_DELETER(HKEY, RegCloseKey);

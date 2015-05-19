#pragma once

// Created: Thu May 18th 18:21 PST 2015
// Originating Programmer: Oliver "kfsone" Smith
//
// Bit-twiddling templates

namespace Teul
{

	namespace Bits
	{
		//! @brief Count the highest bit number needed to describe the given value, with
		//! a minimum of 1 bit to describe an unsigned value and 2 to describe a signed value.
		template<class T, typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, T> = 0>
		constexpr static inline size_t Count(T _value) noexcept
		{
			return Count_impl(static_cast<uint64_t>(_value), 1);
		}
		template<class T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, T> = 0>
		constexpr static inline size_t Count(T _value) noexcept
		{
			return (_value < 0)
				? Count_impl(static_cast<uint64_t>(~_value), 2)
				: Count_impl(static_cast<uint64_t>(_value), 2);
		}
		constexpr static inline size_t Count_impl(uint64_t _value, size_t bitsRequired) noexcept
		{
			return (_value >> 1)
				? Count_impl(_value >> 1, bitsRequired + 1)
				: bitsRequired;
		}

		//! @brief Test whether a given numeric value would pack into a given number of bits.
		template<class T, typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, T> = 0>
		constexpr bool Fit(T value, size_t bits) noexcept
		{
			return (bits > 0 && ((bits >= sizeof(T) * 8) || (value >> bits) == 0));
		}
		template<class T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, T> = 0>
		constexpr bool Fit(T value, size_t bits) noexcept
		{
			return (bits > 1)
				? (value >= 0)
				? Fit(std::make_unsigned_t<T>(value), bits - 1)
				: Fit(std::make_unsigned_t<T>(~value), bits - 1)
				: false;
		}
	}

}
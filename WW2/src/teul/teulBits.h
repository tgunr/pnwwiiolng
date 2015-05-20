#pragma once

// Created: Thu May 18th 18:21 PST 2015
// Originating Programmer: Oliver "kfsone" Smith
//
// Bit-twiddling templates

namespace Teul
{

	namespace Bits
	{
		//! @brief Returns calculates number of bits in a given integral type.
		template<typename T, std::enable_if_t<std::is_integral<T>::value, T> = 0>
		constexpr size_t Size() noexcept
		{
			return sizeof(T) * 8;
		}

		//! @brief Calculates remaining bits in an integral type
		template<typename T, std::enable_if_t<std::is_integral<T>::value, T> = 0>
		constexpr size_t Remainder(size_t bits) noexcept
		{
			return bits < Size<T>() ? Size<T>() - bits : 0;
		}

		//! @brief Returns the low N bits from an integral type
		template<typename T, std::enable_if_t<std::is_integral<T>::value, T> = 0>
		constexpr T Low(T value, size_t bits) noexcept
		{
			return (bits >= Size<T>())
				? value
				: (bits != 0)
					? T(value << Remainder<T>(bits)) >> (Remainder<T>(bits))
					: 0;
		}

		template<typename T, std::enable_if_t<std::is_integral<T>::value, T> = 0>
		constexpr T High(T value, size_t bits) noexcept
		{
			return (bits >= Size<T>())
				? value
				: (bits != 0)
					? T(value >> Remainder<T>(bits))
					: 0;
		}

		template<typename T, std::enable_if_t<std::is_integral<T>::value, T> = 0>
		constexpr T High_InSitu(T value, size_t bits) noexcept
		{
			return (bits >= Size<T>())
				? value
				: (bits != 0)
					? T(value >> Remainder<T>(bits)) << Remainder<T>(bits)
					: 0;
		}

		constexpr static inline size_t Count_impl(uint64_t _value, size_t bitsRequired) noexcept
		{
			return (_value >> 1)
				? Count_impl(_value >> 1, bitsRequired + 1)
				: bitsRequired;
		}

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

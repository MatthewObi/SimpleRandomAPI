///Author: Matthew Obi
///Copyright 2021
#ifndef RANDOM_SINGLE_INCLUDE_HPP
#define RANDOM_SINGLE_INCLUDE_HPP 1

#include <random>

///#include "pcg/pcg_random.hpp"
/*
 * PCG Random Number Generation for C++
 *
 * Copyright 2014-2019 Melissa O'Neill <oneill@pcg-random.org>,
 *                     and the PCG Project contributors.
 *
 * SPDX-License-Identifier: (Apache-2.0 OR MIT)
 *
 * Licensed under the Apache License, Version 2.0 (provided in
 * LICENSE-APACHE.txt and at http://www.apache.org/licenses/LICENSE-2.0)
 * or under the MIT license (provided in LICENSE-MIT.txt and at
 * http://opensource.org/licenses/MIT), at your option. This file may not
 * be copied, modified, or distributed except according to those terms.
 *
 * Distributed on an "AS IS" BASIS, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied.  See your chosen license for details.
 *
 * For additional information about the PCG random number generation scheme,
 * visit http://www.pcg-random.org/.
 */

/*
 * This code provides the reference implementation of the PCG family of
 * random number generators.  The code is complex because it implements
 *
 *      - several members of the PCG family, specifically members corresponding
 *        to the output functions:
 *             - XSH RR         (good for 64-bit state, 32-bit output)
 *             - XSH RS         (good for 64-bit state, 32-bit output)
 *             - XSL RR         (good for 128-bit state, 64-bit output)
 *             - RXS M XS       (statistically most powerful generator)
 *             - XSL RR RR      (good for 128-bit state, 128-bit output)
 *             - and RXS, RXS M, XSH, XSL       (mostly for testing)
 *      - at potentially *arbitrary* bit sizes
 *      - with four different techniques for random streams (MCG, one-stream
 *        LCG, settable-stream LCG, unique-stream LCG)
 *      - and the extended generation schemes allowing arbitrary periods
 *      - with all features of C++11 random number generation (and more),
 *        some of which are somewhat painful, including
 *            - initializing with a SeedSequence which writes 32-bit values
 *              to memory, even though the state of the generator may not
 *              use 32-bit values (it might use smaller or larger integers)
 *            - I/O for RNGs and a prescribed format, which needs to handle
 *              the issue that 8-bit and 128-bit integers don't have working
 *              I/O routines (e.g., normally 8-bit = char, not integer)
 *            - equality and inequality for RNGs
 *      - and a number of convenience typedefs to mask all the complexity
 *
 * The code employes a fairly heavy level of abstraction, and has to deal
 * with various C++ minutia.  If you're looking to learn about how the PCG
 * scheme works, you're probably best of starting with one of the other
 * codebases (see www.pcg-random.org).  But if you're curious about the
 * constants for the various output functions used in those other, simpler,
 * codebases, this code shows how they are calculated.
 *
 * On the positive side, at least there are convenience typedefs so that you
 * can say
 *
 *      pcg32 myRNG;
 *
 * rather than:
 *
 *      pcg_detail::engine<
 *          uint32_t,                                           // Output Type
 *          uint64_t,                                           // State Type
 *          pcg_detail::xsh_rr_mixin<uint32_t, uint64_t>, true, // Output Func
 *          pcg_detail::specific_stream<uint64_t>,              // Stream Kind
 *          pcg_detail::default_multiplier<uint64_t>            // LCG Mult
 *      > myRNG;
 *
 */

#ifndef PCG_RAND_HPP_INCLUDED
#define PCG_RAND_HPP_INCLUDED 1

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <limits>
#include <iostream>
#include <iterator>
#include <type_traits>
#include <utility>
#include <locale>
#include <new>
#include <stdexcept>

#ifdef _MSC_VER
    #pragma warning(disable:4146)
#endif

#ifdef _MSC_VER
    #define PCG_ALWAYS_INLINE __forceinline
#elif __GNUC__
    #define PCG_ALWAYS_INLINE __attribute__((always_inline))
#else
    #define PCG_ALWAYS_INLINE inline
#endif

/*
 * The pcg_extras namespace contains some support code that is likley to
 * be useful for a variety of RNGs, including:
 *      - 128-bit int support for platforms where it isn't available natively
 *      - bit twiddling operations
 *      - I/O of 128-bit and 8-bit integers
 *      - Handling the evilness of SeedSeq
 *      - Support for efficiently producing random numbers less than a given
 *        bound
 */

///#include "pcg_extras.hpp"

/*
 * PCG Random Number Generation for C++
 *
 * Copyright 2014-2017 Melissa O'Neill <oneill@pcg-random.org>,
 *                     and the PCG Project contributors.
 *
 * SPDX-License-Identifier: (Apache-2.0 OR MIT)
 *
 * Licensed under the Apache License, Version 2.0 (provided in
 * LICENSE-APACHE.txt and at http://www.apache.org/licenses/LICENSE-2.0)
 * or under the MIT license (provided in LICENSE-MIT.txt and at
 * http://opensource.org/licenses/MIT), at your option. This file may not
 * be copied, modified, or distributed except according to those terms.
 *
 * Distributed on an "AS IS" BASIS, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied.  See your chosen license for details.
 *
 * For additional information about the PCG random number generation scheme,
 * visit http://www.pcg-random.org/.
 */

/*
 * This file provides support code that is useful for random-number generation
 * but not specific to the PCG generation scheme, including:
 *      - 128-bit int support for platforms where it isn't available natively
 *      - bit twiddling operations
 *      - I/O of 128-bit and 8-bit integers
 *      - Handling the evilness of SeedSeq
 *      - Support for efficiently producing random numbers less than a given
 *        bound
 */

#ifndef PCG_EXTRAS_HPP_INCLUDED
#define PCG_EXTRAS_HPP_INCLUDED 1

#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <limits>
#include <iostream>
#include <type_traits>
#include <utility>
#include <locale>
#include <iterator>

#ifdef __GNUC__
    #include <cxxabi.h>
#endif

/*
 * Abstractions for compiler-specific directives
 */

#ifdef __GNUC__
    #define PCG_NOINLINE __attribute__((noinline))
#else
    #define PCG_NOINLINE
#endif

/*
 * Some members of the PCG library use 128-bit math.  When compiling on 64-bit
 * platforms, both GCC and Clang provide 128-bit integer types that are ideal
 * for the job.
 *
 * On 32-bit platforms (or with other compilers), we fall back to a C++
 * class that provides 128-bit unsigned integers instead.  It may seem
 * like we're reinventing the wheel here, because libraries already exist
 * that support large integers, but most existing libraries provide a very
 * generic multiprecision code, but here we're operating at a fixed size.
 * Also, most other libraries are fairly heavyweight.  So we use a direct
 * implementation.  Sadly, it's much slower than hand-coded assembly or
 * direct CPU support.
 *
 */
#if __SIZEOF_INT128__ && !PCG_FORCE_EMULATED_128BIT_MATH
    namespace pcg_extras {
        typedef __uint128_t pcg128_t;
    }
    #define PCG_128BIT_CONSTANT(high,low) \
            ((pcg_extras::pcg128_t(high) << 64) + low)
#else
    ///#include "pcg_uint128.hpp"

	/*
	* PCG Random Number Generation for C++
	*
	* Copyright 2014-2021 Melissa O'Neill <oneill@pcg-random.org>,
	*                     and the PCG Project contributors.
	*
	* SPDX-License-Identifier: (Apache-2.0 OR MIT)
	*
	* Licensed under the Apache License, Version 2.0 (provided in
	* LICENSE-APACHE.txt and at http://www.apache.org/licenses/LICENSE-2.0)
	* or under the MIT license (provided in LICENSE-MIT.txt and at
	* http://opensource.org/licenses/MIT), at your option. This file may not
	* be copied, modified, or distributed except according to those terms.
	*
	* Distributed on an "AS IS" BASIS, WITHOUT WARRANTY OF ANY KIND, either
	* express or implied.  See your chosen license for details.
	*
	* For additional information about the PCG random number generation scheme,
	* visit http://www.pcg-random.org/.
	*/

	/*
	* This code provides a a C++ class that can provide 128-bit (or higher)
	* integers.  To produce 2K-bit integers, it uses two K-bit integers,
	* placed in a union that allowes the code to also see them as four K/2 bit
	* integers (and access them either directly name, or by index).
	*
	* It may seem like we're reinventing the wheel here, because several
	* libraries already exist that support large integers, but most existing
	* libraries provide a very generic multiprecision code, but here we're
	* operating at a fixed size.  Also, most other libraries are fairly
	* heavyweight.  So we use a direct implementation.  Sadly, it's much slower
	* than hand-coded assembly or direct CPU support.
	*/

	#ifndef PCG_UINT128_HPP_INCLUDED
	#define PCG_UINT128_HPP_INCLUDED 1

	#include <cstdint>
	#include <cstdio>
	#include <cassert>
	#include <climits>
	#include <utility>
	#include <initializer_list>
	#include <type_traits>

	#if defined(_MSC_VER)  // Use MSVC++ intrinsics
	#include <intrin.h>
	#endif

	/*
	* We want to lay the type out the same way that a native type would be laid
	* out, which means we must know the machine's endian, at compile time.
	* This ugliness attempts to do so.
	*/

	#ifndef PCG_LITTLE_ENDIAN
		#if defined(__BYTE_ORDER__)
			#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
				#define PCG_LITTLE_ENDIAN 1
			#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
				#define PCG_LITTLE_ENDIAN 0
			#else
				#error __BYTE_ORDER__ does not match a standard endian, pick a side
			#endif
		#elif __LITTLE_ENDIAN__ || _LITTLE_ENDIAN
			#define PCG_LITTLE_ENDIAN 1
		#elif __BIG_ENDIAN__ || _BIG_ENDIAN
			#define PCG_LITTLE_ENDIAN 0
		#elif __x86_64 || __x86_64__ || _M_X64 || __i386 || __i386__ || _M_IX86
			#define PCG_LITTLE_ENDIAN 1
		#elif __powerpc__ || __POWERPC__ || __ppc__ || __PPC__ \
			|| __m68k__ || __mc68000__
			#define PCG_LITTLE_ENDIAN 0
		#else
			#error Unable to determine target endianness
		#endif
	#endif

	#if INTPTR_MAX == INT64_MAX && !defined(PCG_64BIT_SPECIALIZATIONS)
		#define PCG_64BIT_SPECIALIZATIONS 1
	#endif

	namespace pcg_extras {

	// Recent versions of GCC have intrinsics we can use to quickly calculate
	// the number of leading and trailing zeros in a number.  If possible, we
	// use them, otherwise we fall back to old-fashioned bit twiddling to figure
	// them out.

	#ifndef PCG_BITCOUNT_T
		typedef uint8_t bitcount_t;
	#else
		typedef PCG_BITCOUNT_T bitcount_t;
	#endif

	/*
	* Provide some useful helper functions
	*      * flog2                 floor(log2(x))
	*      * trailingzeros         number of trailing zero bits
	*/

	#if defined(__GNUC__)   // Any GNU-compatible compiler supporting C++11 has
							// some useful intrinsics we can use.

	inline bitcount_t flog2(uint32_t v)
	{
		return 31 - __builtin_clz(v);
	}

	inline bitcount_t trailingzeros(uint32_t v)
	{
		return __builtin_ctz(v);
	}

	inline bitcount_t flog2(uint64_t v)
	{
	#if UINT64_MAX == ULONG_MAX
		return 63 - __builtin_clzl(v);
	#elif UINT64_MAX == ULLONG_MAX
		return 63 - __builtin_clzll(v);
	#else
		#error Cannot find a function for uint64_t
	#endif
	}

	inline bitcount_t trailingzeros(uint64_t v)
	{
	#if UINT64_MAX == ULONG_MAX
		return __builtin_ctzl(v);
	#elif UINT64_MAX == ULLONG_MAX
		return __builtin_ctzll(v);
	#else
		#error Cannot find a function for uint64_t
	#endif
	}

	#elif defined(_MSC_VER)  // Use MSVC++ intrinsics

	#pragma intrinsic(_BitScanReverse, _BitScanForward)
	#if defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64)
	#pragma intrinsic(_BitScanReverse64, _BitScanForward64)
	#endif

	inline bitcount_t flog2(uint32_t v)
	{
		unsigned long i;
		_BitScanReverse(&i, v);
		return bitcount_t(i);
	}

	inline bitcount_t trailingzeros(uint32_t v)
	{
		unsigned long i;
		_BitScanForward(&i, v);
		return bitcount_t(i);
	}

	inline bitcount_t flog2(uint64_t v)
	{
	#if defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64)
		unsigned long i;
		_BitScanReverse64(&i, v);
		return bitcount_t(i);
	#else
		// 32-bit x86
		uint32_t high = v >> 32;
		uint32_t low  = uint32_t(v);
		return high ? 32+flog2(high) : flog2(low);
	#endif
	}

	inline bitcount_t trailingzeros(uint64_t v)
	{
	#if defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64)
		unsigned long i;
		_BitScanForward64(&i, v);
		return bitcount_t(i);
	#else
		// 32-bit x86
		uint32_t high = v >> 32;
		uint32_t low  = uint32_t(v);
		return low ? trailingzeros(low) : trailingzeros(high)+32;
	#endif
	}

	#else                   // Otherwise, we fall back to bit twiddling
							// implementations

	inline bitcount_t flog2(uint32_t v)
	{
		// Based on code by Eric Cole and Mark Dickinson, which appears at
		// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn

		static const uint8_t multiplyDeBruijnBitPos[32] = {
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
		};

		v |= v >> 1; // first round down to one less than a power of 2
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;

		return multiplyDeBruijnBitPos[(uint32_t)(v * 0x07C4ACDDU) >> 27];
	}

	inline bitcount_t trailingzeros(uint32_t v)
	{
		static const uint8_t multiplyDeBruijnBitPos[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
		};

		return multiplyDeBruijnBitPos[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
	}

	inline bitcount_t flog2(uint64_t v)
	{
		uint32_t high = v >> 32;
		uint32_t low  = uint32_t(v);

		return high ? 32+flog2(high) : flog2(low);
	}

	inline bitcount_t trailingzeros(uint64_t v)
	{
		uint32_t high = v >> 32;
		uint32_t low  = uint32_t(v);

		return low ? trailingzeros(low) : trailingzeros(high)+32;
	}

	#endif

	inline bitcount_t flog2(uint8_t v)
	{
		return flog2(uint32_t(v));
	}

	inline bitcount_t flog2(uint16_t v)
	{
		return flog2(uint32_t(v));
	}

	#if __SIZEOF_INT128__
	inline bitcount_t flog2(__uint128_t v)
	{
		uint64_t high = uint64_t(v >> 64);
		uint64_t low  = uint64_t(v);

		return high ? 64+flog2(high) : flog2(low);
	}
	#endif

	inline bitcount_t trailingzeros(uint8_t v)
	{
		return trailingzeros(uint32_t(v));
	}

	inline bitcount_t trailingzeros(uint16_t v)
	{
		return trailingzeros(uint32_t(v));
	}

	#if __SIZEOF_INT128__
	inline bitcount_t trailingzeros(__uint128_t v)
	{
		uint64_t high = uint64_t(v >> 64);
		uint64_t low  = uint64_t(v);
		return low ? trailingzeros(low) : trailingzeros(high)+64;
	}
	#endif

	template <typename UInt>
	inline bitcount_t clog2(UInt v)
	{
		return flog2(v) + ((v & (-v)) != v);
	}

	template <typename UInt>
	inline UInt addwithcarry(UInt x, UInt y, bool carryin, bool* carryout)
	{
		UInt half_result = y + carryin;
		UInt result = x + half_result;
		*carryout = (half_result < y) || (result < x);
		return result;
	}

	template <typename UInt>
	inline UInt subwithcarry(UInt x, UInt y, bool carryin, bool* carryout)
	{
		UInt half_result = y + carryin;
		UInt result = x - half_result;
		*carryout = (half_result < y) || (result > x);
		return result;
	}


	template <typename UInt, typename UIntX2>
	class uint_x4 {
	// private:
		static constexpr unsigned int UINT_BITS = sizeof(UInt) * CHAR_BIT;
	public:
		union {
	#if PCG_LITTLE_ENDIAN
			struct {
				UInt v0, v1, v2, v3;
			} w;
			struct {
				UIntX2 v01, v23;
			} d;
	#else
			struct {
				UInt v3, v2, v1, v0;
			} w;
			struct {
				UIntX2 v23, v01;
			} d;
	#endif
			// For the array access versions, the code that uses the array
			// must handle endian itself.  Yuck.
			UInt wa[4];
		};

	public:
		uint_x4() = default;

		constexpr uint_x4(UInt v3, UInt v2, UInt v1, UInt v0)
	#if PCG_LITTLE_ENDIAN
		: w{v0, v1, v2, v3}
	#else
		: w{v3, v2, v1, v0}
	#endif
		{
			// Nothing (else) to do
		}

		constexpr uint_x4(UIntX2 v23, UIntX2 v01)
	#if PCG_LITTLE_ENDIAN
		: d{v01,v23}
	#else
		: d{v23,v01}
	#endif
		{
			// Nothing (else) to do
		}

		constexpr uint_x4(UIntX2 v01)
	#if PCG_LITTLE_ENDIAN
		: d{v01, UIntX2(0)}
	#else
		: d{UIntX2(0),v01}
	#endif
		{
			// Nothing (else) to do
		}

		template<class Integral,
				typename std::enable_if<(std::is_integral<Integral>::value
										&& sizeof(Integral) <= sizeof(UIntX2))
										>::type* = nullptr>
		constexpr uint_x4(Integral v01)
	#if PCG_LITTLE_ENDIAN
		: d{UIntX2(v01), UIntX2(0)}
	#else
		: d{UIntX2(0), UIntX2(v01)}
	#endif
		{
			// Nothing (else) to do
		}

		explicit constexpr operator UIntX2() const
		{
			return d.v01;
		}

		template<class Integral,
				typename std::enable_if<(std::is_integral<Integral>::value
										&& sizeof(Integral) <= sizeof(UIntX2))
										>::type* = nullptr>
		explicit constexpr operator Integral() const
		{
			return Integral(d.v01);
		}

		explicit constexpr operator bool() const
		{
			return d.v01 || d.v23;
		}

		template<typename U, typename V>
		friend uint_x4<U,V> operator*(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator*(const uint_x4<U,V>&, V);

		template<typename U, typename V>
		friend std::pair< uint_x4<U,V>,uint_x4<U,V> >
			divmod(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator+(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator-(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator<<(const uint_x4<U,V>&, const bitcount_t shift);

		template<typename U, typename V>
		friend uint_x4<U,V> operator>>(const uint_x4<U,V>&, const bitcount_t shift);

	#if PCG_64BIT_SPECIALIZATIONS
		template<typename U>
		friend uint_x4<U,uint64_t> operator<<(const uint_x4<U,uint64_t>&, const bitcount_t shift);

		template<typename U>
		friend uint_x4<U,uint64_t> operator>>(const uint_x4<U,uint64_t>&, const bitcount_t shift);
	#endif

		template<typename U, typename V>
		friend uint_x4<U,V> operator&(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator|(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator^(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bool operator==(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bool operator!=(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bool operator<(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bool operator<=(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bool operator>(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bool operator>=(const uint_x4<U,V>&, const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator~(const uint_x4<U,V>&);

		template<typename U, typename V>
		friend uint_x4<U,V> operator-(const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bitcount_t flog2(const uint_x4<U,V>&);

		template<typename U, typename V>
		friend bitcount_t trailingzeros(const uint_x4<U,V>&);

	#if PCG_64BIT_SPECIALIZATIONS
		template<typename U>
		friend bitcount_t flog2(const uint_x4<U,uint64_t>&);

		template<typename U>
		friend bitcount_t trailingzeros(const uint_x4<U,uint64_t>&);
	#endif

		uint_x4& operator*=(const uint_x4& rhs)
		{
			uint_x4 result = *this * rhs;
			return *this = result;
		}

		uint_x4& operator*=(UIntX2 rhs)
		{
			uint_x4 result = *this * rhs;
			return *this = result;
		}

		uint_x4& operator/=(const uint_x4& rhs)
		{
			uint_x4 result = *this / rhs;
			return *this = result;
		}

		uint_x4& operator%=(const uint_x4& rhs)
		{
			uint_x4 result = *this % rhs;
			return *this = result;
		}

		uint_x4& operator+=(const uint_x4& rhs)
		{
			uint_x4 result = *this + rhs;
			return *this = result;
		}

		uint_x4& operator-=(const uint_x4& rhs)
		{
			uint_x4 result = *this - rhs;
			return *this = result;
		}

		uint_x4& operator&=(const uint_x4& rhs)
		{
			uint_x4 result = *this & rhs;
			return *this = result;
		}

		uint_x4& operator|=(const uint_x4& rhs)
		{
			uint_x4 result = *this | rhs;
			return *this = result;
		}

		uint_x4& operator^=(const uint_x4& rhs)
		{
			uint_x4 result = *this ^ rhs;
			return *this = result;
		}

		uint_x4& operator>>=(bitcount_t shift)
		{
			uint_x4 result = *this >> shift;
			return *this = result;
		}

		uint_x4& operator<<=(bitcount_t shift)
		{
			uint_x4 result = *this << shift;
			return *this = result;
		}

	};

	template<typename U, typename V>
	bitcount_t flog2(const uint_x4<U,V>& v)
	{
	#if PCG_LITTLE_ENDIAN
		for (uint8_t i = 4; i !=0; /* dec in loop */) {
			--i;
	#else
		for (uint8_t i = 0; i < 4; ++i) {
	#endif
			if (v.wa[i] == 0)
				continue;
			return flog2(v.wa[i]) + uint_x4<U,V>::UINT_BITS*i;
		}
		abort();
	}

	template<typename U, typename V>
	bitcount_t trailingzeros(const uint_x4<U,V>& v)
	{
	#if PCG_LITTLE_ENDIAN
		for (uint8_t i = 0; i < 4; ++i) {
	#else
		for (uint8_t i = 4; i !=0; /* dec in loop */) {
			--i;
	#endif
			if (v.wa[i] != 0)
				return trailingzeros(v.wa[i]) + uint_x4<U,V>::UINT_BITS*i;
		}
		return uint_x4<U,V>::UINT_BITS*4;
	}

	#if PCG_64BIT_SPECIALIZATIONS
	template<typename UInt32>
	bitcount_t flog2(const uint_x4<UInt32,uint64_t>& v)
	{
		return v.d.v23 > 0 ? flog2(v.d.v23) + uint_x4<UInt32,uint64_t>::UINT_BITS*2
						: flog2(v.d.v01);
	}

	template<typename UInt32>
	bitcount_t trailingzeros(const uint_x4<UInt32,uint64_t>& v)
	{
		return v.d.v01 == 0 ? trailingzeros(v.d.v23) + uint_x4<UInt32,uint64_t>::UINT_BITS*2
							: trailingzeros(v.d.v01);
	}
	#endif

	template <typename UInt, typename UIntX2>
	std::pair< uint_x4<UInt,UIntX2>, uint_x4<UInt,UIntX2> >
		divmod(const uint_x4<UInt,UIntX2>& orig_dividend,
			const uint_x4<UInt,UIntX2>& divisor)
	{
		// If the dividend is less than the divisor, the answer is always zero.
		// This takes care of boundary cases like 0/x (which would otherwise be
		// problematic because we can't take the log of zero.  (The boundary case
		// of division by zero is undefined.)
		if (orig_dividend < divisor)
			return { uint_x4<UInt,UIntX2>(UIntX2(0)), orig_dividend };

		auto dividend = orig_dividend;

		auto log2_divisor  = flog2(divisor);
		auto log2_dividend = flog2(dividend);
		// assert(log2_dividend >= log2_divisor);
		bitcount_t logdiff = log2_dividend - log2_divisor;

		constexpr uint_x4<UInt,UIntX2> ONE(UIntX2(1));
		if (logdiff == 0)
			return { ONE, dividend - divisor };

		// Now we change the log difference to
		//  floor(log2(divisor)) - ceil(log2(dividend))
		// to ensure that we *underestimate* the result.
		logdiff -= 1;

		uint_x4<UInt,UIntX2> quotient(UIntX2(0));

		auto qfactor = ONE << logdiff;
		auto factor  = divisor << logdiff;

		do {
			dividend -= factor;
			quotient += qfactor;
			while (dividend < factor) {
				factor  >>= 1;
				qfactor >>= 1;
			}
		} while (dividend >= divisor);

		return { quotient, dividend };
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator/(const uint_x4<UInt,UIntX2>& dividend,
								const uint_x4<UInt,UIntX2>& divisor)
	{
		return divmod(dividend, divisor).first;
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator%(const uint_x4<UInt,UIntX2>& dividend,
								const uint_x4<UInt,UIntX2>& divisor)
	{
		return divmod(dividend, divisor).second;
	}


	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator*(const uint_x4<UInt,UIntX2>& a,
								const uint_x4<UInt,UIntX2>& b)
	{
		constexpr auto UINT_BITS = uint_x4<UInt,UIntX2>::UINT_BITS;
		uint_x4<UInt,UIntX2> r = {0U, 0U, 0U, 0U};
		bool carryin = false;
		bool carryout;
		UIntX2 a0b0 = UIntX2(a.w.v0) * UIntX2(b.w.v0);
		r.w.v0 = UInt(a0b0);
		r.w.v1 = UInt(a0b0 >> UINT_BITS);

		UIntX2 a1b0 = UIntX2(a.w.v1) * UIntX2(b.w.v0);
		r.w.v2 = UInt(a1b0 >> UINT_BITS);
		r.w.v1 = addwithcarry(r.w.v1, UInt(a1b0), carryin, &carryout);
		carryin = carryout;
		r.w.v2 = addwithcarry(r.w.v2, UInt(0U), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(0U), carryin, &carryout);

		UIntX2 a0b1 = UIntX2(a.w.v0) * UIntX2(b.w.v1);
		carryin = false;
		r.w.v2 = addwithcarry(r.w.v2, UInt(a0b1 >> UINT_BITS), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(0U), carryin, &carryout);

		carryin = false;
		r.w.v1 = addwithcarry(r.w.v1, UInt(a0b1), carryin, &carryout);
		carryin = carryout;
		r.w.v2 = addwithcarry(r.w.v2, UInt(0U), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(0U), carryin, &carryout);

		UIntX2 a1b1 = UIntX2(a.w.v1) * UIntX2(b.w.v1);
		carryin = false;
		r.w.v2 = addwithcarry(r.w.v2, UInt(a1b1), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(a1b1 >> UINT_BITS), carryin, &carryout);

		r.d.v23 += a.d.v01 * b.d.v23 + a.d.v23 * b.d.v01;

		return r;
	}

	
	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator*(const uint_x4<UInt,UIntX2>& a,
								UIntX2 b01)
	{
		constexpr auto UINT_BITS = uint_x4<UInt,UIntX2>::UINT_BITS;
		uint_x4<UInt,UIntX2> r = {0U, 0U, 0U, 0U};
		bool carryin = false;
		bool carryout;
		UIntX2 a0b0 = UIntX2(a.w.v0) * UIntX2(UInt(b01));
		r.w.v0 = UInt(a0b0);
		r.w.v1 = UInt(a0b0 >> UINT_BITS);

		UIntX2 a1b0 = UIntX2(a.w.v1) * UIntX2(UInt(b01));
		r.w.v2 = UInt(a1b0 >> UINT_BITS);
		r.w.v1 = addwithcarry(r.w.v1, UInt(a1b0), carryin, &carryout);
		carryin = carryout;
		r.w.v2 = addwithcarry(r.w.v2, UInt(0U), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(0U), carryin, &carryout);

		UIntX2 a0b1 = UIntX2(a.w.v0) * UIntX2(b01 >> UINT_BITS);
		carryin = false;
		r.w.v2 = addwithcarry(r.w.v2, UInt(a0b1 >> UINT_BITS), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(0U), carryin, &carryout);

		carryin = false;
		r.w.v1 = addwithcarry(r.w.v1, UInt(a0b1), carryin, &carryout);
		carryin = carryout;
		r.w.v2 = addwithcarry(r.w.v2, UInt(0U), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(0U), carryin, &carryout);

		UIntX2 a1b1 = UIntX2(a.w.v1) * UIntX2(b01 >> UINT_BITS);
		carryin = false;
		r.w.v2 = addwithcarry(r.w.v2, UInt(a1b1), carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(r.w.v3, UInt(a1b1 >> UINT_BITS), carryin, &carryout);

		r.d.v23 += a.d.v23 * b01;

		return r;
	}

	#if PCG_64BIT_SPECIALIZATIONS
	#if defined(_MSC_VER)
	#pragma intrinsic(_umul128)
	#endif

	#if defined(_MSC_VER) || __SIZEOF_INT128__
	template <typename UInt32>
	uint_x4<UInt32,uint64_t> operator*(const uint_x4<UInt32,uint64_t>& a,
					const uint_x4<UInt32,uint64_t>& b)
	{
	#if defined(_MSC_VER)
		uint64_t hi;
		uint64_t lo = _umul128(a.d.v01, b.d.v01, &hi);
	#else
		__uint128_t r = __uint128_t(a.d.v01) * __uint128_t(b.d.v01);
		uint64_t lo = uint64_t(r);
		uint64_t hi = r >> 64;
	#endif
		hi += a.d.v23 * b.d.v01 + a.d.v01 * b.d.v23;
		return {hi, lo};
	}
	#endif
	#endif


	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator+(const uint_x4<UInt,UIntX2>& a,
								const uint_x4<UInt,UIntX2>& b)
	{
		uint_x4<UInt,UIntX2> r = {0U, 0U, 0U, 0U};

		bool carryin = false;
		bool carryout;
		r.w.v0 = addwithcarry(a.w.v0, b.w.v0, carryin, &carryout);
		carryin = carryout;
		r.w.v1 = addwithcarry(a.w.v1, b.w.v1, carryin, &carryout);
		carryin = carryout;
		r.w.v2 = addwithcarry(a.w.v2, b.w.v2, carryin, &carryout);
		carryin = carryout;
		r.w.v3 = addwithcarry(a.w.v3, b.w.v3, carryin, &carryout);

		return r;
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator-(const uint_x4<UInt,UIntX2>& a,
								const uint_x4<UInt,UIntX2>& b)
	{
		uint_x4<UInt,UIntX2> r = {0U, 0U, 0U, 0U};

		bool carryin = false;
		bool carryout;
		r.w.v0 = subwithcarry(a.w.v0, b.w.v0, carryin, &carryout);
		carryin = carryout;
		r.w.v1 = subwithcarry(a.w.v1, b.w.v1, carryin, &carryout);
		carryin = carryout;
		r.w.v2 = subwithcarry(a.w.v2, b.w.v2, carryin, &carryout);
		carryin = carryout;
		r.w.v3 = subwithcarry(a.w.v3, b.w.v3, carryin, &carryout);

		return r;
	}

	#if PCG_64BIT_SPECIALIZATIONS
	template <typename UInt32>
	uint_x4<UInt32,uint64_t> operator+(const uint_x4<UInt32,uint64_t>& a,
					const uint_x4<UInt32,uint64_t>& b)
	{
		uint_x4<UInt32,uint64_t> r = {uint64_t(0u), uint64_t(0u)};

		bool carryin = false;
		bool carryout;
		r.d.v01 = addwithcarry(a.d.v01, b.d.v01, carryin, &carryout);
		carryin = carryout;
		r.d.v23 = addwithcarry(a.d.v23, b.d.v23, carryin, &carryout);

		return r;
	}

	template <typename UInt32>
	uint_x4<UInt32,uint64_t> operator-(const uint_x4<UInt32,uint64_t>& a,
					const uint_x4<UInt32,uint64_t>& b)
	{
		uint_x4<UInt32,uint64_t> r = {uint64_t(0u), uint64_t(0u)};

		bool carryin = false;
		bool carryout;
		r.d.v01 = subwithcarry(a.d.v01, b.d.v01, carryin, &carryout);
		carryin = carryout;
		r.d.v23 = subwithcarry(a.d.v23, b.d.v23, carryin, &carryout);

		return r;
	}
	#endif

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator&(const uint_x4<UInt,UIntX2>& a,
								const uint_x4<UInt,UIntX2>& b)
	{
		return uint_x4<UInt,UIntX2>(a.d.v23 & b.d.v23, a.d.v01 & b.d.v01);
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator|(const uint_x4<UInt,UIntX2>& a,
								const uint_x4<UInt,UIntX2>& b)
	{
		return uint_x4<UInt,UIntX2>(a.d.v23 | b.d.v23, a.d.v01 | b.d.v01);
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator^(const uint_x4<UInt,UIntX2>& a,
								const uint_x4<UInt,UIntX2>& b)
	{
		return uint_x4<UInt,UIntX2>(a.d.v23 ^ b.d.v23, a.d.v01 ^ b.d.v01);
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator~(const uint_x4<UInt,UIntX2>& v)
	{
		return uint_x4<UInt,UIntX2>(~v.d.v23, ~v.d.v01);
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator-(const uint_x4<UInt,UIntX2>& v)
	{
		return uint_x4<UInt,UIntX2>(0UL,0UL) - v;
	}

	template <typename UInt, typename UIntX2>
	bool operator==(const uint_x4<UInt,UIntX2>& a, const uint_x4<UInt,UIntX2>& b)
	{
		return (a.d.v01 == b.d.v01) && (a.d.v23 == b.d.v23);
	}

	template <typename UInt, typename UIntX2>
	bool operator!=(const uint_x4<UInt,UIntX2>& a, const uint_x4<UInt,UIntX2>& b)
	{
		return !operator==(a,b);
	}


	template <typename UInt, typename UIntX2>
	bool operator<(const uint_x4<UInt,UIntX2>& a, const uint_x4<UInt,UIntX2>& b)
	{
		return (a.d.v23 < b.d.v23)
			|| ((a.d.v23 == b.d.v23) && (a.d.v01 < b.d.v01));
	}

	template <typename UInt, typename UIntX2>
	bool operator>(const uint_x4<UInt,UIntX2>& a, const uint_x4<UInt,UIntX2>& b)
	{
		return operator<(b,a);
	}

	template <typename UInt, typename UIntX2>
	bool operator<=(const uint_x4<UInt,UIntX2>& a, const uint_x4<UInt,UIntX2>& b)
	{
		return !(operator<(b,a));
	}

	template <typename UInt, typename UIntX2>
	bool operator>=(const uint_x4<UInt,UIntX2>& a, const uint_x4<UInt,UIntX2>& b)
	{
		return !(operator<(a,b));
	}



	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator<<(const uint_x4<UInt,UIntX2>& v,
									const bitcount_t shift)
	{
		uint_x4<UInt,UIntX2> r = {0U, 0U, 0U, 0U};
		const bitcount_t bits    = uint_x4<UInt,UIntX2>::UINT_BITS;
		const bitcount_t bitmask = bits - 1;
		const bitcount_t shiftdiv = shift / bits;
		const bitcount_t shiftmod = shift & bitmask;

		if (shiftmod) {
			UInt carryover = 0;
	#if PCG_LITTLE_ENDIAN
			for (uint8_t out = shiftdiv, in = 0; out < 4; ++out, ++in) {
	#else
			for (uint8_t out = 4-shiftdiv, in = 4; out != 0; /* dec in loop */) {
				--out, --in;
	#endif
				r.wa[out] = (v.wa[in] << shiftmod) | carryover;
				carryover = (v.wa[in] >> (bits - shiftmod));
			}
		} else {
	#if PCG_LITTLE_ENDIAN
			for (uint8_t out = shiftdiv, in = 0; out < 4; ++out, ++in) {
	#else
			for (uint8_t out = 4-shiftdiv, in = 4; out != 0; /* dec in loop */) {
				--out, --in;
	#endif
				r.wa[out] = v.wa[in];
			}
		}

		return r;
	}

	template <typename UInt, typename UIntX2>
	uint_x4<UInt,UIntX2> operator>>(const uint_x4<UInt,UIntX2>& v,
									const bitcount_t shift)
	{
		uint_x4<UInt,UIntX2> r = {0U, 0U, 0U, 0U};
		const bitcount_t bits    = uint_x4<UInt,UIntX2>::UINT_BITS;
		const bitcount_t bitmask = bits - 1;
		const bitcount_t shiftdiv = shift / bits;
		const bitcount_t shiftmod = shift & bitmask;

		if (shiftmod) {
			UInt carryover = 0;
	#if PCG_LITTLE_ENDIAN
			for (uint8_t out = 4-shiftdiv, in = 4; out != 0; /* dec in loop */) {
				--out, --in;
	#else
			for (uint8_t out = shiftdiv, in = 0; out < 4; ++out, ++in) {
	#endif
				r.wa[out] = (v.wa[in] >> shiftmod) | carryover;
				carryover = (v.wa[in] << (bits - shiftmod));
			}
		} else {
	#if PCG_LITTLE_ENDIAN
			for (uint8_t out = 4-shiftdiv, in = 4; out != 0; /* dec in loop */) {
				--out, --in;
	#else
			for (uint8_t out = shiftdiv, in = 0; out < 4; ++out, ++in) {
	#endif
				r.wa[out] = v.wa[in];
			}
		}

		return r;
	}

	#if PCG_64BIT_SPECIALIZATIONS
	template <typename UInt32>
	uint_x4<UInt32,uint64_t> operator<<(const uint_x4<UInt32,uint64_t>& v,
						const bitcount_t shift)
	{
		constexpr bitcount_t bits2   = uint_x4<UInt32,uint64_t>::UINT_BITS * 2;
		
		if (shift >= bits2) {
			return {v.d.v01 << (shift-bits2), uint64_t(0u)};
		} else {
			return {shift ? (v.d.v23 << shift) | (v.d.v01 >> (bits2-shift)) 
						: v.d.v23,
					v.d.v01 << shift};
		}
	}

	template <typename UInt32>
	uint_x4<UInt32,uint64_t> operator>>(const uint_x4<UInt32,uint64_t>& v,
						const bitcount_t shift)
	{
		constexpr bitcount_t bits2   = uint_x4<UInt32,uint64_t>::UINT_BITS * 2;
		
		if (shift >= bits2) {
			return {uint64_t(0u), v.d.v23 >> (shift-bits2)};
		} else {
			return {v.d.v23 >> shift,
					shift ? (v.d.v01 >> shift) | (v.d.v23 << (bits2-shift))
						: v.d.v01};
		}
	}
	#endif

	} // namespace pcg_extras

	#endif // PCG_UINT128_HPP_INCLUDED

    namespace pcg_extras {
        typedef pcg_extras::uint_x4<uint32_t,uint64_t> pcg128_t;
    }
    #define PCG_128BIT_CONSTANT(high,low) \
            pcg_extras::pcg128_t(high,low)
    #define PCG_EMULATED_128BIT_MATH 1
#endif


namespace pcg_extras {

/*
 * We often need to represent a "number of bits".  When used normally, these
 * numbers are never greater than 128, so an unsigned char is plenty.
 * If you're using a nonstandard generator of a larger size, you can set
 * PCG_BITCOUNT_T to have it define it as a larger size.  (Some compilers
 * might produce faster code if you set it to an unsigned int.)
 */

#ifndef PCG_BITCOUNT_T
    typedef uint8_t bitcount_t;
#else
    typedef PCG_BITCOUNT_T bitcount_t;
#endif

/*
 * C++ requires us to be able to serialize RNG state by printing or reading
 * it from a stream.  Because we use 128-bit ints, we also need to be able
 * ot print them, so here is code to do so.
 *
 * This code provides enough functionality to print 128-bit ints in decimal
 * and zero-padded in hex.  It's not a full-featured implementation.
 */

template <typename CharT, typename Traits>
std::basic_ostream<CharT,Traits>&
operator<<(std::basic_ostream<CharT,Traits>& out, pcg128_t value)
{
    auto desired_base = out.flags() & out.basefield;
    bool want_hex = desired_base == out.hex;

    if (want_hex) {
        uint64_t highpart = uint64_t(value >> 64);
        uint64_t lowpart  = uint64_t(value);
        auto desired_width = out.width();
        if (desired_width > 16) {
            out.width(desired_width - 16);
        }
        if (highpart != 0 || desired_width > 16)
            out << highpart;
        CharT oldfill = '\0';
        if (highpart != 0) {
            out.width(16);
            oldfill = out.fill('0');
        }
        auto oldflags = out.setf(decltype(desired_base){}, out.showbase);
        out << lowpart;
        out.setf(oldflags);
        if (highpart != 0) {
            out.fill(oldfill);
        }
        return out;
    }
    constexpr size_t MAX_CHARS_128BIT = 40;

    char buffer[MAX_CHARS_128BIT];
    char* pos = buffer+sizeof(buffer);
    *(--pos) = '\0';
    constexpr auto BASE = pcg128_t(10ULL);
    do {
        auto div = value / BASE;
        auto mod = uint32_t(value - (div * BASE));
        *(--pos) = '0' + char(mod);
        value = div;
    } while(value != pcg128_t(0ULL));
    return out << pos;
}

template <typename CharT, typename Traits>
std::basic_istream<CharT,Traits>&
operator>>(std::basic_istream<CharT,Traits>& in, pcg128_t& value)
{
    typename std::basic_istream<CharT,Traits>::sentry s(in);

    if (!s)
         return in;

    constexpr auto BASE = pcg128_t(10ULL);
    pcg128_t current(0ULL);
    bool did_nothing = true;
    bool overflow = false;
    for(;;) {
        CharT wide_ch = in.get();
        if (!in.good())
            break;
        auto ch = in.narrow(wide_ch, '\0');
        if (ch < '0' || ch > '9') {
            in.unget();
            break;
        }
        did_nothing = false;
        pcg128_t digit(uint32_t(ch - '0'));
        pcg128_t timesbase = current*BASE;
        overflow = overflow || timesbase < current;
        current = timesbase + digit;
        overflow = overflow || current < digit;
    }

    if (did_nothing || overflow) {
        in.setstate(std::ios::failbit);
        if (overflow)
            current = ~pcg128_t(0ULL);
    }

    value = current;

    return in;
}

/*
 * Likewise, if people use tiny rngs, we'll be serializing uint8_t.
 * If we just used the provided IO operators, they'd read/write chars,
 * not ints, so we need to define our own.  We *can* redefine this operator
 * here because we're in our own namespace.
 */

template <typename CharT, typename Traits>
std::basic_ostream<CharT,Traits>&
operator<<(std::basic_ostream<CharT,Traits>&out, uint8_t value)
{
    return out << uint32_t(value);
}

template <typename CharT, typename Traits>
std::basic_istream<CharT,Traits>&
operator>>(std::basic_istream<CharT,Traits>& in, uint8_t& target)
{
    uint32_t value = 0xdecea5edU;
    in >> value;
    if (!in && value == 0xdecea5edU)
        return in;
    if (value > uint8_t(~0)) {
        in.setstate(std::ios::failbit);
        value = ~0U;
    }
    target = uint8_t(value);
    return in;
}

/* Unfortunately, the above functions don't get found in preference to the
 * built in ones, so we create some more specific overloads that will.
 * Ugh.
 */

inline std::ostream& operator<<(std::ostream& out, uint8_t value)
{
    return pcg_extras::operator<< <char>(out, value);
}

inline std::istream& operator>>(std::istream& in, uint8_t& value)
{
    return pcg_extras::operator>> <char>(in, value);
}



/*
 * Useful bitwise operations.
 */

/*
 * XorShifts are invertable, but they are someting of a pain to invert.
 * This function backs them out.  It's used by the whacky "inside out"
 * generator defined later.
 */

template <typename itype>
inline itype unxorshift(itype x, bitcount_t bits, bitcount_t shift)
{
    if (2*shift >= bits) {
        return x ^ (x >> shift);
    }
    itype lowmask1 = (itype(1U) << (bits - shift*2)) - 1;
    itype highmask1 = ~lowmask1;
    itype top1 = x;
    itype bottom1 = x & lowmask1;
    top1 ^= top1 >> shift;
    top1 &= highmask1;
    x = top1 | bottom1;
    itype lowmask2 = (itype(1U) << (bits - shift)) - 1;
    itype bottom2 = x & lowmask2;
    bottom2 = unxorshift(bottom2, bits - shift, shift);
    bottom2 &= lowmask1;
    return top1 | bottom2;
}

/*
 * Rotate left and right.
 *
 * In ideal world, compilers would spot idiomatic rotate code and convert it
 * to a rotate instruction.  Of course, opinions vary on what the correct
 * idiom is and how to spot it.  For clang, sometimes it generates better
 * (but still crappy) code if you define PCG_USE_ZEROCHECK_ROTATE_IDIOM.
 */

template <typename itype>
inline itype rotl(itype value, bitcount_t rot)
{
    constexpr bitcount_t bits = sizeof(itype) * 8;
    constexpr bitcount_t mask = bits - 1;
#if PCG_USE_ZEROCHECK_ROTATE_IDIOM
    return rot ? (value << rot) | (value >> (bits - rot)) : value;
#else
    return (value << rot) | (value >> ((- rot) & mask));
#endif
}

template <typename itype>
inline itype rotr(itype value, bitcount_t rot)
{
    constexpr bitcount_t bits = sizeof(itype) * 8;
    constexpr bitcount_t mask = bits - 1;
#if PCG_USE_ZEROCHECK_ROTATE_IDIOM
    return rot ? (value >> rot) | (value << (bits - rot)) : value;
#else
    return (value >> rot) | (value << ((- rot) & mask));
#endif
}

/* Unfortunately, both Clang and GCC sometimes perform poorly when it comes
 * to properly recognizing idiomatic rotate code, so for we also provide
 * assembler directives (enabled with PCG_USE_INLINE_ASM).  Boo, hiss.
 * (I hope that these compilers get better so that this code can die.)
 *
 * These overloads will be preferred over the general template code above.
 */
#if PCG_USE_INLINE_ASM && __GNUC__ && (__x86_64__  || __i386__)

inline uint8_t rotr(uint8_t value, bitcount_t rot)
{
    asm ("rorb   %%cl, %0" : "=r" (value) : "0" (value), "c" (rot));
    return value;
}

inline uint16_t rotr(uint16_t value, bitcount_t rot)
{
    asm ("rorw   %%cl, %0" : "=r" (value) : "0" (value), "c" (rot));
    return value;
}

inline uint32_t rotr(uint32_t value, bitcount_t rot)
{
    asm ("rorl   %%cl, %0" : "=r" (value) : "0" (value), "c" (rot));
    return value;
}

#if __x86_64__
inline uint64_t rotr(uint64_t value, bitcount_t rot)
{
    asm ("rorq   %%cl, %0" : "=r" (value) : "0" (value), "c" (rot));
    return value;
}
#endif // __x86_64__

#elif defined(_MSC_VER)
  // Use MSVC++ bit rotation intrinsics

#pragma intrinsic(_rotr, _rotr64, _rotr8, _rotr16)

inline uint8_t rotr(uint8_t value, bitcount_t rot)
{
    return _rotr8(value, rot);
}

inline uint16_t rotr(uint16_t value, bitcount_t rot)
{
    return _rotr16(value, rot);
}

inline uint32_t rotr(uint32_t value, bitcount_t rot)
{
    return _rotr(value, rot);
}

inline uint64_t rotr(uint64_t value, bitcount_t rot)
{
    return _rotr64(value, rot);
}

#endif // PCG_USE_INLINE_ASM


/*
 * The C++ SeedSeq concept (modelled by seed_seq) can fill an array of
 * 32-bit integers with seed data, but sometimes we want to produce
 * larger or smaller integers.
 *
 * The following code handles this annoyance.
 *
 * uneven_copy will copy an array of 32-bit ints to an array of larger or
 * smaller ints (actually, the code is general it only needing forward
 * iterators).  The copy is identical to the one that would be performed if
 * we just did memcpy on a standard little-endian machine, but works
 * regardless of the endian of the machine (or the weirdness of the ints
 * involved).
 *
 * generate_to initializes an array of integers using a SeedSeq
 * object.  It is given the size as a static constant at compile time and
 * tries to avoid memory allocation.  If we're filling in 32-bit constants
 * we just do it directly.  If we need a separate buffer and it's small,
 * we allocate it on the stack.  Otherwise, we fall back to heap allocation.
 * Ugh.
 *
 * generate_one produces a single value of some integral type using a
 * SeedSeq object.
 */

 /* uneven_copy helper, case where destination ints are less than 32 bit. */

template<class SrcIter, class DestIter>
SrcIter uneven_copy_impl(
    SrcIter src_first, DestIter dest_first, DestIter dest_last,
    std::true_type)
{
    typedef typename std::iterator_traits<SrcIter>::value_type  src_t;
    typedef typename std::iterator_traits<DestIter>::value_type dest_t;

    constexpr bitcount_t SRC_SIZE  = sizeof(src_t);
    constexpr bitcount_t DEST_SIZE = sizeof(dest_t);
    constexpr bitcount_t DEST_BITS = DEST_SIZE * 8;
    constexpr bitcount_t SCALE     = SRC_SIZE / DEST_SIZE;

    size_t count = 0;
    src_t value = 0;

    while (dest_first != dest_last) {
        if ((count++ % SCALE) == 0)
            value = *src_first++;       // Get more bits
        else
            value >>= DEST_BITS;        // Move down bits

        *dest_first++ = dest_t(value);  // Truncates, ignores high bits.
    }
    return src_first;
}

 /* uneven_copy helper, case where destination ints are more than 32 bit. */

template<class SrcIter, class DestIter>
SrcIter uneven_copy_impl(
    SrcIter src_first, DestIter dest_first, DestIter dest_last,
    std::false_type)
{
    typedef typename std::iterator_traits<SrcIter>::value_type  src_t;
    typedef typename std::iterator_traits<DestIter>::value_type dest_t;

    constexpr auto SRC_SIZE  = sizeof(src_t);
    constexpr auto SRC_BITS  = SRC_SIZE * 8;
    constexpr auto DEST_SIZE = sizeof(dest_t);
    constexpr auto SCALE     = (DEST_SIZE+SRC_SIZE-1) / SRC_SIZE;

    while (dest_first != dest_last) {
        dest_t value(0UL);
        unsigned int shift = 0;

        for (size_t i = 0; i < SCALE; ++i) {
            value |= dest_t(*src_first++) << shift;
            shift += SRC_BITS;
        }

        *dest_first++ = value;
    }
    return src_first;
}

/* uneven_copy, call the right code for larger vs. smaller */

template<class SrcIter, class DestIter>
inline SrcIter uneven_copy(SrcIter src_first,
                           DestIter dest_first, DestIter dest_last)
{
    typedef typename std::iterator_traits<SrcIter>::value_type  src_t;
    typedef typename std::iterator_traits<DestIter>::value_type dest_t;

    constexpr bool DEST_IS_SMALLER = sizeof(dest_t) < sizeof(src_t);

    return uneven_copy_impl(src_first, dest_first, dest_last,
                            std::integral_constant<bool, DEST_IS_SMALLER>{});
}

/* generate_to, fill in a fixed-size array of integral type using a SeedSeq
 * (actually works for any random-access iterator)
 */

template <size_t size, typename SeedSeq, typename DestIter>
inline void generate_to_impl(SeedSeq&& generator, DestIter dest,
                             std::true_type)
{
    generator.generate(dest, dest+size);
}

template <size_t size, typename SeedSeq, typename DestIter>
void generate_to_impl(SeedSeq&& generator, DestIter dest,
                      std::false_type)
{
    typedef typename std::iterator_traits<DestIter>::value_type dest_t;
    constexpr auto DEST_SIZE = sizeof(dest_t);
    constexpr auto GEN_SIZE  = sizeof(uint32_t);

    constexpr bool GEN_IS_SMALLER = GEN_SIZE < DEST_SIZE;
    constexpr size_t FROM_ELEMS =
        GEN_IS_SMALLER
            ? size * ((DEST_SIZE+GEN_SIZE-1) / GEN_SIZE)
            : (size + (GEN_SIZE / DEST_SIZE) - 1)
                / ((GEN_SIZE / DEST_SIZE) + GEN_IS_SMALLER);
                        //  this odd code ^^^^^^^^^^^^^^^^^ is work-around for
                        //  a bug: http://llvm.org/bugs/show_bug.cgi?id=21287

    if (FROM_ELEMS <= 1024) {
        uint32_t buffer[FROM_ELEMS];
        generator.generate(buffer, buffer+FROM_ELEMS);
        uneven_copy(buffer, dest, dest+size);
    } else {
        uint32_t* buffer = static_cast<uint32_t*>(malloc(GEN_SIZE * FROM_ELEMS));
        generator.generate(buffer, buffer+FROM_ELEMS);
        uneven_copy(buffer, dest, dest+size);
        free(static_cast<void*>(buffer));
    }
}

template <size_t size, typename SeedSeq, typename DestIter>
inline void generate_to(SeedSeq&& generator, DestIter dest)
{
    typedef typename std::iterator_traits<DestIter>::value_type dest_t;
    constexpr bool IS_32BIT = sizeof(dest_t) == sizeof(uint32_t);

    generate_to_impl<size>(std::forward<SeedSeq>(generator), dest,
                           std::integral_constant<bool, IS_32BIT>{});
}

/* generate_one, produce a value of integral type using a SeedSeq
 * (optionally, we can have it produce more than one and pick which one
 * we want)
 */

template <typename UInt, size_t i = 0UL, size_t N = i+1UL, typename SeedSeq>
inline UInt generate_one(SeedSeq&& generator)
{
    UInt result[N];
    generate_to<N>(std::forward<SeedSeq>(generator), result);
    return result[i];
}

template <typename RngType>
auto bounded_rand(RngType& rng, typename RngType::result_type upper_bound)
        -> typename RngType::result_type
{
    typedef typename RngType::result_type rtype;
    rtype threshold = (RngType::max() - RngType::min() + rtype(1) - upper_bound)
                    % upper_bound;
    for (;;) {
        rtype r = rng() - RngType::min();
        if (r >= threshold)
            return r % upper_bound;
    }
}

template <typename Iter, typename RandType>
void shuffle(Iter from, Iter to, RandType&& rng)
{
    typedef typename std::iterator_traits<Iter>::difference_type delta_t;
    typedef typename std::remove_reference<RandType>::type::result_type result_t;
    auto count = to - from;
    while (count > 1) {
        delta_t chosen = delta_t(bounded_rand(rng, result_t(count)));
        --count;
        --to;
        using std::swap;
        swap(*(from + chosen), *to);
    }
}

/*
 * Although std::seed_seq is useful, it isn't everything.  Often we want to
 * initialize a random-number generator some other way, such as from a random
 * device.
 *
 * Technically, it does not meet the requirements of a SeedSequence because
 * it lacks some of the rarely-used member functions (some of which would
 * be impossible to provide).  However the C++ standard is quite specific
 * that actual engines only called the generate method, so it ought not to be
 * a problem in practice.
 */

template <typename RngType>
class seed_seq_from {
private:
    RngType rng_;

    typedef uint_least32_t result_type;

public:
    template<typename... Args>
    seed_seq_from(Args&&... args) :
        rng_(std::forward<Args>(args)...)
    {
        // Nothing (else) to do...
    }

    template<typename Iter>
    void generate(Iter start, Iter finish)
    {
        for (auto i = start; i != finish; ++i)
            *i = result_type(rng_());
    }

    constexpr size_t size() const
    {
        return (sizeof(typename RngType::result_type) > sizeof(result_type)
                && RngType::max() > ~size_t(0UL))
             ? ~size_t(0UL)
             : size_t(RngType::max());
    }
};

/*
 * Sometimes you might want a distinct seed based on when the program
 * was compiled.  That way, a particular instance of the program will
 * behave the same way, but when recompiled it'll produce a different
 * value.
 */

template <typename IntType>
struct static_arbitrary_seed {
private:
    static constexpr IntType fnv(IntType hash, const char* pos) {
        return *pos == '\0'
             ? hash
             : fnv((hash * IntType(16777619U)) ^ *pos, (pos+1));
    }

public:
    static constexpr IntType value = fnv(IntType(2166136261U ^ sizeof(IntType)),
                        __DATE__ __TIME__ __FILE__);
};

// Sometimes, when debugging or testing, it's handy to be able print the name
// of a (in human-readable form).  This code allows the idiom:
//
//      cout << printable_typename<my_foo_type_t>()
//
// to print out my_foo_type_t (or its concrete type if it is a synonym)

#if __cpp_rtti || __GXX_RTTI

template <typename T>
struct printable_typename {};

template <typename T>
std::ostream& operator<<(std::ostream& out, printable_typename<T>) {
    const char *implementation_typename = typeid(T).name();
#ifdef __GNUC__
    int status;
    char* pretty_name =
        abi::__cxa_demangle(implementation_typename, nullptr, nullptr, &status);
    if (status == 0)
        out << pretty_name;
    free(static_cast<void*>(pretty_name));
    if (status == 0)
        return out;
#endif
    out << implementation_typename;
    return out;
}

#endif  // __cpp_rtti || __GXX_RTTI

} // namespace pcg_extras

#endif // PCG_EXTRAS_HPP_INCLUDED

namespace pcg_detail {

using namespace pcg_extras;

/*
 * The LCG generators need some constants to function.  This code lets you
 * look up the constant by *type*.  For example
 *
 *      default_multiplier<uint32_t>::multiplier()
 *
 * gives you the default multipler for 32-bit integers.  We use the name
 * of the constant and not a generic word like value to allow these classes
 * to be used as mixins.
 */

template <typename T>
struct default_multiplier {
    // Not defined for an arbitrary type
};

template <typename T>
struct default_increment {
    // Not defined for an arbitrary type
};

#define PCG_DEFINE_CONSTANT(type, what, kind, constant) \
        template <>                                     \
        struct what ## _ ## kind<type> {                \
            static constexpr type kind() {              \
                return constant;                        \
            }                                           \
        };

PCG_DEFINE_CONSTANT(uint8_t,  default, multiplier, 141U)
PCG_DEFINE_CONSTANT(uint8_t,  default, increment,  77U)

PCG_DEFINE_CONSTANT(uint16_t, default, multiplier, 12829U)
PCG_DEFINE_CONSTANT(uint16_t, default, increment,  47989U)

PCG_DEFINE_CONSTANT(uint32_t, default, multiplier, 747796405U)
PCG_DEFINE_CONSTANT(uint32_t, default, increment,  2891336453U)

PCG_DEFINE_CONSTANT(uint64_t, default, multiplier, 6364136223846793005ULL)
PCG_DEFINE_CONSTANT(uint64_t, default, increment,  1442695040888963407ULL)

PCG_DEFINE_CONSTANT(pcg128_t, default, multiplier,
        PCG_128BIT_CONSTANT(2549297995355413924ULL,4865540595714422341ULL))
PCG_DEFINE_CONSTANT(pcg128_t, default, increment,
        PCG_128BIT_CONSTANT(6364136223846793005ULL,1442695040888963407ULL))

/* Alternative (cheaper) multipliers for 128-bit */

template <typename T>
struct cheap_multiplier : public default_multiplier<T> {
    // For most types just use the default.
};

template <>
struct cheap_multiplier<pcg128_t> {
    static constexpr uint64_t multiplier() {
        return 0xda942042e4dd58b5ULL;
    }
};


/*
 * Each PCG generator is available in four variants, based on how it applies
 * the additive constant for its underlying LCG; the variations are:
 *
 *     single stream   - all instances use the same fixed constant, thus
 *                       the RNG always somewhere in same sequence
 *     mcg             - adds zero, resulting in a single stream and reduced
 *                       period
 *     specific stream - the constant can be changed at any time, selecting
 *                       a different random sequence
 *     unique stream   - the constant is based on the memory address of the
 *                       object, thus every RNG has its own unique sequence
 *
 * This variation is provided though mixin classes which define a function
 * value called increment() that returns the nesessary additive constant.
 */



/*
 * unique stream
 */


template <typename itype>
class unique_stream {
protected:
    static constexpr bool is_mcg = false;

    // Is never called, but is provided for symmetry with specific_stream
    void set_stream(...)
    {
        abort();
    }

public:
    typedef itype state_type;

    constexpr itype increment() const {
        return itype(reinterpret_cast<uintptr_t>(this) | 1);
    }

    constexpr itype stream() const
    {
         return increment() >> 1;
    }

    static constexpr bool can_specify_stream = false;

    static constexpr size_t streams_pow2()
    {
        return (sizeof(itype) < sizeof(size_t) ? sizeof(itype)
                                               : sizeof(size_t))*8 - 1u;
    }

protected:
    constexpr unique_stream() = default;
};


/*
 * no stream (mcg)
 */

template <typename itype>
class no_stream {
protected:
    static constexpr bool is_mcg = true;

    // Is never called, but is provided for symmetry with specific_stream
    void set_stream(...)
    {
        abort();
    }

public:
    typedef itype state_type;

    static constexpr itype increment() {
        return 0;
    }

    static constexpr bool can_specify_stream = false;

    static constexpr size_t streams_pow2()
    {
        return 0u;
    }

protected:
    constexpr no_stream() = default;
};


/*
 * single stream/sequence (oneseq)
 */

template <typename itype>
class oneseq_stream : public default_increment<itype> {
protected:
    static constexpr bool is_mcg = false;

    // Is never called, but is provided for symmetry with specific_stream
    void set_stream(...)
    {
        abort();
    }

public:
    typedef itype state_type;

    static constexpr itype stream()
    {
         return default_increment<itype>::increment() >> 1;
    }

    static constexpr bool can_specify_stream = false;

    static constexpr size_t streams_pow2()
    {
        return 0u;
    }

protected:
    constexpr oneseq_stream() = default;
};


/*
 * specific stream
 */

template <typename itype>
class specific_stream {
protected:
    static constexpr bool is_mcg = false;

    itype inc_ = default_increment<itype>::increment();

public:
    typedef itype state_type;
    typedef itype stream_state;

    constexpr itype increment() const {
        return inc_;
    }

    itype stream()
    {
         return inc_ >> 1;
    }

    void set_stream(itype specific_seq)
    {
         inc_ = (specific_seq << 1) | 1;
    }

    static constexpr bool can_specify_stream = true;

    static constexpr size_t streams_pow2()
    {
        return (sizeof(itype)*8) - 1u;
    }

protected:
    specific_stream() = default;

    specific_stream(itype specific_seq)
        : inc_(itype(specific_seq << 1) | itype(1U))
    {
        // Nothing (else) to do.
    }
};


/*
 * This is where it all comes together.  This function joins together three
 * mixin classes which define
 *    - the LCG additive constant (the stream)
 *    - the LCG multiplier
 *    - the output function
 * in addition, we specify the type of the LCG state, and the result type,
 * and whether to use the pre-advance version of the state for the output
 * (increasing instruction-level parallelism) or the post-advance version
 * (reducing register pressure).
 *
 * Given the high level of parameterization, the code has to use some
 * template-metaprogramming tricks to handle some of the suble variations
 * involved.
 */

template <typename xtype, typename itype,
          typename output_mixin,
          bool output_previous = true,
          typename stream_mixin = oneseq_stream<itype>,
          typename multiplier_mixin = default_multiplier<itype> >
class engine : protected output_mixin,
               public stream_mixin,
               protected multiplier_mixin {
protected:
    itype state_;

    struct can_specify_stream_tag {};
    struct no_specifiable_stream_tag {};

    using stream_mixin::increment;
    using multiplier_mixin::multiplier;

public:
    typedef xtype result_type;
    typedef itype state_type;

    static constexpr size_t period_pow2()
    {
        return sizeof(state_type)*8 - 2*stream_mixin::is_mcg;
    }

    // It would be nice to use std::numeric_limits for these, but
    // we can't be sure that it'd be defined for the 128-bit types.

    static constexpr result_type min()
    {
        return result_type(0UL);
    }

    static constexpr result_type max()
    {
        return result_type(~result_type(0UL));
    }

protected:
    itype bump(itype state)
    {
        return state * multiplier() + increment();
    }

    itype base_generate()
    {
        return state_ = bump(state_);
    }

    itype base_generate0()
    {
        itype old_state = state_;
        state_ = bump(state_);
        return old_state;
    }

public:
    result_type operator()()
    {
        if (output_previous)
            return this->output(base_generate0());
        else
            return this->output(base_generate());
    }

    result_type operator()(result_type upper_bound)
    {
        return bounded_rand(*this, upper_bound);
    }

protected:
    static itype advance(itype state, itype delta,
                         itype cur_mult, itype cur_plus);

    static itype distance(itype cur_state, itype newstate, itype cur_mult,
                          itype cur_plus, itype mask = ~itype(0U));

    itype distance(itype newstate, itype mask = itype(~itype(0U))) const
    {
        return distance(state_, newstate, multiplier(), increment(), mask);
    }

public:
    void advance(itype delta)
    {
        state_ = advance(state_, delta, this->multiplier(), this->increment());
    }

    void backstep(itype delta)
    {
        advance(-delta);
    }

    void discard(itype delta)
    {
        advance(delta);
    }

    bool wrapped()
    {
        if (stream_mixin::is_mcg) {
            // For MCGs, the low order two bits never change. In this
            // implementation, we keep them fixed at 3 to make this test
            // easier.
            return state_ == 3;
        } else {
            return state_ == 0;
        }
    }

    engine(itype state = itype(0xcafef00dd15ea5e5ULL))
        : state_(this->is_mcg ? state|state_type(3U)
                              : bump(state + this->increment()))
    {
        // Nothing else to do.
    }

    // This function may or may not exist.  It thus has to be a template
    // to use SFINAE; users don't have to worry about its template-ness.

    template <typename sm = stream_mixin>
    engine(itype state, typename sm::stream_state stream_seed)
        : stream_mixin(stream_seed),
          state_(this->is_mcg ? state|state_type(3U)
                              : bump(state + this->increment()))
    {
        // Nothing else to do.
    }

    template<typename SeedSeq>
    engine(SeedSeq&& seedSeq, typename std::enable_if<
                  !stream_mixin::can_specify_stream
               && !std::is_convertible<SeedSeq, itype>::value
               && !std::is_convertible<SeedSeq, engine>::value,
               no_specifiable_stream_tag>::type = {})
        : engine(generate_one<itype>(std::forward<SeedSeq>(seedSeq)))
    {
        // Nothing else to do.
    }

    template<typename SeedSeq>
    engine(SeedSeq&& seedSeq, typename std::enable_if<
                   stream_mixin::can_specify_stream
               && !std::is_convertible<SeedSeq, itype>::value
               && !std::is_convertible<SeedSeq, engine>::value,
        can_specify_stream_tag>::type = {})
    {
        itype seeddata[2];
        generate_to<2>(std::forward<SeedSeq>(seedSeq), seeddata);
        seed(seeddata[1], seeddata[0]);
    }


    template<typename... Args>
    void seed(Args&&... args)
    {
        new (this) engine(std::forward<Args>(args)...);
    }

    template <typename xtype1, typename itype1,
              typename output_mixin1, bool output_previous1,
              typename stream_mixin_lhs, typename multiplier_mixin_lhs,
              typename stream_mixin_rhs, typename multiplier_mixin_rhs>
    friend bool operator==(const engine<xtype1,itype1,
                                     output_mixin1,output_previous1,
                                     stream_mixin_lhs, multiplier_mixin_lhs>&,
                           const engine<xtype1,itype1,
                                     output_mixin1,output_previous1,
                                     stream_mixin_rhs, multiplier_mixin_rhs>&);

    template <typename xtype1, typename itype1,
              typename output_mixin1, bool output_previous1,
              typename stream_mixin_lhs, typename multiplier_mixin_lhs,
              typename stream_mixin_rhs, typename multiplier_mixin_rhs>
    friend itype1 operator-(const engine<xtype1,itype1,
                                     output_mixin1,output_previous1,
                                     stream_mixin_lhs, multiplier_mixin_lhs>&,
                            const engine<xtype1,itype1,
                                     output_mixin1,output_previous1,
                                     stream_mixin_rhs, multiplier_mixin_rhs>&);

    template <typename CharT, typename Traits,
              typename xtype1, typename itype1,
              typename output_mixin1, bool output_previous1,
              typename stream_mixin1, typename multiplier_mixin1>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& out,
               const engine<xtype1,itype1,
                              output_mixin1,output_previous1,
                              stream_mixin1, multiplier_mixin1>&);

    template <typename CharT, typename Traits,
              typename xtype1, typename itype1,
              typename output_mixin1, bool output_previous1,
              typename stream_mixin1, typename multiplier_mixin1>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& in,
               engine<xtype1, itype1,
                        output_mixin1, output_previous1,
                        stream_mixin1, multiplier_mixin1>& rng);
};

template <typename CharT, typename Traits,
          typename xtype, typename itype,
          typename output_mixin, bool output_previous,
          typename stream_mixin, typename multiplier_mixin>
std::basic_ostream<CharT,Traits>&
operator<<(std::basic_ostream<CharT,Traits>& out,
           const engine<xtype,itype,
                          output_mixin,output_previous,
                          stream_mixin, multiplier_mixin>& rng)
{
    using pcg_extras::operator<<;

    auto orig_flags = out.flags(std::ios_base::dec | std::ios_base::left);
    auto space = out.widen(' ');
    auto orig_fill = out.fill();

    out << rng.multiplier() << space
        << rng.increment() << space
        << rng.state_;

    out.flags(orig_flags);
    out.fill(orig_fill);
    return out;
}


template <typename CharT, typename Traits,
          typename xtype, typename itype,
          typename output_mixin, bool output_previous,
          typename stream_mixin, typename multiplier_mixin>
std::basic_istream<CharT,Traits>&
operator>>(std::basic_istream<CharT,Traits>& in,
           engine<xtype,itype,
                    output_mixin,output_previous,
                    stream_mixin, multiplier_mixin>& rng)
{
    using pcg_extras::operator>>;

    auto orig_flags = in.flags(std::ios_base::dec | std::ios_base::skipws);

    itype multiplier, increment, state;
    in >> multiplier >> increment >> state;

    if (!in.fail()) {
        bool good = true;
        if (multiplier != rng.multiplier()) {
           good = false;
        } else if (rng.can_specify_stream) {
           rng.set_stream(increment >> 1);
        } else if (increment != rng.increment()) {
           good = false;
        }
        if (good) {
            rng.state_ = state;
        } else {
            in.clear(std::ios::failbit);
        }
    }

    in.flags(orig_flags);
    return in;
}


template <typename xtype, typename itype,
          typename output_mixin, bool output_previous,
          typename stream_mixin, typename multiplier_mixin>
itype engine<xtype,itype,output_mixin,output_previous,stream_mixin,
             multiplier_mixin>::advance(
    itype state, itype delta, itype cur_mult, itype cur_plus)
{
    // The method used here is based on Brown, "Random Number Generation
    // with Arbitrary Stride,", Transactions of the American Nuclear
    // Society (Nov. 1994).  The algorithm is very similar to fast
    // exponentiation.
    //
    // Even though delta is an unsigned integer, we can pass a
    // signed integer to go backwards, it just goes "the long way round".

    constexpr itype ZERO = 0u;  // itype may be a non-trivial types, so
    constexpr itype ONE  = 1u;  // we define some ugly constants.
    itype acc_mult = 1;
    itype acc_plus = 0;
    while (delta > ZERO) {
       if (delta & ONE) {
          acc_mult *= cur_mult;
          acc_plus = acc_plus*cur_mult + cur_plus;
       }
       cur_plus = (cur_mult+ONE)*cur_plus;
       cur_mult *= cur_mult;
       delta >>= 1;
    }
    return acc_mult * state + acc_plus;
}

template <typename xtype, typename itype,
          typename output_mixin, bool output_previous,
          typename stream_mixin, typename multiplier_mixin>
itype engine<xtype,itype,output_mixin,output_previous,stream_mixin,
               multiplier_mixin>::distance(
    itype cur_state, itype newstate, itype cur_mult, itype cur_plus, itype mask)
{
    constexpr itype ONE  = 1u;  // itype could be weird, so use constant
    bool is_mcg = cur_plus == itype(0);
    itype the_bit = is_mcg ? itype(4u) : itype(1u);
    itype distance = 0u;
    while ((cur_state & mask) != (newstate & mask)) {
       if ((cur_state & the_bit) != (newstate & the_bit)) {
           cur_state = cur_state * cur_mult + cur_plus;
           distance |= the_bit;
       }
       assert((cur_state & the_bit) == (newstate & the_bit));
       the_bit <<= 1;
       cur_plus = (cur_mult+ONE)*cur_plus;
       cur_mult *= cur_mult;
    }
    return is_mcg ? distance >> 2 : distance;
}

template <typename xtype, typename itype,
          typename output_mixin, bool output_previous,
          typename stream_mixin_lhs, typename multiplier_mixin_lhs,
          typename stream_mixin_rhs, typename multiplier_mixin_rhs>
itype operator-(const engine<xtype,itype,
                               output_mixin,output_previous,
                               stream_mixin_lhs, multiplier_mixin_lhs>& lhs,
               const engine<xtype,itype,
                               output_mixin,output_previous,
                               stream_mixin_rhs, multiplier_mixin_rhs>& rhs)
{
    static_assert(
        std::is_same<stream_mixin_lhs, stream_mixin_rhs>::value &&
            std::is_same<multiplier_mixin_lhs, multiplier_mixin_rhs>::value,
        "Incomparable generators");
    if (lhs.increment() == rhs.increment()) {
       return rhs.distance(lhs.state_);
    } else  {
       constexpr itype ONE = 1u;
       itype lhs_diff = lhs.increment() + (lhs.multiplier()-ONE) * lhs.state_;
       itype rhs_diff = rhs.increment() + (rhs.multiplier()-ONE) * rhs.state_;
       if ((lhs_diff & itype(3u)) != (rhs_diff & itype(3u))) {
           rhs_diff = -rhs_diff;
       }
       return rhs.distance(rhs_diff, lhs_diff, rhs.multiplier(), itype(0u));
    }
}


template <typename xtype, typename itype,
          typename output_mixin, bool output_previous,
          typename stream_mixin_lhs, typename multiplier_mixin_lhs,
          typename stream_mixin_rhs, typename multiplier_mixin_rhs>
bool operator==(const engine<xtype,itype,
                               output_mixin,output_previous,
                               stream_mixin_lhs, multiplier_mixin_lhs>& lhs,
                const engine<xtype,itype,
                               output_mixin,output_previous,
                               stream_mixin_rhs, multiplier_mixin_rhs>& rhs)
{
    return    (lhs.multiplier() == rhs.multiplier())
           && (lhs.increment()  == rhs.increment())
           && (lhs.state_       == rhs.state_);
}

template <typename xtype, typename itype,
          typename output_mixin, bool output_previous,
          typename stream_mixin_lhs, typename multiplier_mixin_lhs,
          typename stream_mixin_rhs, typename multiplier_mixin_rhs>
inline bool operator!=(const engine<xtype,itype,
                               output_mixin,output_previous,
                               stream_mixin_lhs, multiplier_mixin_lhs>& lhs,
                       const engine<xtype,itype,
                               output_mixin,output_previous,
                               stream_mixin_rhs, multiplier_mixin_rhs>& rhs)
{
    return !operator==(lhs,rhs);
}


template <typename xtype, typename itype,
         template<typename XT,typename IT> class output_mixin,
         bool output_previous = (sizeof(itype) <= 8),
         template<typename IT> class multiplier_mixin = default_multiplier>
using oneseq_base  = engine<xtype, itype,
                        output_mixin<xtype, itype>, output_previous,
                        oneseq_stream<itype>,
                        multiplier_mixin<itype> >;

template <typename xtype, typename itype,
         template<typename XT,typename IT> class output_mixin,
         bool output_previous = (sizeof(itype) <= 8),
         template<typename IT> class multiplier_mixin = default_multiplier>
using unique_base = engine<xtype, itype,
                         output_mixin<xtype, itype>, output_previous,
                         unique_stream<itype>,
                         multiplier_mixin<itype> >;

template <typename xtype, typename itype,
         template<typename XT,typename IT> class output_mixin,
         bool output_previous = (sizeof(itype) <= 8),
         template<typename IT> class multiplier_mixin = default_multiplier>
using setseq_base = engine<xtype, itype,
                         output_mixin<xtype, itype>, output_previous,
                         specific_stream<itype>,
                         multiplier_mixin<itype> >;

template <typename xtype, typename itype,
         template<typename XT,typename IT> class output_mixin,
         bool output_previous = (sizeof(itype) <= 8),
         template<typename IT> class multiplier_mixin = default_multiplier>
using mcg_base = engine<xtype, itype,
                      output_mixin<xtype, itype>, output_previous,
                      no_stream<itype>,
                      multiplier_mixin<itype> >;

/*
 * OUTPUT FUNCTIONS.
 *
 * These are the core of the PCG generation scheme.  They specify how to
 * turn the base LCG's internal state into the output value of the final
 * generator.
 *
 * They're implemented as mixin classes.
 *
 * All of the classes have code that is written to allow it to be applied
 * at *arbitrary* bit sizes, although in practice they'll only be used at
 * standard sizes supported by C++.
 */

/*
 * XSH RS -- high xorshift, followed by a random shift
 *
 * Fast.  A good performer.
 */

template <typename xtype, typename itype>
struct xsh_rs_mixin {
    static xtype output(itype internal)
    {
        constexpr bitcount_t bits        = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t xtypebits   = bitcount_t(sizeof(xtype) * 8);
        constexpr bitcount_t sparebits   = bits - xtypebits;
        constexpr bitcount_t opbits =
                              sparebits-5 >= 64 ? 5
                            : sparebits-4 >= 32 ? 4
                            : sparebits-3 >= 16 ? 3
                            : sparebits-2 >= 4  ? 2
                            : sparebits-1 >= 1  ? 1
                            :                     0;
        constexpr bitcount_t mask = (1 << opbits) - 1;
        constexpr bitcount_t maxrandshift  = mask;
        constexpr bitcount_t topspare     = opbits;
        constexpr bitcount_t bottomspare = sparebits - topspare;
        constexpr bitcount_t xshift     = topspare + (xtypebits+maxrandshift)/2;
        bitcount_t rshift =
            opbits ? bitcount_t(internal >> (bits - opbits)) & mask : 0;
        internal ^= internal >> xshift;
        xtype result = xtype(internal >> (bottomspare - maxrandshift + rshift));
        return result;
    }
};

/*
 * XSH RR -- high xorshift, followed by a random rotate
 *
 * Fast.  A good performer.  Slightly better statistically than XSH RS.
 */

template <typename xtype, typename itype>
struct xsh_rr_mixin {
    static xtype output(itype internal)
    {
        constexpr bitcount_t bits        = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t xtypebits   = bitcount_t(sizeof(xtype)*8);
        constexpr bitcount_t sparebits   = bits - xtypebits;
        constexpr bitcount_t wantedopbits =
                              xtypebits >= 128 ? 7
                            : xtypebits >=  64 ? 6
                            : xtypebits >=  32 ? 5
                            : xtypebits >=  16 ? 4
                            :                    3;
        constexpr bitcount_t opbits =
                              sparebits >= wantedopbits ? wantedopbits
                                                        : sparebits;
        constexpr bitcount_t amplifier = wantedopbits - opbits;
        constexpr bitcount_t mask = (1 << opbits) - 1;
        constexpr bitcount_t topspare    = opbits;
        constexpr bitcount_t bottomspare = sparebits - topspare;
        constexpr bitcount_t xshift      = (topspare + xtypebits)/2;
        bitcount_t rot = opbits ? bitcount_t(internal >> (bits - opbits)) & mask
                                : 0;
        bitcount_t amprot = (rot << amplifier) & mask;
        internal ^= internal >> xshift;
        xtype result = xtype(internal >> bottomspare);
        result = rotr(result, amprot);
        return result;
    }
};

/*
 * RXS -- random xorshift
 */

template <typename xtype, typename itype>
struct rxs_mixin {
static xtype output_rxs(itype internal)
    {
        constexpr bitcount_t bits        = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t xtypebits   = bitcount_t(sizeof(xtype)*8);
        constexpr bitcount_t shift       = bits - xtypebits;
        constexpr bitcount_t extrashift  = (xtypebits - shift)/2;
        bitcount_t rshift = shift > 64+8 ? (internal >> (bits - 6)) & 63
                       : shift > 32+4 ? (internal >> (bits - 5)) & 31
                       : shift > 16+2 ? (internal >> (bits - 4)) & 15
                       : shift >  8+1 ? (internal >> (bits - 3)) & 7
                       : shift >  4+1 ? (internal >> (bits - 2)) & 3
                       : shift >  2+1 ? (internal >> (bits - 1)) & 1
                       :              0;
        internal ^= internal >> (shift + extrashift - rshift);
        xtype result = internal >> rshift;
        return result;
    }
};

/*
 * RXS M XS -- random xorshift, mcg multiply, fixed xorshift
 *
 * The most statistically powerful generator, but all those steps
 * make it slower than some of the others.  We give it the rottenest jobs.
 *
 * Because it's usually used in contexts where the state type and the
 * result type are the same, it is a permutation and is thus invertable.
 * We thus provide a function to invert it.  This function is used to
 * for the "inside out" generator used by the extended generator.
 */

/* Defined type-based concepts for the multiplication step.  They're actually
 * all derived by truncating the 128-bit, which was computed to be a good
 * "universal" constant.
 */

template <typename T>
struct mcg_multiplier {
    // Not defined for an arbitrary type
};

template <typename T>
struct mcg_unmultiplier {
    // Not defined for an arbitrary type
};

PCG_DEFINE_CONSTANT(uint8_t,  mcg, multiplier,   217U)
PCG_DEFINE_CONSTANT(uint8_t,  mcg, unmultiplier, 105U)

PCG_DEFINE_CONSTANT(uint16_t, mcg, multiplier,   62169U)
PCG_DEFINE_CONSTANT(uint16_t, mcg, unmultiplier, 28009U)

PCG_DEFINE_CONSTANT(uint32_t, mcg, multiplier,   277803737U)
PCG_DEFINE_CONSTANT(uint32_t, mcg, unmultiplier, 2897767785U)

PCG_DEFINE_CONSTANT(uint64_t, mcg, multiplier,   12605985483714917081ULL)
PCG_DEFINE_CONSTANT(uint64_t, mcg, unmultiplier, 15009553638781119849ULL)

PCG_DEFINE_CONSTANT(pcg128_t, mcg, multiplier,
        PCG_128BIT_CONSTANT(17766728186571221404ULL, 12605985483714917081ULL))
PCG_DEFINE_CONSTANT(pcg128_t, mcg, unmultiplier,
        PCG_128BIT_CONSTANT(14422606686972528997ULL, 15009553638781119849ULL))


template <typename xtype, typename itype>
struct rxs_m_xs_mixin {
    static xtype output(itype internal)
    {
        constexpr bitcount_t xtypebits = bitcount_t(sizeof(xtype) * 8);
        constexpr bitcount_t bits = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t opbits = xtypebits >= 128 ? 6
                                 : xtypebits >=  64 ? 5
                                 : xtypebits >=  32 ? 4
                                 : xtypebits >=  16 ? 3
                                 :                    2;
        constexpr bitcount_t shift = bits - xtypebits;
        constexpr bitcount_t mask = (1 << opbits) - 1;
        bitcount_t rshift =
            opbits ? bitcount_t(internal >> (bits - opbits)) & mask : 0;
        internal ^= internal >> (opbits + rshift);
        internal *= mcg_multiplier<itype>::multiplier();
        xtype result = internal >> shift;
        result ^= result >> ((2U*xtypebits+2U)/3U);
        return result;
    }

    static itype unoutput(itype internal)
    {
        constexpr bitcount_t bits = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t opbits = bits >= 128 ? 6
                                 : bits >=  64 ? 5
                                 : bits >=  32 ? 4
                                 : bits >=  16 ? 3
                                 :               2;
        constexpr bitcount_t mask = (1 << opbits) - 1;

        internal = unxorshift(internal, bits, (2U*bits+2U)/3U);

        internal *= mcg_unmultiplier<itype>::unmultiplier();

        bitcount_t rshift = opbits ? (internal >> (bits - opbits)) & mask : 0;
        internal = unxorshift(internal, bits, opbits + rshift);

        return internal;
    }
};


/*
 * RXS M -- random xorshift, mcg multiply
 */

template <typename xtype, typename itype>
struct rxs_m_mixin {
    static xtype output(itype internal)
    {
        constexpr bitcount_t xtypebits = bitcount_t(sizeof(xtype) * 8);
        constexpr bitcount_t bits = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t opbits = xtypebits >= 128 ? 6
                                 : xtypebits >=  64 ? 5
                                 : xtypebits >=  32 ? 4
                                 : xtypebits >=  16 ? 3
                                 :                    2;
        constexpr bitcount_t shift = bits - xtypebits;
        constexpr bitcount_t mask = (1 << opbits) - 1;
        bitcount_t rshift = opbits ? (internal >> (bits - opbits)) & mask : 0;
        internal ^= internal >> (opbits + rshift);
        internal *= mcg_multiplier<itype>::multiplier();
        xtype result = internal >> shift;
        return result;
    }
};


/*
 * DXSM -- double xorshift multiply
 *
 * This is a new, more powerful output permutation (added in 2019).  It's
 * a more comprehensive scrambling than RXS M, but runs faster on 128-bit
 * types.  Although primarily intended for use at large sizes, also works
 * at smaller sizes as well.
 *
 * This permutation is similar to xorshift multiply hash functions, except
 * that one of the multipliers is the LCG multiplier (to avoid needing to
 * have a second constant) and the other is based on the low-order bits.
 * This latter aspect means that the scrambling applied to the high bits
 * depends on the low bits, and makes it (to my eye) impractical to back
 * out the permutation without having the low-order bits.
 */

template <typename xtype, typename itype>
struct dxsm_mixin {
    inline xtype output(itype internal)
    {
        constexpr bitcount_t xtypebits = bitcount_t(sizeof(xtype) * 8);
        constexpr bitcount_t itypebits = bitcount_t(sizeof(itype) * 8);
        static_assert(xtypebits <= itypebits/2,
                      "Output type must be half the size of the state type.");
        
        xtype hi = xtype(internal >> (itypebits - xtypebits));
        xtype lo = xtype(internal);

        lo |= 1;
        hi ^= hi >> (xtypebits/2);
	hi *= xtype(cheap_multiplier<itype>::multiplier());
	hi ^= hi >> (3*(xtypebits/4));
	hi *= lo;
	return hi;
    }
};


/*
 * XSL RR -- fixed xorshift (to low bits), random rotate
 *
 * Useful for 128-bit types that are split across two CPU registers.
 */

template <typename xtype, typename itype>
struct xsl_rr_mixin {
    static xtype output(itype internal)
    {
        constexpr bitcount_t xtypebits = bitcount_t(sizeof(xtype) * 8);
        constexpr bitcount_t bits = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t sparebits = bits - xtypebits;
        constexpr bitcount_t wantedopbits = xtypebits >= 128 ? 7
                                       : xtypebits >=  64 ? 6
                                       : xtypebits >=  32 ? 5
                                       : xtypebits >=  16 ? 4
                                       :                    3;
        constexpr bitcount_t opbits = sparebits >= wantedopbits ? wantedopbits
                                                             : sparebits;
        constexpr bitcount_t amplifier = wantedopbits - opbits;
        constexpr bitcount_t mask = (1 << opbits) - 1;
        constexpr bitcount_t topspare = sparebits;
        constexpr bitcount_t bottomspare = sparebits - topspare;
        constexpr bitcount_t xshift = (topspare + xtypebits) / 2;

        bitcount_t rot =
            opbits ? bitcount_t(internal >> (bits - opbits)) & mask : 0;
        bitcount_t amprot = (rot << amplifier) & mask;
        internal ^= internal >> xshift;
        xtype result = xtype(internal >> bottomspare);
        result = rotr(result, amprot);
        return result;
    }
};


/*
 * XSL RR RR -- fixed xorshift (to low bits), random rotate (both parts)
 *
 * Useful for 128-bit types that are split across two CPU registers.
 * If you really want an invertable 128-bit RNG, I guess this is the one.
 */

template <typename T> struct halfsize_trait {};
template <> struct halfsize_trait<pcg128_t>  { typedef uint64_t type; };
template <> struct halfsize_trait<uint64_t>  { typedef uint32_t type; };
template <> struct halfsize_trait<uint32_t>  { typedef uint16_t type; };
template <> struct halfsize_trait<uint16_t>  { typedef uint8_t type;  };

template <typename xtype, typename itype>
struct xsl_rr_rr_mixin {
    typedef typename halfsize_trait<itype>::type htype;

    static itype output(itype internal)
    {
        constexpr bitcount_t htypebits = bitcount_t(sizeof(htype) * 8);
        constexpr bitcount_t bits      = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t sparebits = bits - htypebits;
        constexpr bitcount_t wantedopbits = htypebits >= 128 ? 7
                                       : htypebits >=  64 ? 6
                                       : htypebits >=  32 ? 5
                                       : htypebits >=  16 ? 4
                                       :                    3;
        constexpr bitcount_t opbits = sparebits >= wantedopbits ? wantedopbits
                                                                : sparebits;
        constexpr bitcount_t amplifier = wantedopbits - opbits;
        constexpr bitcount_t mask = (1 << opbits) - 1;
        constexpr bitcount_t topspare = sparebits;
        constexpr bitcount_t xshift = (topspare + htypebits) / 2;

        bitcount_t rot =
            opbits ? bitcount_t(internal >> (bits - opbits)) & mask : 0;
        bitcount_t amprot = (rot << amplifier) & mask;
        internal ^= internal >> xshift;
        htype lowbits = htype(internal);
        lowbits = rotr(lowbits, amprot);
        htype highbits = htype(internal >> topspare);
        bitcount_t rot2 = lowbits & mask;
        bitcount_t amprot2 = (rot2 << amplifier) & mask;
        highbits = rotr(highbits, amprot2);
        return (itype(highbits) << topspare) ^ itype(lowbits);
    }
};


/*
 * XSH -- fixed xorshift (to high bits)
 *
 * You shouldn't use this at 64-bits or less.
 */

template <typename xtype, typename itype>
struct xsh_mixin {
    static xtype output(itype internal)
    {
        constexpr bitcount_t xtypebits = bitcount_t(sizeof(xtype) * 8);
        constexpr bitcount_t bits = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t sparebits = bits - xtypebits;
        constexpr bitcount_t topspare = 0;
        constexpr bitcount_t bottomspare = sparebits - topspare;
        constexpr bitcount_t xshift = (topspare + xtypebits) / 2;

        internal ^= internal >> xshift;
        xtype result = internal >> bottomspare;
        return result;
    }
};

/*
 * XSL -- fixed xorshift (to low bits)
 *
 * You shouldn't use this at 64-bits or less.
 */

template <typename xtype, typename itype>
struct xsl_mixin {
    inline xtype output(itype internal)
    {
        constexpr bitcount_t xtypebits = bitcount_t(sizeof(xtype) * 8);
        constexpr bitcount_t bits = bitcount_t(sizeof(itype) * 8);
        constexpr bitcount_t sparebits = bits - xtypebits;
        constexpr bitcount_t topspare = sparebits;
        constexpr bitcount_t bottomspare = sparebits - topspare;
        constexpr bitcount_t xshift = (topspare + xtypebits) / 2;

        internal ^= internal >> xshift;
        xtype result = internal >> bottomspare;
        return result;
    }
};


/* ---- End of Output Functions ---- */


template <typename baseclass>
struct inside_out : private baseclass {
    inside_out() = delete;

    typedef typename baseclass::result_type result_type;
    typedef typename baseclass::state_type  state_type;
    static_assert(sizeof(result_type) == sizeof(state_type),
                  "Require a RNG whose output function is a permutation");

    static bool external_step(result_type& randval, size_t i)
    {
        state_type state = baseclass::unoutput(randval);
        state = state * baseclass::multiplier() + baseclass::increment()
                + state_type(i*2);
        result_type result = baseclass::output(state);
        randval = result;
        state_type zero =
            baseclass::is_mcg ? state & state_type(3U) : state_type(0U);
        return result == zero;
    }

    static bool external_advance(result_type& randval, size_t i,
                                 result_type delta, bool forwards = true)
    {
        state_type state = baseclass::unoutput(randval);
        state_type mult  = baseclass::multiplier();
        state_type inc   = baseclass::increment() + state_type(i*2);
        state_type zero =
            baseclass::is_mcg ? state & state_type(3U) : state_type(0U);
        state_type dist_to_zero = baseclass::distance(state, zero, mult, inc);
        bool crosses_zero =
            forwards ? dist_to_zero <= delta
                     : (-dist_to_zero) <= delta;
        if (!forwards)
            delta = -delta;
        state = baseclass::advance(state, delta, mult, inc);
        randval = baseclass::output(state);
        return crosses_zero;
    }
};


template <bitcount_t table_pow2, bitcount_t advance_pow2, typename baseclass, typename extvalclass, bool kdd = true>
class extended : public baseclass {
public:
    typedef typename baseclass::state_type  state_type;
    typedef typename baseclass::result_type result_type;
    typedef inside_out<extvalclass> insideout;

private:
    static constexpr bitcount_t rtypebits = sizeof(result_type)*8;
    static constexpr bitcount_t stypebits = sizeof(state_type)*8;

    static constexpr bitcount_t tick_limit_pow2 = 64U;

    static constexpr size_t table_size  = 1UL << table_pow2;
    static constexpr size_t table_shift = stypebits - table_pow2;
    static constexpr state_type table_mask =
        (state_type(1U) << table_pow2) - state_type(1U);

    static constexpr bool   may_tick  =
        (advance_pow2 < stypebits) && (advance_pow2 < tick_limit_pow2);
    static constexpr size_t tick_shift = stypebits - advance_pow2;
    static constexpr state_type tick_mask  =
        may_tick ? state_type(
                       (uint64_t(1) << (advance_pow2*may_tick)) - 1)
                                        // ^-- stupidity to appease GCC warnings
                 : ~state_type(0U);

    static constexpr bool may_tock = stypebits < tick_limit_pow2;

    result_type data_[table_size];

    PCG_NOINLINE void advance_table();

    PCG_NOINLINE void advance_table(state_type delta, bool isForwards = true);

    result_type& get_extended_value()
    {
        state_type state = this->state_;
        if (kdd && baseclass::is_mcg) {
            // The low order bits of an MCG are constant, so drop them.
            state >>= 2;
        }
        size_t index       = kdd ? state &  table_mask
                                 : state >> table_shift;

        if (may_tick) {
            bool tick = kdd ? (state & tick_mask) == state_type(0u)
                            : (state >> tick_shift) == state_type(0u);
            if (tick)
                    advance_table();
        }
        if (may_tock) {
            bool tock = state == state_type(0u);
            if (tock)
                advance_table();
        }
        return data_[index];
    }

public:
    static constexpr size_t period_pow2()
    {
        return baseclass::period_pow2() + table_size*extvalclass::period_pow2();
    }

    PCG_ALWAYS_INLINE result_type operator()()
    {
        result_type rhs = get_extended_value();
        result_type lhs = this->baseclass::operator()();
        return lhs ^ rhs;
    }

    result_type operator()(result_type upper_bound)
    {
        return bounded_rand(*this, upper_bound);
    }

    void set(result_type wanted)
    {
        result_type& rhs = get_extended_value();
        result_type lhs = this->baseclass::operator()();
        rhs = lhs ^ wanted;
    }

    void advance(state_type distance, bool forwards = true);

    void backstep(state_type distance)
    {
        advance(distance, false);
    }

    extended(const result_type* data)
        : baseclass()
    {
        datainit(data);
    }

    extended(const result_type* data, state_type seed)
        : baseclass(seed)
    {
        datainit(data);
    }

    // This function may or may not exist.  It thus has to be a template
    // to use SFINAE; users don't have to worry about its template-ness.

    template <typename bc = baseclass>
    extended(const result_type* data, state_type seed,
            typename bc::stream_state stream_seed)
        : baseclass(seed, stream_seed)
    {
        datainit(data);
    }

    extended()
        : baseclass()
    {
        selfinit();
    }

    extended(state_type seed)
        : baseclass(seed)
    {
        selfinit();
    }

    // This function may or may not exist.  It thus has to be a template
    // to use SFINAE; users don't have to worry about its template-ness.

    template <typename bc = baseclass>
    extended(state_type seed, typename bc::stream_state stream_seed)
        : baseclass(seed, stream_seed)
    {
        selfinit();
    }

private:
    void selfinit();
    void datainit(const result_type* data);

public:

    template<typename SeedSeq, typename = typename std::enable_if<
           !std::is_convertible<SeedSeq, result_type>::value
        && !std::is_convertible<SeedSeq, extended>::value>::type>
    extended(SeedSeq&& seedSeq)
        : baseclass(seedSeq)
    {
        generate_to<table_size>(seedSeq, data_);
    }

    template<typename... Args>
    void seed(Args&&... args)
    {
        new (this) extended(std::forward<Args>(args)...);
    }

    template <bitcount_t table_pow2_, bitcount_t advance_pow2_,
              typename baseclass_, typename extvalclass_, bool kdd_>
    friend bool operator==(const extended<table_pow2_, advance_pow2_,
                                              baseclass_, extvalclass_, kdd_>&,
                           const extended<table_pow2_, advance_pow2_,
                                              baseclass_, extvalclass_, kdd_>&);

    template <typename CharT, typename Traits,
              bitcount_t table_pow2_, bitcount_t advance_pow2_,
              typename baseclass_, typename extvalclass_, bool kdd_>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& out,
               const extended<table_pow2_, advance_pow2_,
                              baseclass_, extvalclass_, kdd_>&);

    template <typename CharT, typename Traits,
              bitcount_t table_pow2_, bitcount_t advance_pow2_,
              typename baseclass_, typename extvalclass_, bool kdd_>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& in,
               extended<table_pow2_, advance_pow2_,
                        baseclass_, extvalclass_, kdd_>&);

};


template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
void extended<table_pow2,advance_pow2,baseclass,extvalclass,kdd>::datainit(
         const result_type* data)
{
    for (size_t i = 0; i < table_size; ++i)
        data_[i] = data[i];
}

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
void extended<table_pow2,advance_pow2,baseclass,extvalclass,kdd>::selfinit()
{
    // We need to fill the extended table with something, and we have
    // very little provided data, so we use the base generator to
    // produce values.  Although not ideal (use a seed sequence, folks!),
    // unexpected correlations are mitigated by
    //      - using XOR differences rather than the number directly
    //      - the way the table is accessed, its values *won't* be accessed
    //        in the same order the were written.
    //      - any strange correlations would only be apparent if we
    //        were to backstep the generator so that the base generator
    //        was generating the same values again
    result_type lhs = baseclass::operator()();
    result_type rhs = baseclass::operator()();
    result_type xdiff = lhs - rhs;
    for (size_t i = 0; i < table_size; ++i) {
        data_[i] = baseclass::operator()() ^ xdiff;
    }
}

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
bool operator==(const extended<table_pow2, advance_pow2,
                               baseclass, extvalclass, kdd>& lhs,
                const extended<table_pow2, advance_pow2,
                               baseclass, extvalclass, kdd>& rhs)
{
    auto& base_lhs = static_cast<const baseclass&>(lhs);
    auto& base_rhs = static_cast<const baseclass&>(rhs);
    return base_lhs == base_rhs
        && std::equal(
               std::begin(lhs.data_), std::end(lhs.data_),
               std::begin(rhs.data_)
           );
}

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
inline bool operator!=(const extended<table_pow2, advance_pow2,
                                      baseclass, extvalclass, kdd>& lhs,
                       const extended<table_pow2, advance_pow2,
                                      baseclass, extvalclass, kdd>& rhs)
{
    return !operator==(lhs, rhs);
}

template <typename CharT, typename Traits,
          bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
std::basic_ostream<CharT,Traits>&
operator<<(std::basic_ostream<CharT,Traits>& out,
           const extended<table_pow2, advance_pow2,
                          baseclass, extvalclass, kdd>& rng)
{
    using pcg_extras::operator<<;

    auto orig_flags = out.flags(std::ios_base::dec | std::ios_base::left);
    auto space = out.widen(' ');
    auto orig_fill = out.fill();

    out << rng.multiplier() << space
        << rng.increment() << space
        << rng.state_;

    for (const auto& datum : rng.data_)
        out << space << datum;

    out.flags(orig_flags);
    out.fill(orig_fill);
    return out;
}

template <typename CharT, typename Traits,
          bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
std::basic_istream<CharT,Traits>&
operator>>(std::basic_istream<CharT,Traits>& in,
           extended<table_pow2, advance_pow2,
                    baseclass, extvalclass, kdd>& rng)
{
    extended<table_pow2, advance_pow2, baseclass, extvalclass> new_rng;
    auto& base_rng = static_cast<baseclass&>(new_rng);
    in >> base_rng;

    if (in.fail())
        return in;

    using pcg_extras::operator>>;

    auto orig_flags = in.flags(std::ios_base::dec | std::ios_base::skipws);

    for (auto& datum : new_rng.data_) {
        in >> datum;
        if (in.fail())
            goto bail;
    }

    rng = new_rng;

bail:
    in.flags(orig_flags);
    return in;
}



template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
void
extended<table_pow2,advance_pow2,baseclass,extvalclass,kdd>::advance_table()
{
    bool carry = false;
    for (size_t i = 0; i < table_size; ++i) {
        if (carry) {
            carry = insideout::external_step(data_[i],i+1);
        }
        bool carry2 = insideout::external_step(data_[i],i+1);
        carry = carry || carry2;
    }
}

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
void
extended<table_pow2,advance_pow2,baseclass,extvalclass,kdd>::advance_table(
        state_type delta, bool isForwards)
{
    typedef typename baseclass::state_type   base_state_t;
    typedef typename extvalclass::state_type ext_state_t;
    constexpr bitcount_t basebits = sizeof(base_state_t)*8;
    constexpr bitcount_t extbits  = sizeof(ext_state_t)*8;
    static_assert(basebits <= extbits || advance_pow2 > 0,
                  "Current implementation might overflow its carry");

    base_state_t carry = 0;
    for (size_t i = 0; i < table_size; ++i) {
        base_state_t total_delta = carry + delta;
        ext_state_t  trunc_delta = ext_state_t(total_delta);
        if (basebits > extbits) {
            carry = total_delta >> extbits;
        } else {
            carry = 0;
        }
        carry +=
            insideout::external_advance(data_[i],i+1, trunc_delta, isForwards);
    }
}

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename baseclass, typename extvalclass, bool kdd>
void extended<table_pow2,advance_pow2,baseclass,extvalclass,kdd>::advance(
    state_type distance, bool forwards)
{
    static_assert(kdd,
        "Efficient advance is too hard for non-kdd extension. "
        "For a weak advance, cast to base class");
    state_type zero =
        baseclass::is_mcg ? this->state_ & state_type(3U) : state_type(0U);
    if (may_tick) {
        state_type ticks = distance >> (advance_pow2*may_tick);
                                        // ^-- stupidity to appease GCC
                                        // warnings
        state_type adv_mask =
            baseclass::is_mcg ? tick_mask << 2 : tick_mask;
        state_type next_advance_distance = this->distance(zero, adv_mask);
        if (!forwards)
            next_advance_distance = (-next_advance_distance) & tick_mask;
        if (next_advance_distance < (distance & tick_mask)) {
            ++ticks;
        }
        if (ticks)
            advance_table(ticks, forwards);
    }
    if (forwards) {
        if (may_tock && this->distance(zero) <= distance)
            advance_table();
        baseclass::advance(distance);
    } else {
        if (may_tock && -(this->distance(zero)) <= distance)
            advance_table(state_type(1U), false);
        baseclass::advance(-distance);
    }
}

} // namespace pcg_detail

namespace pcg_engines {

using namespace pcg_detail;

/* Predefined types for XSH RS */

typedef oneseq_base<uint8_t,  uint16_t, xsh_rs_mixin>  oneseq_xsh_rs_16_8;
typedef oneseq_base<uint16_t, uint32_t, xsh_rs_mixin>  oneseq_xsh_rs_32_16;
typedef oneseq_base<uint32_t, uint64_t, xsh_rs_mixin>  oneseq_xsh_rs_64_32;
typedef oneseq_base<uint64_t, pcg128_t, xsh_rs_mixin>  oneseq_xsh_rs_128_64;
typedef oneseq_base<uint64_t, pcg128_t, xsh_rs_mixin, true, cheap_multiplier>
                                                       cm_oneseq_xsh_rs_128_64;

typedef unique_base<uint8_t,  uint16_t, xsh_rs_mixin>  unique_xsh_rs_16_8;
typedef unique_base<uint16_t, uint32_t, xsh_rs_mixin>  unique_xsh_rs_32_16;
typedef unique_base<uint32_t, uint64_t, xsh_rs_mixin>  unique_xsh_rs_64_32;
typedef unique_base<uint64_t, pcg128_t, xsh_rs_mixin>  unique_xsh_rs_128_64;
typedef unique_base<uint64_t, pcg128_t, xsh_rs_mixin, true, cheap_multiplier>
                                                       cm_unique_xsh_rs_128_64;

typedef setseq_base<uint8_t,  uint16_t, xsh_rs_mixin>  setseq_xsh_rs_16_8;
typedef setseq_base<uint16_t, uint32_t, xsh_rs_mixin>  setseq_xsh_rs_32_16;
typedef setseq_base<uint32_t, uint64_t, xsh_rs_mixin>  setseq_xsh_rs_64_32;
typedef setseq_base<uint64_t, pcg128_t, xsh_rs_mixin>  setseq_xsh_rs_128_64;
typedef setseq_base<uint64_t, pcg128_t, xsh_rs_mixin, true, cheap_multiplier>
                                                       cm_setseq_xsh_rs_128_64;

typedef mcg_base<uint8_t,  uint16_t, xsh_rs_mixin>  mcg_xsh_rs_16_8;
typedef mcg_base<uint16_t, uint32_t, xsh_rs_mixin>  mcg_xsh_rs_32_16;
typedef mcg_base<uint32_t, uint64_t, xsh_rs_mixin>  mcg_xsh_rs_64_32;
typedef mcg_base<uint64_t, pcg128_t, xsh_rs_mixin>  mcg_xsh_rs_128_64;
typedef mcg_base<uint64_t, pcg128_t, xsh_rs_mixin, true, cheap_multiplier>
                                                    cm_mcg_xsh_rs_128_64;

/* Predefined types for XSH RR */

typedef oneseq_base<uint8_t,  uint16_t, xsh_rr_mixin>  oneseq_xsh_rr_16_8;
typedef oneseq_base<uint16_t, uint32_t, xsh_rr_mixin>  oneseq_xsh_rr_32_16;
typedef oneseq_base<uint32_t, uint64_t, xsh_rr_mixin>  oneseq_xsh_rr_64_32;
typedef oneseq_base<uint64_t, pcg128_t, xsh_rr_mixin>  oneseq_xsh_rr_128_64;
typedef oneseq_base<uint64_t, pcg128_t, xsh_rr_mixin, true, cheap_multiplier>
                                                       cm_oneseq_xsh_rr_128_64;

typedef unique_base<uint8_t,  uint16_t, xsh_rr_mixin>  unique_xsh_rr_16_8;
typedef unique_base<uint16_t, uint32_t, xsh_rr_mixin>  unique_xsh_rr_32_16;
typedef unique_base<uint32_t, uint64_t, xsh_rr_mixin>  unique_xsh_rr_64_32;
typedef unique_base<uint64_t, pcg128_t, xsh_rr_mixin>  unique_xsh_rr_128_64;
typedef unique_base<uint64_t, pcg128_t, xsh_rr_mixin, true, cheap_multiplier>
                                                       cm_unique_xsh_rr_128_64;

typedef setseq_base<uint8_t,  uint16_t, xsh_rr_mixin>  setseq_xsh_rr_16_8;
typedef setseq_base<uint16_t, uint32_t, xsh_rr_mixin>  setseq_xsh_rr_32_16;
typedef setseq_base<uint32_t, uint64_t, xsh_rr_mixin>  setseq_xsh_rr_64_32;
typedef setseq_base<uint64_t, pcg128_t, xsh_rr_mixin>  setseq_xsh_rr_128_64;
typedef setseq_base<uint64_t, pcg128_t, xsh_rr_mixin, true, cheap_multiplier>
                                                       cm_setseq_xsh_rr_128_64;

typedef mcg_base<uint8_t,  uint16_t, xsh_rr_mixin>  mcg_xsh_rr_16_8;
typedef mcg_base<uint16_t, uint32_t, xsh_rr_mixin>  mcg_xsh_rr_32_16;
typedef mcg_base<uint32_t, uint64_t, xsh_rr_mixin>  mcg_xsh_rr_64_32;
typedef mcg_base<uint64_t, pcg128_t, xsh_rr_mixin>  mcg_xsh_rr_128_64;
typedef mcg_base<uint64_t, pcg128_t, xsh_rr_mixin, true, cheap_multiplier>
                                                    cm_mcg_xsh_rr_128_64;


/* Predefined types for RXS M XS */

typedef oneseq_base<uint8_t,  uint8_t, rxs_m_xs_mixin>   oneseq_rxs_m_xs_8_8;
typedef oneseq_base<uint16_t, uint16_t, rxs_m_xs_mixin>  oneseq_rxs_m_xs_16_16;
typedef oneseq_base<uint32_t, uint32_t, rxs_m_xs_mixin>  oneseq_rxs_m_xs_32_32;
typedef oneseq_base<uint64_t, uint64_t, rxs_m_xs_mixin>  oneseq_rxs_m_xs_64_64;
typedef oneseq_base<pcg128_t, pcg128_t, rxs_m_xs_mixin>
                                                        oneseq_rxs_m_xs_128_128;
typedef oneseq_base<pcg128_t, pcg128_t, rxs_m_xs_mixin, true, cheap_multiplier>
                                                     cm_oneseq_rxs_m_xs_128_128;

typedef unique_base<uint8_t,  uint8_t, rxs_m_xs_mixin>  unique_rxs_m_xs_8_8;
typedef unique_base<uint16_t, uint16_t, rxs_m_xs_mixin> unique_rxs_m_xs_16_16;
typedef unique_base<uint32_t, uint32_t, rxs_m_xs_mixin> unique_rxs_m_xs_32_32;
typedef unique_base<uint64_t, uint64_t, rxs_m_xs_mixin> unique_rxs_m_xs_64_64;
typedef unique_base<pcg128_t, pcg128_t, rxs_m_xs_mixin> unique_rxs_m_xs_128_128;
typedef unique_base<pcg128_t, pcg128_t, rxs_m_xs_mixin, true, cheap_multiplier>
                                                     cm_unique_rxs_m_xs_128_128;

typedef setseq_base<uint8_t,  uint8_t, rxs_m_xs_mixin>  setseq_rxs_m_xs_8_8;
typedef setseq_base<uint16_t, uint16_t, rxs_m_xs_mixin> setseq_rxs_m_xs_16_16;
typedef setseq_base<uint32_t, uint32_t, rxs_m_xs_mixin> setseq_rxs_m_xs_32_32;
typedef setseq_base<uint64_t, uint64_t, rxs_m_xs_mixin> setseq_rxs_m_xs_64_64;
typedef setseq_base<pcg128_t, pcg128_t, rxs_m_xs_mixin> setseq_rxs_m_xs_128_128;
typedef setseq_base<pcg128_t, pcg128_t, rxs_m_xs_mixin, true, cheap_multiplier>
                                                     cm_setseq_rxs_m_xs_128_128;

                // MCG versions don't make sense here, so aren't defined.

/* Predefined types for RXS M */

typedef oneseq_base<uint8_t,  uint16_t, rxs_m_mixin>  oneseq_rxs_m_16_8;
typedef oneseq_base<uint16_t, uint32_t, rxs_m_mixin>  oneseq_rxs_m_32_16;
typedef oneseq_base<uint32_t, uint64_t, rxs_m_mixin>  oneseq_rxs_m_64_32;
typedef oneseq_base<uint64_t, pcg128_t, rxs_m_mixin>  oneseq_rxs_m_128_64;
typedef oneseq_base<uint64_t, pcg128_t, rxs_m_mixin, true, cheap_multiplier>
                                                      cm_oneseq_rxs_m_128_64;

typedef unique_base<uint8_t,  uint16_t, rxs_m_mixin>  unique_rxs_m_16_8;
typedef unique_base<uint16_t, uint32_t, rxs_m_mixin>  unique_rxs_m_32_16;
typedef unique_base<uint32_t, uint64_t, rxs_m_mixin>  unique_rxs_m_64_32;
typedef unique_base<uint64_t, pcg128_t, rxs_m_mixin>  unique_rxs_m_128_64;
typedef unique_base<uint64_t, pcg128_t, rxs_m_mixin, true, cheap_multiplier>
                                                      cm_unique_rxs_m_128_64;

typedef setseq_base<uint8_t,  uint16_t, rxs_m_mixin>  setseq_rxs_m_16_8;
typedef setseq_base<uint16_t, uint32_t, rxs_m_mixin>  setseq_rxs_m_32_16;
typedef setseq_base<uint32_t, uint64_t, rxs_m_mixin>  setseq_rxs_m_64_32;
typedef setseq_base<uint64_t, pcg128_t, rxs_m_mixin>  setseq_rxs_m_128_64;
typedef setseq_base<uint64_t, pcg128_t, rxs_m_mixin, true, cheap_multiplier>
                                                      cm_setseq_rxs_m_128_64;

typedef mcg_base<uint8_t,  uint16_t, rxs_m_mixin>  mcg_rxs_m_16_8;
typedef mcg_base<uint16_t, uint32_t, rxs_m_mixin>  mcg_rxs_m_32_16;
typedef mcg_base<uint32_t, uint64_t, rxs_m_mixin>  mcg_rxs_m_64_32;
typedef mcg_base<uint64_t, pcg128_t, rxs_m_mixin>  mcg_rxs_m_128_64;
typedef mcg_base<uint64_t, pcg128_t, rxs_m_mixin, true, cheap_multiplier>
                                                   cm_mcg_rxs_m_128_64;

/* Predefined types for DXSM */

typedef oneseq_base<uint8_t,  uint16_t, dxsm_mixin>  oneseq_dxsm_16_8;
typedef oneseq_base<uint16_t, uint32_t, dxsm_mixin>  oneseq_dxsm_32_16;
typedef oneseq_base<uint32_t, uint64_t, dxsm_mixin>  oneseq_dxsm_64_32;
typedef oneseq_base<uint64_t, pcg128_t, dxsm_mixin>  oneseq_dxsm_128_64;
typedef oneseq_base<uint64_t, pcg128_t, dxsm_mixin, true, cheap_multiplier>
                                                     cm_oneseq_dxsm_128_64;

typedef unique_base<uint8_t,  uint16_t, dxsm_mixin>  unique_dxsm_16_8;
typedef unique_base<uint16_t, uint32_t, dxsm_mixin>  unique_dxsm_32_16;
typedef unique_base<uint32_t, uint64_t, dxsm_mixin>  unique_dxsm_64_32;
typedef unique_base<uint64_t, pcg128_t, dxsm_mixin>  unique_dxsm_128_64;
typedef unique_base<uint64_t, pcg128_t, dxsm_mixin, true, cheap_multiplier>
                                                     cm_unique_dxsm_128_64;

typedef setseq_base<uint8_t,  uint16_t, dxsm_mixin>  setseq_dxsm_16_8;
typedef setseq_base<uint16_t, uint32_t, dxsm_mixin>  setseq_dxsm_32_16;
typedef setseq_base<uint32_t, uint64_t, dxsm_mixin>  setseq_dxsm_64_32;
typedef setseq_base<uint64_t, pcg128_t, dxsm_mixin>  setseq_dxsm_128_64;
typedef setseq_base<uint64_t, pcg128_t, dxsm_mixin, true, cheap_multiplier>
                                                     cm_setseq_dxsm_128_64;

typedef mcg_base<uint8_t,  uint16_t, dxsm_mixin>  mcg_dxsm_16_8;
typedef mcg_base<uint16_t, uint32_t, dxsm_mixin>  mcg_dxsm_32_16;
typedef mcg_base<uint32_t, uint64_t, dxsm_mixin>  mcg_dxsm_64_32;
typedef mcg_base<uint64_t, pcg128_t, dxsm_mixin>  mcg_dxsm_128_64;
typedef mcg_base<uint64_t, pcg128_t, dxsm_mixin, true, cheap_multiplier>
                                                  cm_mcg_dxsm_128_64;

/* Predefined types for XSL RR (only defined for "large" types) */

typedef oneseq_base<uint32_t, uint64_t, xsl_rr_mixin>  oneseq_xsl_rr_64_32;
typedef oneseq_base<uint64_t, pcg128_t, xsl_rr_mixin>  oneseq_xsl_rr_128_64;
typedef oneseq_base<uint64_t, pcg128_t, xsl_rr_mixin, true, cheap_multiplier>
                                                       cm_oneseq_xsl_rr_128_64;

typedef unique_base<uint32_t, uint64_t, xsl_rr_mixin>  unique_xsl_rr_64_32;
typedef unique_base<uint64_t, pcg128_t, xsl_rr_mixin>  unique_xsl_rr_128_64;
typedef unique_base<uint64_t, pcg128_t, xsl_rr_mixin, true, cheap_multiplier>
                                                       cm_unique_xsl_rr_128_64;

typedef setseq_base<uint32_t, uint64_t, xsl_rr_mixin>  setseq_xsl_rr_64_32;
typedef setseq_base<uint64_t, pcg128_t, xsl_rr_mixin>  setseq_xsl_rr_128_64;
typedef setseq_base<uint64_t, pcg128_t, xsl_rr_mixin, true, cheap_multiplier>
                                                       cm_setseq_xsl_rr_128_64;

typedef mcg_base<uint32_t, uint64_t, xsl_rr_mixin>  mcg_xsl_rr_64_32;
typedef mcg_base<uint64_t, pcg128_t, xsl_rr_mixin>  mcg_xsl_rr_128_64;
typedef mcg_base<uint64_t, pcg128_t, xsl_rr_mixin, true, cheap_multiplier>
                                                    cm_mcg_xsl_rr_128_64;


/* Predefined types for XSL RR RR (only defined for "large" types) */

typedef oneseq_base<uint64_t, uint64_t, xsl_rr_rr_mixin>
    oneseq_xsl_rr_rr_64_64;
typedef oneseq_base<pcg128_t, pcg128_t, xsl_rr_rr_mixin>
    oneseq_xsl_rr_rr_128_128;
typedef oneseq_base<pcg128_t, pcg128_t, xsl_rr_rr_mixin, true, cheap_multiplier>
    cm_oneseq_xsl_rr_rr_128_128;

typedef unique_base<uint64_t, uint64_t, xsl_rr_rr_mixin>
    unique_xsl_rr_rr_64_64;
typedef unique_base<pcg128_t, pcg128_t, xsl_rr_rr_mixin>
    unique_xsl_rr_rr_128_128;
typedef unique_base<pcg128_t, pcg128_t, xsl_rr_rr_mixin, true, cheap_multiplier>
    cm_unique_xsl_rr_rr_128_128;

typedef setseq_base<uint64_t, uint64_t, xsl_rr_rr_mixin>
    setseq_xsl_rr_rr_64_64;
typedef setseq_base<pcg128_t, pcg128_t, xsl_rr_rr_mixin>
    setseq_xsl_rr_rr_128_128;
typedef setseq_base<pcg128_t, pcg128_t, xsl_rr_rr_mixin, true, cheap_multiplier>
    cm_setseq_xsl_rr_rr_128_128;

                // MCG versions don't make sense here, so aren't defined.

/* Extended generators */

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename BaseRNG, bool kdd = true>
using ext_std8 = extended<table_pow2, advance_pow2, BaseRNG,
                          oneseq_rxs_m_xs_8_8, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename BaseRNG, bool kdd = true>
using ext_std16 = extended<table_pow2, advance_pow2, BaseRNG,
                           oneseq_rxs_m_xs_16_16, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename BaseRNG, bool kdd = true>
using ext_std32 = extended<table_pow2, advance_pow2, BaseRNG,
                           oneseq_rxs_m_xs_32_32, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2,
          typename BaseRNG, bool kdd = true>
using ext_std64 = extended<table_pow2, advance_pow2, BaseRNG,
                           oneseq_rxs_m_xs_64_64, kdd>;


template <bitcount_t table_pow2, bitcount_t advance_pow2, bool kdd = true>
using ext_oneseq_rxs_m_xs_32_32 =
          ext_std32<table_pow2, advance_pow2, oneseq_rxs_m_xs_32_32, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2, bool kdd = true>
using ext_mcg_xsh_rs_64_32 =
          ext_std32<table_pow2, advance_pow2, mcg_xsh_rs_64_32, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2, bool kdd = true>
using ext_oneseq_xsh_rs_64_32 =
          ext_std32<table_pow2, advance_pow2, oneseq_xsh_rs_64_32, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2, bool kdd = true>
using ext_setseq_xsh_rr_64_32 =
          ext_std32<table_pow2, advance_pow2, setseq_xsh_rr_64_32, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2, bool kdd = true>
using ext_mcg_xsl_rr_128_64 =
          ext_std64<table_pow2, advance_pow2, mcg_xsl_rr_128_64, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2, bool kdd = true>
using ext_oneseq_xsl_rr_128_64 =
          ext_std64<table_pow2, advance_pow2, oneseq_xsl_rr_128_64, kdd>;

template <bitcount_t table_pow2, bitcount_t advance_pow2, bool kdd = true>
using ext_setseq_xsl_rr_128_64 =
          ext_std64<table_pow2, advance_pow2, setseq_xsl_rr_128_64, kdd>;

} // namespace pcg_engines

typedef pcg_engines::setseq_xsh_rr_64_32        pcg32;
typedef pcg_engines::oneseq_xsh_rr_64_32        pcg32_oneseq;
typedef pcg_engines::unique_xsh_rr_64_32        pcg32_unique;
typedef pcg_engines::mcg_xsh_rs_64_32           pcg32_fast;

typedef pcg_engines::setseq_xsl_rr_128_64       pcg64;
typedef pcg_engines::oneseq_xsl_rr_128_64       pcg64_oneseq;
typedef pcg_engines::unique_xsl_rr_128_64       pcg64_unique;
typedef pcg_engines::mcg_xsl_rr_128_64          pcg64_fast;

typedef pcg_engines::setseq_rxs_m_xs_8_8        pcg8_once_insecure;
typedef pcg_engines::setseq_rxs_m_xs_16_16      pcg16_once_insecure;
typedef pcg_engines::setseq_rxs_m_xs_32_32      pcg32_once_insecure;
typedef pcg_engines::setseq_rxs_m_xs_64_64      pcg64_once_insecure;
typedef pcg_engines::setseq_xsl_rr_rr_128_128   pcg128_once_insecure;

typedef pcg_engines::oneseq_rxs_m_xs_8_8        pcg8_oneseq_once_insecure;
typedef pcg_engines::oneseq_rxs_m_xs_16_16      pcg16_oneseq_once_insecure;
typedef pcg_engines::oneseq_rxs_m_xs_32_32      pcg32_oneseq_once_insecure;
typedef pcg_engines::oneseq_rxs_m_xs_64_64      pcg64_oneseq_once_insecure;
typedef pcg_engines::oneseq_xsl_rr_rr_128_128   pcg128_oneseq_once_insecure;


// These two extended RNGs provide two-dimensionally equidistributed
// 32-bit generators.  pcg32_k2_fast occupies the same space as pcg64,
// and can be called twice to generate 64 bits, but does not required
// 128-bit math; on 32-bit systems, it's faster than pcg64 as well.

typedef pcg_engines::ext_setseq_xsh_rr_64_32<1,16,true>     pcg32_k2;
typedef pcg_engines::ext_oneseq_xsh_rs_64_32<1,32,true>     pcg32_k2_fast;

// These eight extended RNGs have about as much state as arc4random
//
//  - the k variants are k-dimensionally equidistributed
//  - the c variants offer better crypographic security
//
// (just how good the cryptographic security is is an open question)

typedef pcg_engines::ext_setseq_xsh_rr_64_32<6,16,true>     pcg32_k64;
typedef pcg_engines::ext_mcg_xsh_rs_64_32<6,32,true>        pcg32_k64_oneseq;
typedef pcg_engines::ext_oneseq_xsh_rs_64_32<6,32,true>     pcg32_k64_fast;

typedef pcg_engines::ext_setseq_xsh_rr_64_32<6,16,false>    pcg32_c64;
typedef pcg_engines::ext_oneseq_xsh_rs_64_32<6,32,false>    pcg32_c64_oneseq;
typedef pcg_engines::ext_mcg_xsh_rs_64_32<6,32,false>       pcg32_c64_fast;

typedef pcg_engines::ext_setseq_xsl_rr_128_64<5,16,true>    pcg64_k32;
typedef pcg_engines::ext_oneseq_xsl_rr_128_64<5,128,true>   pcg64_k32_oneseq;
typedef pcg_engines::ext_mcg_xsl_rr_128_64<5,128,true>      pcg64_k32_fast;

typedef pcg_engines::ext_setseq_xsl_rr_128_64<5,16,false>   pcg64_c32;
typedef pcg_engines::ext_oneseq_xsl_rr_128_64<5,128,false>  pcg64_c32_oneseq;
typedef pcg_engines::ext_mcg_xsl_rr_128_64<5,128,false>     pcg64_c32_fast;

// These eight extended RNGs have more state than the Mersenne twister
//
//  - the k variants are k-dimensionally equidistributed
//  - the c variants offer better crypographic security
//
// (just how good the cryptographic security is is an open question)

typedef pcg_engines::ext_setseq_xsh_rr_64_32<10,16,true>    pcg32_k1024;
typedef pcg_engines::ext_oneseq_xsh_rs_64_32<10,32,true>    pcg32_k1024_fast;

typedef pcg_engines::ext_setseq_xsh_rr_64_32<10,16,false>   pcg32_c1024;
typedef pcg_engines::ext_oneseq_xsh_rs_64_32<10,32,false>   pcg32_c1024_fast;

typedef pcg_engines::ext_setseq_xsl_rr_128_64<10,16,true>   pcg64_k1024;
typedef pcg_engines::ext_oneseq_xsl_rr_128_64<10,128,true>  pcg64_k1024_fast;

typedef pcg_engines::ext_setseq_xsl_rr_128_64<10,16,false>  pcg64_c1024;
typedef pcg_engines::ext_oneseq_xsl_rr_128_64<10,128,false> pcg64_c1024_fast;

// These generators have an insanely huge period (2^524352), and is suitable
// for silly party tricks, such as dumping out 64 KB ZIP files at an arbitrary
// point in the future.   [Actually, over the full period of the generator, it
// will produce every 64 KB ZIP file 2^64 times!]

typedef pcg_engines::ext_setseq_xsh_rr_64_32<14,16,true>    pcg32_k16384;
typedef pcg_engines::ext_oneseq_xsh_rs_64_32<14,32,true>    pcg32_k16384_fast;

#ifdef _MSC_VER
    #pragma warning(default:4146)
#endif

#endif // PCG_RAND_HPP_INCLUDED

#include <array>

class Random
{
private:
	pcg32 rng;
public:
	Random()
	{
		pcg_extras::seed_seq_from<std::random_device> seed_source;
		rng.seed(seed_source);
	}

	Random(const Random&) = delete;
	Random(Random&&) = delete;
	Random& operator=(const Random&) = delete;
	Random& operator=(Random&&) = delete;

	struct Charset
	{
		/// <summary>
		/// Charset for base64 strings.
		/// </summary>
		static constexpr const char* Base64 
			= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-";

		/// <summary>
		/// Charset for alphabetic strings.
		/// </summary>
		static constexpr const char* Alpha
			= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		/// <summary>
		/// Charset for alphanumeric strings.
		/// </summary>
		static constexpr const char* AlphaNum
			= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		/// <summary>
		/// Charset for numeric strings.
		/// </summary>
		static constexpr const char* Numeric
			= "0123456789";

		/// <summary>
		/// Charset for hexadecimal strings.
		/// </summary>
		static constexpr const char* Hex
			= "0123456789ABCDEF";

		/// <summary>
		/// Charset for binary strings.
		/// </summary>
		static constexpr const char* Binary
			= "01";
	};
private:
	inline static Random& Get() noexcept
	{
		static Random instance;
		return instance;
	}

	template <typename T> 
	inline T GetInt_Impl(T begin, T end)
	{
		std::uniform_int_distribution<T> dis(begin, end);
		return dis(rng);
	}

	template <typename T> 
	inline T GetInt_Binomial_Impl(T t, double p)
	{
		std::binomial_distribution<T> dis{ t, p };
		return dis(rng);
	}

	template <typename T, size_t N> 
	inline std::array<T, N>&& GetIntArray_Impl(T begin, T end)
	{
		std::array<T, N>&& arr{};
		std::uniform_int_distribution<T> distribution{ begin, end };
		for (auto& i : arr) {
			i = distribution(rng);
		}
		return std::move(arr);
	}

	template <typename Float_t, size_t N> 
	inline std::array<Float_t, N>&& GetFloatArray_Impl(Float_t min, Float_t max)
	{
		std::array<Float_t, N>&& arr{};
		std::uniform_real_distribution<Float_t> distribution(min, max);
		for (auto& i : arr) {
			i = distribution(rng);
		}
		return std::move(arr);
	}

	template <typename Float_t>
	inline Float_t GetFloat_Impl(Float_t min, Float_t max)
	{
		std::uniform_real_distribution<Float_t> dis{ min, max };
		return dis(rng);
	}

	template <typename Float_t>
	inline Float_t GetFloat_Normal_Impl(Float_t mean, Float_t stddev = static_cast<Float_t>(1.0))
	{
		std::normal_distribution<Float_t> dis{ mean, stddev };
		return dis(rng);
	}

	inline std::string GetString_Impl(char begin, char end, const size_t length)
	{
		std::uniform_int_distribution<std::int16_t> dis{ begin, end };
		std::string str;
		str.resize(length + 1);
		for (size_t i = 0; i < length + 1; i++) {
			str[i] = static_cast<char>(dis(rng));
		}
		return std::move(str);
	}

	inline std::string GetString_Impl(std::string_view charset, const size_t length)
	{
		std::uniform_int_distribution<size_t> dis{ 0, charset.length()-1 };
		std::string str;
		str.resize(length + 1);
		for (size_t i = 0; i < length + 1; i++) {
			str[i] = charset[dis(rng)];
		}
		return std::move(str);
	}
public:
	/// <summary>
	/// Generates a random integer of chosen size between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	template<class Int_t>
	inline static Int_t GetInt(Int_t begin, Int_t end)
	{
		return Get().GetInt_Impl(begin, end);
	}

	/// <summary>
	/// Generates a random integer of word size between 0 and t (inclusive).
	/// Uses a binomial distribution with a probability of p.
	/// </summary>
	/// <param name="t"></param>
	/// <param name="p"></param>
	/// <returns></returns>
	template<class Int_t>
	inline static Int_t GetIntBinomial(Int_t t, double p)
	{
		return Get().GetInt_Binomial_Impl(t, p);
	}

	/// <summary>
	/// Generates a random unsigned 8-bit integer between 0 and 255 (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	inline static unsigned char GetByte()
	{
		return static_cast<unsigned char>(GetInt<int16_t>(0, 255));
	}

	/// <summary>
	/// Generates a random float between min and max (exclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="min"></param>
	/// <param name="max"></param>
	/// <returns></returns>
	template<typename Float_t>
	inline static Float_t GetFloat(Float_t min, Float_t max)
	{
		return Get().GetFloat_Impl(min, max);
	}

	/// <summary>
	/// Generates a random float between min and max (exclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="min"></param>
	/// <param name="max"></param>
	/// <returns></returns>
	template<typename Float_t>
	inline static Float_t GetFloatNormal(Float_t mean, Float_t stddev = static_cast<Float_t>(1.0))
	{
		return Get().GetFloat_Normal_Impl(mean, stddev);
	}

	/// <summary>
	/// Generates a random double between 0.0 and 1.0 (exclusive), and returns true if the double is
	/// less than pct.
	/// </summary>
	/// <param name="pct">The percentage chance that the function returns true.</param>
	/// <returns></returns>
	inline static bool Chance(double pct)
	{
		pct = std::clamp(pct, 0.0, 1.0);
		return (Get().GetFloat_Impl(0.0, 1.0)) < pct;
	}

	/// <summary>
	/// Generates a random integer between 1 and d (inclusive), and returns true if the integer is
	/// less than or equal to n.
	/// </summary>
	/// <param name="n">Numerator of the fraction.</param>
	/// <param name="d">Denominator of the fraction.</param>
	/// <returns></returns>
	inline static bool Chance(int n, int d)
	{
		n = std::clamp(n, 0, d);
		return Get().GetInt_Impl(1, d) <= n;
	}

	/// <summary>
	/// Shuffles a container between iterators begin and end.
	/// </summary>
	/// <typeparam name="Iter_t">Iterator type</typeparam>
	/// <param name="begin">Start iterator.</param>
	/// <param name="end">End iterator.</param>
	template<typename Iter_t>
	inline static void Shuffle(Iter_t begin, Iter_t end)
	{
		std::shuffle(begin, end, Get().rng);
	}

	/// <summary>
	/// Shuffles a container.
	/// </summary>
	/// <typeparam name="Container_t">Iterator type</typeparam>
	/// <param name="container">Reference to container to be shuffled.</param>
	template<typename Container_t>
	inline static void Shuffle(Container_t& container)
	{
		std::shuffle(container.begin(), container.end(), Get().rng);
	}

	/// <summary>
	/// Copies a container, then shuffles and returns the copy.
	/// </summary>
	/// <typeparam name="Container_t">Iterator type</typeparam>
	/// <param name="container">Constant reference to container to be copied and shuffled.</param>
	template<typename Container_t>
	inline static Container_t&& ShuffleCopy(const Container_t& container)
	{
		Container_t copy = container;
		std::shuffle(copy.begin(), copy.end(), Get().rng);
		return std::move(copy);
	}

	/// <summary>
	/// Generates a string of length "length" + 1 with characters between begin and end (inclusive).
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <param name="length"></param>
	/// <returns></returns>
	inline static std::string GetString(char begin, char end, const size_t length)
	{
		return Get().GetString_Impl(begin, end, length);
	}

	/// <summary>
	/// Generates a string of length "length" + 1 with a defined charset.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <param name="length"></param>
	/// <returns></returns>
	inline static std::string GetString(std::string_view charset, const size_t length)
	{
		return Get().GetString_Impl(charset, length);
	}

	/// <summary>
	/// Reseeds the generator engine with a new seed.
	/// </summary>
	/// <param name="value">Seed</param>
	template<typename... Args>
	inline static void Seed(Args&& ...args)
	{
		Get().rng.seed(args...);
	}

	inline static void Seed()
	{
		pcg_extras::seed_seq_from<std::random_device> seed_source;
		Get().rng.seed(seed_source);
	}
public:
	/// <summary>
	/// Generates N random integers between begin and end and returns them in a std::array
	/// by r-value reference.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	template<size_t N, typename Int_t>
	inline static std::array<Int_t, N>&& GetIntArray(Int_t begin, Int_t end) {
		return std::move(Get().GetIntArray_Impl<Int_t, N>(begin, end));
	}

	/// <summary>
	/// Generates N random floats between begin and end and returns them in a std::array
	/// by r-value reference.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	template<size_t N, typename Float_t>
	inline static std::array<Float_t, N>&& GetFloatArray(Float_t begin, Float_t end) {
		return std::move(Get().GetFloatArray_Impl<Float_t, N>(begin, end));
	}
};

#endif //RANDOM_SINGLE_INCLUDE_HPP
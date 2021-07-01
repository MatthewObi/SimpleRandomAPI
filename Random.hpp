///Author: Matthew Obi
///Copyright 2021
#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <random>
#include "pcg/pcg_random.hpp"
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

#endif
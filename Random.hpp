///Author: Matthew Obi
///Copyright 2021
#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <random>
#include <array>

class Random
{
public:
	Random();
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
	};

	/// <summary>
	/// Generates a random 64-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::int64_t GetInt64(std::int64_t begin, std::int64_t end); 

	/// <summary>
	/// Generates a random 64-bit integer between 0 and t, following a binomial
	/// distribution.
	/// </summary>
	/// <param name="t"></param>
	/// <param name="p"></param>
	/// <returns></returns>
	static std::int64_t GetInt64Binomial(std::int64_t t, double p = 0.5);

	/// <summary>
	/// Generates a random 32-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::int32_t GetInt32(std::int32_t begin, std::int32_t end);

	/// <summary>
	/// Generates a random 32-bit integer between 0 and t, following a binomial
	/// distribution.
	/// </summary>
	/// <param name="t"></param>
	/// <param name="p"></param>
	/// <returns></returns>
	static std::int32_t GetInt32Binomial(std::int32_t t, double p = 0.5);

	/// <summary>
	/// Generates a random 16-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::int16_t GetInt16(std::int16_t begin, std::int16_t end);

	/// <summary>
	/// Generates a random 16-bit integer between 0 and t, following a binomial
	/// distribution.
	/// </summary>
	/// <param name="t"></param>
	/// <param name="p"></param>
	/// <returns></returns>
	static std::int16_t GetInt16Binomial(std::int16_t t, double p = 0.5);

	/// <summary>
	/// Generates a random 8-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::int8_t GetInt8(std::int8_t begin, std::int8_t end);

	/// <summary>
	/// Generates a random unsigned 64-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::uint64_t GetUint64(std::uint64_t begin, std::uint64_t end);

	/// <summary>
	/// Generates a random unsigned 32-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::uint32_t GetUint32(std::uint32_t begin, std::uint32_t end);

	/// <summary>
	/// Generates a random unsigned 16-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::uint16_t GetUint16(std::uint16_t begin, std::uint16_t end);

	/// <summary>
	/// Generates a random unsigned 8-bit integer between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static std::uint8_t GetUint8(std::uint8_t begin, std::uint8_t end);

	/// <summary>
	/// Generates a random unsigned 8-bit integer between 0 and 255 (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static unsigned char GetByte();

	/// <summary>
	/// Generates a random integer of word size between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static int GetInt(int begin, int end);

	/// <summary>
	/// Generates a random integer of word size between begin and end (inclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	static unsigned GetUnsigned(unsigned begin, unsigned end);

	/// <summary>
	/// Generates a random double between min and max (exclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="min"></param>
	/// <param name="max"></param>
	/// <returns></returns>
	static double GetDouble(double min, double max);

	/// <summary>
	/// Generates a random double using a normal distribution with a mean at mean,
	/// and a standard deviation of stddev.
	/// </summary>
	/// <param name="mean"></param>
	/// <param name="stddev"></param>
	/// <returns></returns>
	static double GetDoubleNormal(double mean, double stddev = 1.0);

	/// <summary>
	/// Generates a random float between min and max (exclusive).
	/// Uses a uniform distribution.
	/// </summary>
	/// <param name="min"></param>
	/// <param name="max"></param>
	/// <returns></returns>
	static float GetFloat(float min, float max);

	/// <summary>
	/// Generates a random float using a normal distribution with a mean at mean,
	/// and a standard deviation of stddev.
	/// </summary>
	/// <param name="mean"></param>
	/// <param name="stddev"></param>
	/// <returns></returns>
	static float GetFloatNormal(float mean, float stddev = 1.0f);

	/// <summary>
	/// Generates a random double between 0.0 and 1.0 (exclusive), and returns true if the double is
	/// less than pct.
	/// </summary>
	/// <param name="pct">The percentage chance that the function returns true.</param>
	/// <returns></returns>
	static bool Chance(double pct);

	/// <summary>
	/// Generates a random integer between 1 and d (inclusive), and returns true if the integer is
	/// less than or equal to n.
	/// </summary>
	/// <param name="n">Numerator of the fraction.</param>
	/// <param name="d">Denominator of the fraction.</param>
	/// <returns></returns>
	static bool Chance(int n, int d);

	/// <summary>
	/// Shuffles a container between iterators begin and end.
	/// </summary>
	/// <typeparam name="Iter_t">Iterator type</typeparam>
	/// <param name="begin">Start iterator.</param>
	/// <param name="end">End iterator.</param>
	template<typename Iter_t>
	static void Shuffle(Iter_t begin, Iter_t end)
	{
		std::shuffle(begin, end, Get().mt);
	}

	/// <summary>
	/// Shuffles a container.
	/// </summary>
	/// <typeparam name="Container_t">Iterator type</typeparam>
	/// <param name="container">Reference to container to be shuffled.</param>
	template<typename Container_t>
	static void Shuffle(Container_t& container)
	{
		std::shuffle(container.begin(), container.end(), Get().mt);
	}

	/// <summary>
	/// Copies a container, then shuffles and returns the copy.
	/// </summary>
	/// <typeparam name="Container_t">Iterator type</typeparam>
	/// <param name="container">Constant reference to container to be copied and shuffled.</param>
	template<typename Container_t>
	static Container_t&& ShuffleCopy(const Container_t& container)
	{
		Container_t copy = container;
		std::shuffle(copy.begin(), copy.end(), Get().mt);
		return std::move(copy);
	}

	/// <summary>
	/// Generates a string of length "length" + 1 with characters between begin and end (inclusive).
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <param name="length"></param>
	/// <returns></returns>
	static std::string GetString(char begin, char end, const size_t length);

	/// <summary>
	/// Generates a string of length "length" + 1 with a defined charset.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <param name="length"></param>
	/// <returns></returns>
	static std::string GetString(std::string_view charset, const size_t length);

	/// <summary>
	/// Reseeds the generator engine with a new seed.
	/// </summary>
	/// <param name="value">Seed</param>
	static void Seed(std::mt19937_64::result_type value = std::mt19937_64::default_seed);
private:
	static Random& Get() noexcept;
	template <typename T> T GetInt_Impl(T begin, T end);
	template <typename T> T GetInt_Binomial_Impl(T t, double p);
	template <typename T, size_t N> std::array<T, N>&& GetIntArray_Impl(T begin, T end)
	{
		std::array<T, N>&& arr{};
		std::uniform_int_distribution<T> distribution{ begin, end };
		for (auto& i : arr) {
			i = distribution(mt);
		}
		return std::move(arr);
	}

	template <size_t N> std::array<double, N>&& GetDoubleArray_Impl(double min, double max)
	{
		std::array<double, N>&& arr{};
		std::uniform_real_distribution<double> distribution(min, max);
		for (auto& i : arr) {
			i = distribution(mt);
		}
		return std::move(arr);
	}

	template <size_t N> std::array<float, N>&& GetFloatArray_Impl(float min, float max)
	{
		std::array<float, N>&& arr{};
		std::uniform_real_distribution<float> distribution( min, max );
		for (auto& i : arr) {
			i = distribution(mt);
		}
		return std::move(arr);
	}

	double GetDouble_Impl(double min, double max);
	double GetDouble_Normal_Impl(double mean, double stddev = 1.0);
	float GetFloat_Impl(float min, float max);
	float GetFloat_Normal_Impl(float mean, float stddev = 1.0f);

	std::string GetString_Impl(char begin, char end, const size_t length);
	std::string GetString_Impl(std::string_view chars, const size_t length);
public:
	/// <summary>
	/// Generates N random integers between begin and end and returns them in a std::array
	/// by r-value reference.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	template<size_t N>
	inline static std::array<int, N>&& GetIntArray(int begin, int end) {
		return std::move(Get().GetIntArray_Impl<int, N>(begin, end));
	}

	/// <summary>
	/// Generates N random doubles between begin and end and returns them in a std::array
	/// by r-value reference.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	template<size_t N>
	inline static std::array<double, N>&& GetDoubleArray(double begin, double end) {
		return std::move(Get().GetDoubleArray_Impl<N>(begin, end));
	}

	/// <summary>
	/// Generates N random floats between begin and end and returns them in a std::array
	/// by r-value reference.
	/// </summary>
	/// <param name="begin"></param>
	/// <param name="end"></param>
	/// <returns></returns>
	template<size_t N>
	inline static std::array<float, N>&& GetFloatArray(float begin, float end) {
		return std::move(Get().GetFloatArray_Impl<N>(begin, end));
	}
private:
	std::mt19937_64 mt;
};

#endif
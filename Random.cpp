///Author: Matthew Obi
///Copyright 2021
#include "Random.hpp"

Random::Random() : mt(std::random_device{}())
{
}

Random& Random::Get() noexcept
{
	static Random instance;
	return instance;
}


std::int64_t Random::GetInt64(std::int64_t begin, std::int64_t end)
{
	return Get().GetInt_Impl(begin, end);
}

std::int64_t Random::GetInt64Binomial(std::int64_t t, double p)
{
	return Get().GetInt_Binomial_Impl(t, p);
}

std::int32_t Random::GetInt32(std::int32_t begin, std::int32_t end)
{
	return Get().GetInt_Impl(begin, end);
}

std::int32_t Random::GetInt32Binomial(std::int32_t t, double p)
{
	return Get().GetInt_Binomial_Impl(t, p);
}

std::int16_t Random::GetInt16(std::int16_t begin, std::int16_t end)
{
	return Get().GetInt_Impl(begin, end);
}

std::int16_t Random::GetInt16Binomial(std::int16_t t, double p)
{
	return Get().GetInt_Binomial_Impl(t, p);
}

std::int8_t Random::GetInt8(std::int8_t begin, std::int8_t end)
{
	return static_cast<std::int8_t>(GetInt16(begin, end));
}

std::uint64_t Random::GetUint64(std::uint64_t begin, std::uint64_t end)
{
	return Get().GetInt_Impl(begin, end);
}

std::uint32_t Random::GetUint32(std::uint32_t begin, std::uint32_t end)
{
	return Get().GetInt_Impl(begin, end);
}

std::uint16_t Random::GetUint16(std::uint16_t begin, std::uint16_t end)
{
	return Get().GetInt_Impl(begin, end);
}

std::uint8_t Random::GetUint8(std::uint8_t begin, std::uint8_t end)
{
	return static_cast<std::uint8_t>(GetUint16(begin, end));
}

unsigned char Random::GetByte()
{
	return static_cast<unsigned char>(GetInt16(0, 255));
}

int Random::GetInt(int begin, int end)
{
	return Get().GetInt_Impl(begin, end);
}

unsigned Random::GetUnsigned(unsigned begin, unsigned end)
{
	return Get().GetInt_Impl(begin, end);
}

double Random::GetDouble(double min, double max)
{
	return Get().GetDouble_Impl(min, max);
}

double Random::GetDoubleNormal(double mean, double stddev)
{
	return Get().GetDouble_Normal_Impl(mean, stddev);
}

float Random::GetFloat(float min, float max)
{
	return Get().GetFloat_Impl(min, max);
}

float Random::GetFloatNormal(float mean, float stddev)
{
	return Get().GetFloat_Normal_Impl(mean, stddev);
}

bool Random::Chance(double pct)
{
	pct = std::clamp(pct, 0.0, 1.0);
	return Get().GetDouble_Impl(0.0, 1.0) < pct;
}

bool Random::Chance(int n, int d)
{
	n = std::clamp(n, 0, d);
	return Get().GetInt_Impl(1, d) <= n;
}

std::string Random::GetString(char begin, char end, const size_t length)
{
	return std::move(Get().GetString_Impl(begin, end, length));
}

std::string Random::GetString(std::string_view charset, const size_t length)
{
	return std::move(Get().GetString_Impl(charset, length));
}

void Random::Seed(std::mt19937_64::result_type value)
{
	Get().mt.seed(value);
}

double Random::GetDouble_Impl(double min, double max)
{
	std::uniform_real_distribution<double> dis{ min, max };
	return dis(mt);
}

double Random::GetDouble_Normal_Impl(double mean, double stddev)
{
	std::normal_distribution<double> dis{ mean, stddev };
	return dis(mt);
}

float Random::GetFloat_Impl(float min, float max)
{
	std::uniform_real_distribution<float> dis{ min, max };
	return dis(mt);
}

float Random::GetFloat_Normal_Impl(float mean, float stddev)
{
	std::normal_distribution<float> dis{ mean, stddev };
	return dis(mt);
}

std::string Random::GetString_Impl(char begin, char end, const size_t length)
{
	std::uniform_int_distribution<std::int16_t> dis{ begin, end };
	std::string str;
	str.resize(length + 1);
	for (size_t i = 0; i < length + 1; i++) {
		str[i] = static_cast<char>(dis(mt));
	}
	return std::move(str);
}

std::string Random::GetString_Impl(std::string_view chars, const size_t length)
{
	std::uniform_int_distribution<size_t> dis{ 0, chars.length()-1 };
	std::string str;
	str.resize(length + 1);
	for (size_t i = 0; i < length + 1; i++) {
		str[i] = chars[dis(mt)];
	}
	return std::move(str);
}


template<typename T>
T Random::GetInt_Impl(T begin, T end)
{
	std::uniform_int_distribution<T> dis(begin, end);
	return dis(mt);
}

template<typename T>
T Random::GetInt_Binomial_Impl(T t, double p)
{
	std::binomial_distribution<T> dis{ t, p };
	return dis(mt);
}

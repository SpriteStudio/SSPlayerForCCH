
#include "SaverUtil.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace saverutil {

/** 整数値を文字列で返す */
std::string toString(int value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}

/** 浮動小数点を短い文字列で返す */
std::string toShortString(float value)
{
	std::stringstream ss;
	double i;
	double d = std::modf(value, &i);

	// 小数部が0のときは小数１桁、それ以外は小数２桁にする。
	// 切り捨てられる小数部は四捨五入される
	int nd = (d == 0.0) ? 1 : 2;
	ss << std::setprecision(nd) << std::setiosflags(std::ios::fixed);

	ss << value;
	return ss.str();
}



/**
 * ParameterBuffer
 */

ParameterBuffer::ParameterBuffer()
{}

ParameterBuffer::~ParameterBuffer()
{}

void ParameterBuffer::addInt(int value)
{
	Parameter param;
	param.value = toString(value);
	_parameters.push_back(param);
}

void ParameterBuffer::addInt(int value, int defaultValue)
{
	Parameter param;
	param.value = toString(value);
	param.defaultValue = boost::optional<std::string>(toString(defaultValue));
	_parameters.push_back(param);
}

void ParameterBuffer::addString( const std::string str )
{
	Parameter	param;
	param.value = str;
	_parameters.push_back(param);
}

void ParameterBuffer::addFloat(float value)
{
	Parameter param;
	param.value = toShortString(value);
	_parameters.push_back(param);
}

void ParameterBuffer::addFloat(float value, float defaultValue)
{
	Parameter param;
	param.value = toShortString(value);
	param.defaultValue = boost::optional<std::string>(toShortString(defaultValue));
	_parameters.push_back(param);
}

/** デフォルト値とおなじ値は後方部から省略しデータ郡を文字列で返す */
std::string ParameterBuffer::toEllipsisString() const
{
	// 後方からデフォルト値とおなじ値のパラメータは除外する 
	int n = static_cast<int>(_parameters.size()) - 1;
	while (n >= 0
		&& _parameters[n].defaultValue
		&& _parameters[n].defaultValue.get() == _parameters[n].value)
	{
		n--;
	}

	// パラメータを文字連結する 
	std::stringstream ss;
	for (int i = 0; i <= n; i++)
	{
		if (i > 0) ss << ",";
		ss << _parameters[i].value;
	}
	return ss.str();
}


};	// saverutil

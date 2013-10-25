#ifndef _SAVER_UTIL_H_
#define _SAVER_UTIL_H_

#include <string>
#include <vector>
#include <boost/optional/optional.hpp>

namespace saverutil {

	/** 整数値を文字列で返す */
	std::string toString(int value);

	/** 浮動小数点を短い文字列で返す */
	std::string toShortString(float value);



	/**
	 * ParameterBuffer
	 */
	class ParameterBuffer
	{
		struct Parameter
		{
			std::string						value;
			boost::optional<std::string>	defaultValue;
		};

		std::vector<Parameter>	_parameters;

	public:
		ParameterBuffer(void);
		virtual ~ParameterBuffer();

		void addInt(int value);
		void addInt(int value, int defaultValue);
		void addFloat(float value);
		void addFloat(float value, float defaultValue);

		void addString( const std::string str );


		/** デフォルト値とおなじ値は後方部から省略しデータ郡を文字列で返す */
		std::string toEllipsisString() const;
	};

};	// saverutil

#endif	// _SAVER_UTIL_H_

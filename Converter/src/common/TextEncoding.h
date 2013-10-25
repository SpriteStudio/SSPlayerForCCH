#ifndef _TEXT_ENCODING_H_
#define _TEXT_ENCODING_H_

#include <string>
#include <fstream>

namespace textenc
{

	enum Encoding
	{
		UNKNOWN,
		UTF8,
		UTF8N,
		SHIFT_JIS,

		END_OF_ENCODING
	};

	Encoding findEncoding(const std::string& enc);

	std::string convert(const std::string& text, Encoding in, Encoding out);

	std::string getUtf8Bom();

	void writeBom(std::ofstream& out, Encoding outEncoding);


};	// textenc

#endif	// _TEXT_ENCODING_H_


#include "TextEncoding.h"
#include <boost/algorithm/string.hpp>


namespace textenc
{

#ifdef _WIN32

    /****************************************************************
     * Windows版コード / Win32 APIを使用
     ****************************************************************/

    #include <windows.h>

	std::wstring toWideChar(const std::string& text, UINT codePage)
	{
		// 変換後の文字数を得る 
		int wcharLen = MultiByteToWideChar(codePage, 0, text.c_str(), -1, NULL, 0);
		// 変換 
		int bufLen = wcharLen + 1;
		wchar_t* buf = new wchar_t[bufLen];
		MultiByteToWideChar(codePage, 0, text.c_str(), -1, buf, bufLen);
		buf[wcharLen] = 0;		// 終端null
		std::wstring str(buf);
		delete [] buf;
		return str;
	}

	std::string toMultiByte(const std::wstring& text, UINT codePage)
	{
		// 変換後の文字数を得る 
		int charLen = WideCharToMultiByte(codePage, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
		// 変換 
		int bufLen = charLen + 1;
		char* buf = new char[bufLen];
		WideCharToMultiByte(codePage, 0, text.c_str(), -1, buf, bufLen, NULL, NULL);
		buf[charLen] = 0;		// 終端null
		std::string str(buf);
		delete [] buf;
		return str;
	}


	std::string convert(const std::string& text, Encoding in, Encoding out)
	{
		if (in == SHIFT_JIS && (out == UTF8 || out == UTF8N))
		{
			std::wstring utf16str = toWideChar(text, 932);
			return toMultiByte(utf16str, CP_UTF8);
		}
		else if ((in == UTF8 || in == UTF8N) && out == SHIFT_JIS)
		{
			std::wstring utf16str = toWideChar(text, CP_UTF8);
			return toMultiByte(utf16str, 932);
		}
		return text;
	}

#else

    /****************************************************************
     * Mac版コード / iconvを使用
     ****************************************************************/
    
    #include <iconv.h>

    std::string convertByIconv(const std::string& text, const char* tocode, const char* fromcode)
    {
        std::string result;
        iconv_t cd = iconv_open(tocode, fromcode);
        if (cd != (iconv_t)-1)
        {
            size_t fromLen = text.size();
            char* fromBuf = new char[fromLen + 1];
            text.copy(fromBuf, fromLen);
            fromBuf[fromLen] = 0;
            
            size_t toLen = fromLen * 4;
            char* toBuf = new char[toLen + 1];
            
            size_t inLen = fromLen;
            size_t outLen = toLen;
            char* in = fromBuf;
            char* out = toBuf;

            // iconv側でin, inLen, out, outLenは書き換えるられるので注意
            size_t rlen = iconv(cd, &in, &inLen, &out, &outLen);
            if ((iconv_t)rlen != (iconv_t)-1)
            {
                *out = 0;
                result = std::string(toBuf);
            }
            
            delete [] toBuf;
            delete [] fromBuf;
            iconv_close(cd);
        }
        return result;
    }

	std::string convert(const std::string& text, Encoding in, Encoding out)
	{
		if (in == SHIFT_JIS && (out == UTF8 || out == UTF8N))
		{
            return convertByIconv(text, "UTF-8", "SHIFT_JIS");
		}
		else if ((in == UTF8 || in == UTF8N) && out == SHIFT_JIS)
		{
            return convertByIconv(text, "SHIFT_JIS", "UTF-8");
		}
		return text;
	}

#endif


	Encoding findEncoding(const std::string& enc)
	{
		// 小文字に変換した上で探す 
		std::string s = boost::algorithm::to_lower_copy(enc);

		if (s == "sjis" || s == "shift_jis" || s == "shift-jis")
		{
			return textenc::SHIFT_JIS;
		}
		else if (s == "utf8" || s == "utf-8")
		{
			return textenc::UTF8;
		}
		else if (s == "utf8n" || s == "utf-8n")
		{
			return textenc::UTF8N;
		}
		else
		{
			return textenc::UNKNOWN;
		}
	}


	std::string getUtf8Bom()
	{
		static unsigned char bom[] = { 0xEF, 0xBB, 0xBF, 0 };
		return std::string(reinterpret_cast<char*>(bom));
	}


	void writeBom(std::ofstream& out, Encoding outEncoding)
	{
		if (outEncoding == textenc::UTF8)
		{
			out << textenc::getUtf8Bom();
		}
	}


};	// textenc


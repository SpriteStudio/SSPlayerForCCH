

#include <string>
#include <vector>
#include <map>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "SsaxLoader.h"
#include "SsfLoader.h"
#include "SsMotionDecoder.h"
#include "SsMotionUtil.h"
#include "Cocos2dSaver.h"
#include "FileUtil.h"
#include "TextEncoding.h"

#include "SsPlayerConverter.h"
#include "ConverterShared.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace ss;
using boost::shared_ptr;



static const char* APP_NAME		= "SsToCocos2d";
static const char* APP_VERSION	= "1.0.1 alpha (Build: " __DATE__ " " __TIME__ ")";


/** 使用方法を出力 */
static void usage(std::ostream& out, po::options_description& desc)
{
	out << APP_NAME << " converter version " << APP_VERSION << std::endl;
	out << "usage: " << APP_NAME << " Input files(.ssax) and/or a file list for image(.ssf) ..." << std::endl;
	out << desc << std::endl;
}

/** デフォルトの拡張子を返す */
static const char* getOutputFileExtension(bool binaryFormatMode)
{
	return !binaryFormatMode ? "c" : "ssba";
}

/** コンバートオプション */
struct Options
{
    SsPlayerConverterResultCode resultCode;
	bool                        isVerbose;
	fs::path                    outFilePath;
	bool						binaryFormatMode;
	textenc::Encoding           outFileEncoding;
	std::vector<fs::path>       ssaxList;
	std::vector<fs::path>       ssfList;
	bool						useTragetAffineTransformation;
};

/** コマンドライン引数をパースしオプションを返す */
static shared_ptr<Options> parseOptions(int argc, const char* argv[]);

typedef std::map<fs::path, SsImageList::Ptr> SsfImageListMap;

/** リストに指定されたssfファイルを読み込み返す */
static SsfImageListMap loadSsfAll(const std::vector<fs::path>& ssfList);

/** ssaxを読み込み、cocos2d用プレイヤー形式で出力する */
static SsPlayerConverterResultCode ssaxToCocos2d(std::ostream& out, const Options& options, const fs::path& ssaxPath, const SsfImageListMap& ssfImageListMap);





/**
 * main
 */
extern "C" int Converter_SsToCocos2d(int argc, const char *argv[])
{
    try
    {
        shared_ptr<Options> options = parseOptions(argc, argv);
        if (options->resultCode != SSPC_SUCCESS)
        {
            return options->resultCode;
        }
        
        SsfImageListMap ssfImageListMap = loadSsfAll(options->ssfList);
        
        SsPlayerConverterResultCode resultCode = SSPC_SUCCESS;

		std::ios_base::openmode openmode = std::ios_base::out;
		if (options->binaryFormatMode) openmode |= std::ios_base::binary;

        if (options->outFilePath.empty())
        {
            // ssaxごとに個別のファイルに出力
            BOOST_FOREACH( const fs::path& ssaxPath, options->ssaxList )
            {
                if (options->isVerbose) std::cout << "converting: " << ssaxPath << std::endl;

                fs::path outPath = ssaxPath;
                outPath.replace_extension(getOutputFileExtension(options->binaryFormatMode));
				std::ofstream out(outPath.generic_string().c_str(), openmode);

				if (!options->binaryFormatMode)
				{
					textenc::writeBom(out, options->outFileEncoding);
				}

                resultCode = ssaxToCocos2d(out, *options, ssaxPath, ssfImageListMap);
                if (resultCode != SSPC_SUCCESS) break;
            }
        }
        else
        {
            // ひとつのファイルに集約し出力
            std::ofstream out(options->outFilePath.generic_string().c_str(), openmode);

			if (!options->binaryFormatMode)
			{
				textenc::writeBom(out, options->outFileEncoding);
			}

            BOOST_FOREACH( const fs::path& ssaxPath, options->ssaxList )
            {
                if (options->isVerbose) std::cout << "converting: " << ssaxPath << std::endl;

                resultCode = ssaxToCocos2d(out, *options, ssaxPath, ssfImageListMap);
                if (resultCode != SSPC_SUCCESS) break;
            }
        }
        
        return resultCode;
    }
    catch (std::exception& /*ex*/)
    {
        // 何らかの内部エラーが起きた
        return SSPC_INTERNAL_ERROR;
    }
}


/**
 * ssaxを読み込み、cocos2dプレイヤー形式で出力する
 */
SsPlayerConverterResultCode ssaxToCocos2d(std::ostream& out, const Options& options, const fs::path& ssaxPath, const SsfImageListMap& ssfImageListMap)
{
	// ssax読み込み
    SsPlayerConverterResultCode resultCode;
    SsMotion::Ptr motion = ConverterShared::loadSsax(ssaxPath, resultCode);
    if (resultCode != SSPC_SUCCESS) return resultCode;

	// 対応関係にあるssfがあるか探す
	SsImageList::ConstPtr imageList;
	if (ssfImageListMap.size() == 1)
	{
		// リストに一つしかないときは常に
		imageList = ssfImageListMap.begin()->second;
	}
	else
	{
		// ssaxと同名のssfを探す
		fs::path ssfPath = ssaxPath;
		ssfPath.replace_extension("ssf");
		if (ssfImageListMap.find(ssfPath) != ssfImageListMap.end())
		{
			imageList = ssfImageListMap.find(ssfPath)->second;
		}
	}

	std::string prefix = ssaxPath.stem().generic_string();
	std::string comment = (boost::format("Created by %1% v%2%") % APP_NAME % APP_VERSION).str();
	Cocos2dSaver::save(out, options.binaryFormatMode, options.outFileEncoding, options.useTragetAffineTransformation, motion, imageList, prefix, comment);

    return SSPC_SUCCESS;
}


/**
 * リストに指定されたssfファイルを読み込み返す
 */
static SsfImageListMap loadSsfAll(const std::vector<fs::path>& ssfList)
{
	SsfImageListMap map;

	BOOST_FOREACH( fs::path ssfPath, ssfList )
	{
		SsImageList::Ptr imageList = SsfLoader::load(ssfPath.generic_string());
		if (imageList)
		{
			map[ssfPath] = imageList;
		}
	}
	return map;
}


/**
 * コマンドライン引数をパースしオプションを返す
 */
static shared_ptr<Options> parseOptions(int argc, const char* argv[])
{
	po::options_description desc("option");
	desc.add_options()
		("help,h",											"Display usage.")
		("out,o", po::value< std::string >(),				"Output filename.")
		("bf,b",											"Output as binary format. (default)")
		("cf,c",											"Output as c source code.")
		("encoding,e", po::value< std::string >(),			"Encoding of output file (UTF8/UTF8N/SJIS) default:UTF8.")
		("affine,a",										"Use Cocos2d-x affine transformation.")
		("in,i", po::value< std::vector<std::string> >(),	"ssax, ssf filename.")
		("verbose,v",										"Verbose mode.")
		;

	po::positional_options_description p;
	p.add("in", -1);

	shared_ptr<Options> options = shared_ptr<Options>(new Options());
	po::variables_map vm;
	try
	{
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	}
	catch (std::exception& ex)
	{
		std::cerr << "Invalid arguments: " << ex.what() << std::endl;
		usage(std::cout, desc);
        options->resultCode = SSPC_ILLEGAL_ARGUMENT;
		return options;
	}
	po::notify(vm);

	if (vm.count("help"))
	{
        // usage表示。正常終了にしています。
		usage(std::cout, desc);
        options->resultCode = SSPC_SUCCESS;
		return options;
	}
	if (!vm.count("in"))
	{
        // 入力ファイルの指定がない
		usage(std::cout, desc);
        options->resultCode = SSPC_NOT_SPECIFY_INPUT_FILE;
		return options;
	}
	

	// *** バイナリ形式で出力するか
	if (vm.count("bf") && vm.count("cf"))
	{
		std::cerr << "Cannot specify both, output as 'binary format' or 'c source code'." << std::endl;
		usage(std::cout, desc);
		options->resultCode = SSPC_NOT_SUPPORT_OUTPUT_ENCODING;
		return options;
	}
	
	bool binaryFormatMode = vm.count("bf") || !vm.count("cf");


	// *** 出力ファイル名チェック
	fs::path outPath;
	if (vm.count("out"))
	{
		std::string outPathStr = vm["out"].as< std::string >();
		outPath = fs::path(outPathStr);
		// 拡張子の指定が無ければ付与する
		if (outPath.extension().empty())
		{
			outPath.replace_extension(getOutputFileExtension(binaryFormatMode));
		}
	}
	
	
	// *** 出力ファイルのエンコーディング形式
	textenc::Encoding outFileEncoding = textenc::UTF8;	// default
	if (vm.count("encoding"))
	{
		std::string encodingStr = vm["encoding"].as< std::string >();

		// 対応するエンコーディング形式か
		textenc::Encoding enc = textenc::findEncoding(encodingStr);
		bool ok =
			   enc == textenc::UTF8
			|| enc == textenc::UTF8N
			|| enc == textenc::SHIFT_JIS
			;

		if (!ok)
		{
			std::cerr << "Unsupported encoding type: " << encodingStr << std::endl;
			usage(std::cout, desc);
            options->resultCode = SSPC_NOT_SUPPORT_OUTPUT_ENCODING;
			return options;
		}

		outFileEncoding = enc;
	}


	// *** 入力ファイル名チェック
	std::vector<fs::path> sources;
	{
		bool error = false;
		std::vector<std::string> in = vm["in"].as< std::vector<std::string> >();
		BOOST_FOREACH( const std::string& str, in )
		{
#ifdef _WIN32
			// Win32プラットフォーム用コード。Win32APIを使ってワイルドカード展開する
			std::vector<std::string> fileList = FileUtil::findPath(str);
			if (!fileList.empty())
			{
				std::copy(fileList.begin(), fileList.end(), std::back_inserter(sources));
			}
			else
			{
				std::cerr << "Cannot find input file: " << str << std::endl;
				error = true;
			}
#else
			// Mac/Unixプラットフォーム用コード
			// 本当にファイルが存在するか確認し、見つからないものがあるときはエラーとする
			fs::path filePath = fs::path(str);
			if (fs::exists(filePath))
			{
				sources.push_back(filePath);
			}
			else
			{
				std::cerr << "Cannot find input file: " << filePath << std::endl;
				error = true;
			}
#endif
		}
		if (error)
        {
            options->resultCode = SSPC_NOT_EXIST_INPUT_FILE;
            return options;
        }
	}

	// 入力ファイルをssax,ssfに分ける
	std::vector<fs::path> ssaxList;
	std::vector<fs::path> ssfList;
	BOOST_FOREACH( const fs::path& filePath, sources )
	{
		fs::path ext = filePath.extension();
		if (ext == ".ssax") ssaxList.push_back(filePath);
		else if (ext == ".ssf") ssfList.push_back(filePath);
		else ;	// それ以外は除外する
	}

	if (ssfList.empty())
	{
		// ssaxと同名のssfが見つかったらリストに加える
		bool existsSsf = false;
		BOOST_FOREACH( const fs::path& filePath, ssaxList )
		{
			fs::path ssfPath(filePath);
			ssfPath.replace_extension(".ssf");
			if (fs::exists(ssfPath))
			{
				ssfList.push_back(ssfPath);

				if (!existsSsf)
				{
					existsSsf = true;
					std::cout<< ".ssf file having the same name to .ssax will be read because a SSF file is not specified." << std::endl;
				}
			}
		}
	}


	// 引数の解析結果を格納し返す
    options->resultCode = SSPC_SUCCESS;
	options->isVerbose = vm.count("verbose") != 0;
	options->outFilePath = outPath;
	options->binaryFormatMode = binaryFormatMode;
	options->outFileEncoding = outFileEncoding;
	options->ssaxList = ssaxList;
	options->ssfList = ssfList;
	options->useTragetAffineTransformation = vm.count("affine") != 0;

	return options;
}


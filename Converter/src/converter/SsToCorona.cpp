

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
#include "CoronaSaver.h"
#include "FileUtil.h"
#include "TextEncoding.h"

#include "SsPlayerConverter.h"
#include "ConverterShared.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace ss;
using boost::shared_ptr;



static const char* APP_NAME		= "SsToCorona";
static const char* APP_VERSION	= "1.0.6 (Build: " __DATE__ " " __TIME__ ")";

static const char* DEFAULT_OUT_EXTESION = "lua";


/** 使用方法を出力 */
static void usage(std::ostream& out, po::options_description& desc)
{
	out << APP_NAME << " converter version " << APP_VERSION << std::endl;
	out << "usage: " << APP_NAME << " Input files(.ssax) and/or a file list for image(.ssf) ..." << std::endl;
	out << desc << std::endl;
}


/** コンバートオプション */
/**
	@note	outFilePath の定義位置を bool isJason, isNoSuffix の後にすると
			dll呼び出し時にssaxList, ssfListの格納領域が不正になり例外が発生する不具合あり
*/
struct Options
{
    SsPlayerConverterResultCode resultCode;
	bool                        isVerbose;
	fs::path                    outFilePath;
	bool                        isJson;				/**< JSON形式で出力します */
	bool                        isNoSuffix;			/**< アニメテーブル変数名に_animationを付けないようにします */
	textenc::Encoding           outFileEncoding;
	std::vector<fs::path>       ssaxList;
	std::vector<fs::path>       ssfList;
	bool						isluaModule;
	bool						isOmitNullPart;		/**< NULLパーツを出力から省く */
};

/** コマンドライン引数をパースしオプションを返す */
static shared_ptr<Options> parseOptions(int argc, const char* argv[]);

typedef std::map<fs::path, SsImageList::Ptr> SsfImageListMap;

/** リストに指定されたssfファイルを読み込み返す */
static SsfImageListMap loadSsfAll(const std::vector<fs::path>& ssfList);


static fs::path appendSuffix(const fs::path& org, const std::string& suffix)
{
	fs::path name = org.stem().generic_string() + suffix + org.extension().generic_string();

	fs::path f = org.parent_path();
	f /= name;
	return f;
}





/**
 * main
 */
extern "C" int Converter_SsToCorona(int argc, const char *argv[])
{
    try
    {
        shared_ptr<Options> options = parseOptions(argc, argv);
        if (options->resultCode != SSPC_SUCCESS)
        {
            return options->resultCode;
        }

        SsfImageListMap ssfImageListMap = loadSsfAll(options->ssfList);
        // ssfが指定されているときは使用する
        SsImageList::ConstPtr imageList;
        fs::path ssfPath;
        if (ssfImageListMap.size() == 1)
        {
            imageList = ssfImageListMap.begin()->second;
            ssfPath = ssfImageListMap.begin()->first;
        }

		std::string comment = (boost::format("Created by %1% v%2%") % APP_NAME % APP_VERSION).str();

        CoronaSaver::Options outOptions;
		outOptions.isluaModule = options->isluaModule;
		outOptions.isNoSuffix = options->isNoSuffix;
		outOptions.isOmitNullPart = options->isOmitNullPart;
        SsPlayerConverterResultCode resultCode = SSPC_SUCCESS;

        if (options->outFilePath.empty())
        {
            // ssaxごとに個別のファイルに出力
            BOOST_FOREACH( const fs::path& ssaxPath, options->ssaxList )
            {
                if (options->isVerbose) std::cout << "converting: " << ssaxPath << std::endl;

                // ssax読み込み
                SsMotion::Ptr motion = ConverterShared::loadSsax(ssaxPath, resultCode);
                if (resultCode != SSPC_SUCCESS) break;
                
                // 出力ファイルを開く
                fs::path outPath = ssaxPath;
                outPath.replace_extension(DEFAULT_OUT_EXTESION);
                std::ofstream out(outPath.generic_string().c_str());
                textenc::writeBom(out, options->outFileEncoding);
                
                CoronaSaver saver(out, options->outFileEncoding, outOptions, comment);
                
                // prefix
                std::string prefix = ssaxPath.stem().generic_string();

                // ssfの指定があるときは、こちらを画像リストファイルとして出力
                if (imageList)
                {
                    saver.writeImageList(imageList, prefix);
                }
                // ssfの指定がなく、motionにImageList部があれば画像リストファイルを出力
                else if (motion->hasImageList())
                {
                    saver.writeImageList(motion, prefix);
                }

                // アニメ部出力
                saver.writeAnimation(motion, imageList, prefix);
            }
        }
        else
        {
            // ひとつのファイルに集約し出力
            std::ofstream out(options->outFilePath.generic_string().c_str());
            textenc::writeBom(out, options->outFileEncoding);

            CoronaSaver saver(out, options->outFileEncoding, outOptions, comment);

            BOOST_FOREACH( const fs::path& ssaxPath, options->ssaxList )
            {
                if (options->isVerbose) std::cout << "converting: " << ssaxPath << std::endl;

                // ssax読み込み
                SsMotion::Ptr motion = ConverterShared::loadSsax(ssaxPath, resultCode);
                if (resultCode != SSPC_SUCCESS) break;

                // prefix
                std::string prefix = ssaxPath.stem().generic_string();

                // ssfの指定があるときは、こちらを画像リストとして出力
                if (imageList)
                {
                    saver.writeImageList(imageList, prefix);
                }
                // ssfの指定がなく、motionにImageList部があれば画像リスト部出力
                else if (motion->hasImageList())
                {
                    saver.writeImageList(motion, prefix);
                }

                // アニメ部出力
                saver.writeAnimation(motion, imageList, prefix);
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
		("encoding,e", po::value< std::string >(),			"Encoding of output file (UTF8/UTF8N/SJIS) default:UTF8.")
		("in,i", po::value< std::vector<std::string> >(),	"ssax, ssf filename.")
		("verbose,v",										"Verbose mode.")
		("module,m",										"file to lua module.")
		("nosuffix,n",										"Avoid adding \"_animation\" to the animation variable name.")
		("nonull,u",										"Omit NULL part from output.")
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


	// *** 出力ファイル名チェック
	fs::path outPath;
	if (vm.count("out"))
	{
		std::string outPathStr = vm["out"].as< std::string >();
		outPath = fs::path(outPathStr);

		// 拡張子の指定が無ければ付与する
		if (outPath.extension().empty())
		{
			outPath.replace_extension(DEFAULT_OUT_EXTESION);
		}
	}


	// *** 出力ファイルのエンコーディング形式
	textenc::Encoding outFileEncoding = textenc::UTF8N;	// default
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

		if ( enc == textenc::UTF8 ) enc = textenc::UTF8N;

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

	// ssfファイルは一つのみ指定可
	if (ssfList.size() > 1)
	{
		std::cerr << ".ssf file must be specified singularly." << std::endl;
		usage(std::cout, desc);
        options->resultCode = SSPC_IMPOSSIBLE_PLURAL_SSF_FILES;
		return options;
	}


	// 引数の解析結果を格納し返す
    options->resultCode = SSPC_SUCCESS;
	options->isVerbose = vm.count("verbose") != 0;
	options->outFilePath = outPath;
	options->outFileEncoding = outFileEncoding;
	options->ssaxList = ssaxList;
	options->ssfList = ssfList;

	options->isluaModule = vm.count("module") != 0;;
	options->isNoSuffix = vm.count("nosuffix") != 0;
	options->isOmitNullPart = vm.count("nonull") != 0;
	

	return options;
}



#include "Canvas2dSaver.h"
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/path.hpp>
#include <sstream>
#include <iostream>

#include "SsMotionDecoder.h"
#include "SsMotionUtil.h"
#include "SaverUtil.h"
#include "MathUtil.h"

using boost::shared_ptr;
using boost::format;
using namespace ss;

// インデントあたりのスペース数 
#define SPACE_OF_INDENT		0


namespace {

	/** 現在のインデント数をカウントするクラス */
	class Indenting
	{
		static int s_count;

	public:
		Indenting()  { s_count++; }
		~Indenting() { s_count--; }
		static int getCount() { return s_count; }
	};

	int Indenting::s_count = 0;


	/** 現在のインデント数に合わせスペースを挿入するマニピュレータ */
	std::ostream& indent(std::ostream& ros)
	{
		std::string s;
		for (int i = 0; i < Indenting::getCount() * SPACE_OF_INDENT; i++) s.append(" ");
		return ros << s;
	}

	int toCanvas2dPartId(int id)
	{
		return id;
	}


	typedef enum {
		kSSPartAlphaBlendMix,
		kSSPartAlphaBlendMultiplication,
		kSSPartAlphaBlendAddition,
		kSSPartAlphaBlendSubtraction
	} SSPartAlphaBlend;

	/** 内部パーツαブレンド方法からCanvasプレイヤー用パーツαブレンド方法に変換 */
	static int toCanvasPartAlphaBlend(SsPart::AlphaBlend ssPartAlphaBlend)
	{
		switch (ssPartAlphaBlend)
		{
			case SsPart::AlphaBlendMix:            return kSSPartAlphaBlendMix;
			case SsPart::AlphaBlendMultiplication: return kSSPartAlphaBlendMultiplication;
			case SsPart::AlphaBlendAddition:       return kSSPartAlphaBlendAddition;
			case SsPart::AlphaBlendSubtraction:    return kSSPartAlphaBlendSubtraction;
			default:                               return kSSPartAlphaBlendMix;
		}
	}

};


static void writeParts(std::ostream& out, textenc::Encoding outEncoding, ss::SsMotion::Ptr motion, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const Canvas2dSaver::Options& options);
static void writeFrameParam(std::ostream& out, const SsMotionFrameDecoder::FrameParam& param, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, int frameNo);
static void writeImageList(std::ostream& out, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const Canvas2dSaver::Options& options);




/**
 * Canvas2dSaver
 */

Canvas2dSaver::Canvas2dSaver(std::ostream& out, textenc::Encoding outEncoding, const Options& options, const std::string& creatorComment)
	: _out(out)
	, _outEncoding(outEncoding)
	, _options(options)
	, _blockWritten(false)
{
	if (_options.isJson) _out << "{" << std::endl;

	// creator情報埋め込み
	if (!_options.isJson)
	{
		_out << "// " << creatorComment << std::endl;
	}
}

Canvas2dSaver::~Canvas2dSaver()
{
    _out << std::endl;
	if (_options.isJson) _out << "}" << std::endl;
}

void Canvas2dSaver::prewriteBlock()
{
	if (_blockWritten)
    {
        if (_options.isJson) _out << ",";
        _out << std::endl;
    }
	_blockWritten = true;
}


/** 
 * ImageListあるとき画像リストを出力 
 */
void Canvas2dSaver::writeImageList(
	ss::SsMotion::Ptr motion, 
	const std::string& prefixLabel
	)
{
	if (motion->hasImageList())
	{
		prewriteBlock();
		::writeImageList(_out, motion->getImageList(), prefixLabel, _options);
	}
}


/**
 * 画像リストを出力 
 */
void Canvas2dSaver::writeImageList(
	ss::SsImageList::ConstPtr imageList, 
	const std::string& prefixLabel
	)
{
	prewriteBlock();
	::writeImageList(_out, imageList, prefixLabel, _options);
}


/**
 * アニメーションデータを出力 
 */
void Canvas2dSaver::writeAnimation(
	ss::SsMotion::Ptr motion, 
	ss::SsImageList::ConstPtr optImageList, 
	const std::string& prefixLabel
	)
{
	ss::SsImageList::ConstPtr imageList = optImageList;
	if (!imageList && motion->hasImageList())
	{
		imageList = motion->getImageList();
	}

	// 各パーツのフレームデータと、fpsなどの基盤情報の出力 
	prewriteBlock();
	::writeParts(_out, _outEncoding, motion, imageList, prefixLabel, _options);
}



static std::string getPartNameList(ss::SsMotion::ConstPtr motion, textenc::Encoding outEncoding, const char* qt)
{
	// ノードをフラットな構造で得る 
	std::vector<SsNode::ConstPtr> nodes = utilities::listTreeNodes(motion->getRootNode());

	std::stringstream ss;
	int partCount = 0;
	BOOST_FOREACH( SsNode::ConstPtr node, nodes )
	{
		std::string encName = textenc::convert(node->getName(), textenc::SHIFT_JIS, outEncoding);
		if (partCount > 0) ss << ",";
		ss << format("%1%%2%%1%") % qt % encName;
		partCount++;
	}
	return ss.str();
}


#if 0

"name": "girl",
"animation": {
"fps": 30,
"parts": ["胴","腰","右足","左足","顔","髪","右テール","左テール","口","目","眉","左腕","右腕","武器"],
"ssa": [
[
[6,0,256,0,128,256,165,107,0.0,1.0,1.0,110,23,1,0,1.0,0,0],

var girl_animation = {
fps: 30,
parts: ['胴','腰','右足','左足','顔','髪','右テール','左テール','口','目','眉','左腕','右腕','武器'],
ssa: [
[
[6,0,256,0,128,256,165,107,0.0,1.0,1.0,110,23,1],

#endif





/** 該当するものはデータ出力から省く */
static bool isRemovePart(const SsMotionFrameDecoder::FrameParam& r)
{
	return SsMotionFrameDecoder::FrameParam::isHidden(r)				// 非表示
		|| SsMotionFrameDecoder::FrameParam::isInvisible(r)				// 完全に透明なパーツ
		|| SsMotionFrameDecoder::FrameParam::isRoot(r)					// ルートパーツ
		|| SsMotionFrameDecoder::FrameParam::isHitTestOrSoundPart(r)	// 当たり判定, サウンドパーツ
		|| SsMotionFrameDecoder::FrameParam::isNullPart(r)				// NULLパーツ
	;
}


/**
 * 各パーツの情報を出力する 
 */
void writeParts(std::ostream& out, textenc::Encoding outEncoding, ss::SsMotion::Ptr motion, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const Canvas2dSaver::Options& options)
{
	const char* qt = options.isJson ? "\"" : "'";

	const char* fmtPrefix = (options.isJson || options.isNoSuffix) ? "%1%" : "%1%_animation";
	const std::string prefix = (format(fmtPrefix) % prefixLabel).str();

	const int fps = motion->getBaseTickTime();
	const std::string nameList = getPartNameList(motion, outEncoding, qt);

	std::stringstream ss;
	ss << std::endl;
	const std::string crlf = ss.str();


	// %1%は改行 
	const char* fmtTop = options.isJson ?
		// json
        "\"name\": \"%2%\","	"%1%"
		"\"animation\": {"      "%1%"
		"\"fps\": %3%,"         "%1%"
		"\"parts\": [%4%],"     "%1%"
		"\"ssa\": ["            "%1%" :
		// normal
		"var %2% = {"           "%1%"
		"fps: %3%,"             "%1%"
		"parts: [%4%],"         "%1%"
		"ssa: ["                "%1%" ;

	out << format(fmtTop) % crlf % prefix % fps % nameList;


	// 各パーツのフレームごとのパラメータ値 
	int outtedFrameCount = 0;
	for (int frameNo = 0; frameNo < motion->getTotalFrame(); frameNo++)
	{
		// このフレームのパラメータを計算する 
		std::vector<SsMotionFrameDecoder::FrameParam> r;
		SsMotionFrameDecoder::decodeNodes(r, motion, frameNo, SsMotionFrameDecoder::InheritCalcuation_Calculate);

		// 優先順位でソート 
		std::sort(r.begin(), r.end(), SsMotionFrameDecoder::FrameParam::priorityComparator);

        // データ出力の必要ないものはリストから削除する
        std::vector<SsMotionFrameDecoder::FrameParam>::iterator removes =
            std::remove_if(r.begin(), r.end(), isRemovePart);
        r.erase(removes, r.end());

        //int partCount = static_cast<int>(r.size());

		if (!r.empty())
		{
			if (outtedFrameCount > 0) out << format(",%1%") % crlf;

			out << format("[%1%") % crlf;

			SsMotionFrameDecoder::FrameParam dummy;

			int partCount = 0;
			BOOST_FOREACH( SsMotionFrameDecoder::FrameParam& param, r )
			{
				Indenting _;

				if (partCount++ > 0) out << "," << crlf;
				out << indent;

                writeFrameParam(out, param, imageList, prefixLabel, frameNo);
			}
			out << crlf;

			out << format("]");

			outtedFrameCount++;
		}
	}


	// %1%は改行 
	const char* fmtBottom = options.isJson ?
		// json
		"%1%"
		"]"		"%1%"
		"}":
		// normal
		"%1%"
		"]"		"%1%"
		"};";

	out << format(fmtBottom) % crlf;
}



namespace 
{

    static SsRect getPicRect(const SsMotionFrameDecoder::FrameParam& param)
    {
        const SsRect& picArea = param.node->getPicArea();
        int left   = picArea.getLeft() + (int)param.imgx.value;
        int top    = picArea.getTop()  + (int)param.imgy.value;
        int right  = left + picArea.getWidth()  + (int)param.imgw.value;
        int bottom = top  + picArea.getHeight() + (int)param.imgh.value;
        return SsRect(left, top, right, bottom);
    }

    static SsRect clipByImageRect(const SsRect& rect, SsImage::ConstPtr image)
    {
        SsRect imageRect(0, 0, image->getSize().getWidth(), image->getSize().getHeight());
        return SsRect::intersect(rect, imageRect);
    }

    static SsPointF getPosition(const SsMotionFrameDecoder::FrameParam& param)
    {
        return SsPointF(
            param.posx.value,
            param.posy.value
            );
    }

    static SsPoint getOrigin(const SsMotionFrameDecoder::FrameParam& param)
    {
        SsPoint origin(param.node->getOrigin());
        origin.x += (int)param.orfx.value;
        origin.y += (int)param.orfy.value;
        return origin;
    }

};


/**
 * １パーツ分のフレーム情報を出力する 
 */
static void writeFrameParam(std::ostream& out, const SsMotionFrameDecoder::FrameParam& param, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, int frameNo)
{
	SsNode::ConstPtr node = param.node;

	SsRect souRect  = getPicRect(param);

	// 画像参照矩形が画像の大きさを超えていないかチェックし、超えていたらクリッピングする 
	if (imageList && node->getPicId() < static_cast<int>(imageList->getImages().size()))
	{
		SsImage::ConstPtr image = imageList->getImages()[node->getPicId()];
		if (image->hasSize())
		{
			SsRect orgRect = souRect;
			souRect = clipByImageRect(souRect, image);
			if (souRect.getWidth() != orgRect.getWidth() || souRect.getHeight() != orgRect.getHeight())
			{
				std::cerr << format("アニメ:%1% パーツ名:%2% フレーム:%3% の参照元基準位置・サイズが画像サイズを超えています")
					% prefixLabel % param.node->getName() % frameNo;
				std::cerr << std::endl;
			}
		}
	}

	SsPointF position = getPosition(param);
	SsPoint origin   = getOrigin(param);

	// 回転角はラジアン 
	const float angle = param.angl.value;

	int flipH = param.flph.value == 0 ? 0 : 1;
	int flipV = param.flpv.value == 0 ? 0 : 1;
	int alphaBlend = toCanvasPartAlphaBlend(node->getAlphaBlend());

	saverutil::ParameterBuffer params;

	params.addInt(toCanvas2dPartId(node->getId()));
	params.addInt(node->getPicId());

	params.addInt(souRect.getLeft());
	params.addInt(souRect.getTop());
	params.addInt(souRect.getWidth());
	params.addInt(souRect.getHeight());
	params.addFloat(position.x);
	params.addFloat(position.y);

	params.addFloat(angle);
	params.addFloat(param.scax.value);
	params.addFloat(param.scay.value);

	// 以降、省略可能パラメータ 
	params.addInt(origin.x, 0);
	params.addInt(origin.y, 0);
	params.addInt(flipH, 0);
	params.addInt(flipV, 0);
	params.addFloat(param.tran.value, 1.0f);
	params.addInt(alphaBlend, 0);
	params.addInt(param.vert.value.v[0].x, 0);	//頂点変形がある場合は出力します。
	params.addInt(param.vert.value.v[0].y, 0);
	params.addInt(param.vert.value.v[1].x, 0);
	params.addInt(param.vert.value.v[1].y, 0);
	params.addInt(param.vert.value.v[2].x, 0);
	params.addInt(param.vert.value.v[2].y, 0);
	params.addInt(param.vert.value.v[3].x, 0);
	params.addInt(param.vert.value.v[3].y, 0);
	

	out << "[";
	out << params.toEllipsisString();
	out << "]";
}



#if 0
"images":["girl.png"]
var girl_images = ['girl.png'];
#endif

/**
 * 画像ソースの情報を出力する 
 */
void writeImageList(std::ostream& out, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const Canvas2dSaver::Options& options)
{
	const char* qt = options.isJson ? "\"" : "'";

	std::stringstream ss;
	bool first = true;
	BOOST_FOREACH( SsImage::ConstPtr image, imageList->getImages() )
	{
		// ファイル名部のみ出力 
		boost::filesystem::path path = image->getPath();
		boost::filesystem::path filename = path.filename();

		if (!first) ss << ",";
		ss << format("%1%%2%%1%") % qt % filename.generic_string();
		first = false;
	}

	const std::string prefix = (format("%1%_images") % prefixLabel).str();

	const char* fmt = options.isJson ?
		"\"images\":[%2%]" :
		"var %1% = [%2%];" ;

	out << format(fmt) % prefix % ss.str();
}


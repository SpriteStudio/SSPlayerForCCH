
#include "CoronaSaver.h"
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/path.hpp>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>

#include "SsMotionDecoder.h"
#include "SsMotionUtil.h"
#include "SaverUtil.h"
#include "MathUtil.h"

using boost::shared_ptr;
using boost::format;
using namespace ss;

// インデントあたりのスペース数 
#define SPACE_OF_INDENT		2


namespace {

	enum {
		SS_USER_DATA_FLAG_NUMBER		= 1 << 0,
		SS_USER_DATA_FLAG_RECT			= 1 << 1,
		SS_USER_DATA_FLAG_POINT			= 1 << 2,
		SS_USER_DATA_FLAG_STRING		= 1 << 3,

		NUM_SS_USER_DATA_FLAGS,

		SS_USER_DATA_FLAGS				= SS_USER_DATA_FLAG_NUMBER |
										  SS_USER_DATA_FLAG_RECT   |
										  SS_USER_DATA_FLAG_POINT  |
										  SS_USER_DATA_FLAG_STRING
	};

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

    int toCoronaPartId(int id)
    {
        return id;
    }


    class ImageRectList
    {
        typedef std::map<SsRect, int> RectMap;
        typedef std::vector<shared_ptr<RectMap> > RectMapList;
        
        RectMapList _list;
        
    public:
        ImageRectList(size_t numImages)
        {
            addImage(static_cast<int>(numImages) - 1);
        }
        
        int append(int imageNo, const SsRect& rect)
        {
            assert(imageNo >= 0);
 
            addImage(imageNo);
            
            RectMap::const_iterator i = _list[imageNo]->find(rect);
            if (i != _list[imageNo]->end())
            {
                return i->second;
            }
            
            int no = static_cast<int>(_list[imageNo]->size());
            (*_list[imageNo])[rect] = no;
            return no;
        }
        
        void addImage(int imageNo)
        {
            size_t n = imageNo + 1;
            while (_list.size() < n)
            {
                _list.push_back(shared_ptr<RectMap>(new RectMap()));
            }
        }
        
        int numImages() const
        {
            return static_cast<int>(_list.size());
        }
        
        std::vector<SsRect> getRectList(int imageNo) const
        {
            assert(imageNo >= 0 && imageNo < static_cast<int>(_list.size()));

            std::vector<SsRect> list;
            list.resize(_list[imageNo]->size());

            for (RectMap::iterator i = _list[imageNo]->begin(), end = _list[imageNo]->end(); i != end; i++)
            {
                list[i->second] = i->first;
            }
            
            return list;
        }
    };

};


static void writeParts(std::ostream& out, textenc::Encoding outEncoding, ss::SsMotion::Ptr motion, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const CoronaSaver::Options& options);
static void writeFrameParam(std::ostream& out, const SsMotionFrameDecoder::FrameParam& param, ImageRectList& imageRectList, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, int frameNo , std::string& udat);
static void writeImageList(std::ostream& out, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const CoronaSaver::Options& options);
static void writeImageRectList(std::ostream& out, const ImageRectList& imageRectList, CoronaSaver::Options& options);



/**
 * CoronaSaver
 */

CoronaSaver::CoronaSaver(std::ostream& out, textenc::Encoding outEncoding, const Options& options, const std::string& creatorComment)
	: _out(out)
	, _outEncoding(outEncoding)
	, _options(options)
	, _blockWritten(false)
{
	// creator情報埋め込み
	out << "-- " << creatorComment << std::endl;
}

CoronaSaver::~CoronaSaver()
{
}

void CoronaSaver::prewriteBlock()
{
	_blockWritten = true;
}


/** 
 * ImageListあるとき画像リストを出力 
 */
void CoronaSaver::writeImageList(
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
void CoronaSaver::writeImageList(
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
void CoronaSaver::writeAnimation(
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



static std::string toBlendModeString(ss::SsPart::AlphaBlend partBlendMode)
{
	switch (partBlendMode)
	{
		case ss::SsPart::AlphaBlendAddition:		return "add";
		case ss::SsPart::AlphaBlendMultiplication:	return "mul";
		case ss::SsPart::AlphaBlendSubtraction:		return "sub";
		case ss::SsPart::AlphaBlendMix:
		default:									return "mix";
	}
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
		if (partCount > 0) ss << ", ";
		std::string blendMode = toBlendModeString(node->getAlphaBlend());
		ss << format("{ name=%1%%2%%1%, imageNo=%3%, blendMode=%1%%4%%1% }") % qt % encName % node->getPicId() % blendMode;
		partCount++;
	}
	return ss.str();
}


#if 0

girl_ssa = {
    fps = 30,
    parts = {'胴','腰','右足','左足','顔','髪','右テール','左テール','口','目','眉','左腕','右腕','武器'},
    ssa = {
        {
            {6,0,256,0,128,256,165,107,0.0,1.0,1.0,110,23,1},
        }
    }

#endif


/** 該当するものはデータ出力から省く */
static bool isRemovePart(const SsMotionFrameDecoder::FrameParam& r)
{
	return SsMotionFrameDecoder::FrameParam::isHidden(r)				// 非表示
		|| SsMotionFrameDecoder::FrameParam::isInvisible(r)				// 完全に透明なパーツ
		|| SsMotionFrameDecoder::FrameParam::isRoot(r)					// ルートパーツ
		|| SsMotionFrameDecoder::FrameParam::isHitTestOrSoundPart(r)	// 当たり判定, サウンドパーツ
		|| SsMotionFrameDecoder::FrameParam::isNullPart(r)				// NULLパーツ
		|| SsMotionFrameDecoder::FrameParam::isScaleZero(r)				// スケール値が０のもの
	;
}


/**
 * 各パーツの情報を出力する 
 */
void writeParts(std::ostream& out, textenc::Encoding outEncoding, ss::SsMotion::Ptr motion, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const CoronaSaver::Options& options)
{
    ImageRectList imageRectList(imageList->getImages().size());
    
	const char* qt = "'";

	const char* fmtPrefix = "%1%_animation";
	const std::string prefix = (format(fmtPrefix) % prefixLabel).str();

	const int fps = motion->getBaseTickTime();
	const std::string nameList = getPartNameList(motion, outEncoding, qt);

	std::stringstream ss;
	ss << std::endl;
	const std::string crlf = ss.str();


	// %1%は改行 
	const char* fmtTop =
		"%2% = {"           "%1%"
		"fps = %3%,"		"%1%"
		"parts = { %4% },"	"%1%"
		"ssa = {"			"%1%" ;

	out << format(fmtTop) % crlf % prefix % fps % nameList;

	std::string udat_string = "";
	// 各パーツのフレームごとのパラメータ値 
	int outtedFrameCount = 0;
	for (int frameNo = 0; frameNo < motion->getTotalFrame(); frameNo++)
	{
		// このフレームのパラメータを計算する 
		std::vector<SsMotionFrameDecoder::FrameParam> r;
		SsMotionFrameDecoder::decodeNodes(r, motion, frameNo, SsMotionFrameDecoder::InheritCalcuation_Calculate, true);

		// 優先順位でソート 
		std::sort(r.begin(), r.end(), SsMotionFrameDecoder::FrameParam::priorityComparator);

        // データ出力の必要ないものはリストから削除する
        std::vector<SsMotionFrameDecoder::FrameParam>::iterator removes =
            std::remove_if(r.begin(), r.end(), isRemovePart);
        r.erase(removes, r.end());

        //int partCount = static_cast<int>(r.size());

		//if (!r.empty())
		{
			if (outtedFrameCount > 0) out << format(",%1%") % crlf;

			out << format("{%1%") % crlf;

            if (!r.empty())
            {
                int partCount = 0;
                BOOST_FOREACH( SsMotionFrameDecoder::FrameParam& param, r )
                {
                    Indenting _;
                    
                    if (partCount++ > 0) out << "," << crlf;
                    out << indent;
                    
                    writeFrameParam(out, param, imageRectList, imageList, prefixLabel, frameNo , udat_string);
                }
                out << crlf;
            }

			out << format("}");

			outtedFrameCount++;
		}
	}


	// %1%は改行 
	const char* fmtCenter =
		"%1%"
        "},"                "%1%"
        "imageOptions = {"	"%1%"
    ;
	out << format(fmtCenter) % crlf;


    static const char* fmtImageOption =
        "{ x = %1%, y = %2%, width = %3%, height = %4% }";

    for (int imageNo = 0; imageNo < imageRectList.numImages(); imageNo++)
    {
        Indenting _;
        if (imageNo > 0) out << "," << crlf;
        out << indent;
        out << format("{%1%") % crlf;
        {
            Indenting __;
            out << indent;
            out << format("frames = {%1%") % crlf;
            
            std::vector<SsRect> rects = imageRectList.getRectList(imageNo);
            if (!rects.empty())
            {
                int rectCount = 0;
                for (std::vector<SsRect>::const_iterator i = rects.begin(), end = rects.end(); i != end; i++)
                {
                    Indenting _;
                    if (rectCount++ > 0) out << "," << crlf;
                    out << indent;
                    
                    out << format(fmtImageOption) % i->getLeft() % i->getTop() % i->getWidth() % i->getHeight();
                }
                out << crlf;
            }

            out << indent;
            out << format("}%1%") % crlf;
        }
        out << indent;
        out << format("}");
    }
	
	if ( udat_string.length() == 0 )
	{
		// %1%は改行
		const char* fmtBottom =
			"%1%"
			"}"     "%1%"
			"}"     "%1%"
			"%1%"
		;
		out << format(fmtBottom) % crlf;
	}else{
		const char* fmtBottom =
			"%1%"
			"},"     "%1%"
			"userdata = { " "%1%"
			"%2%"
			"}" "%1%"
			"}"     "%1%"
			"%1%"
		;
		out << format(fmtBottom) % crlf % udat_string;
	}

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

    static SsPoint getPosition(const SsMotionFrameDecoder::FrameParam& param)
    {
        return SsPoint(
            (int)param.posx.value,
            (int)param.posy.value
            );
    }
    static SsPointF getPositionF(const SsMotionFrameDecoder::FrameParam& param)
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
static void writeFrameParam(std::ostream& out, const SsMotionFrameDecoder::FrameParam& param, ImageRectList& imageRectList, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, int frameNo , std::string& udat)
{
	SsNode::ConstPtr node = param.node;

	SsRect souRect  = getPicRect(param);

#if 0
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
#endif
    
    int imageNo = node->getPicId();
    int rectNo = imageRectList.append(imageNo, souRect);

	SsPointF position = getPositionF(param);
	SsPoint origin   = getOrigin(param);
    // 原点0,0が画像の中心
    origin.x -= souRect.getWidth() / 2;
    origin.y -= souRect.getHeight() / 2;

	// 回転角は度で回転角度が逆
	const float angle = mathutil::radianToDegree(param.angl.value) * -1.0f;

    int flags = 0;
    if (param.flph.value != 0) flags |= 1 << 0;
    if (param.flpv.value != 0) flags |= 1 << 1;

	saverutil::ParameterBuffer params;

	params.addInt(1 + toCoronaPartId(node->getId()));
    params.addInt(flags);
    params.addInt(rectNo);
	params.addFloat(position.x);
	params.addFloat(position.y);

	// 以降、省略可能パラメータ
	params.addInt(origin.x, 0);
	params.addInt(origin.y, 0);
	params.addFloat(angle, 0.0f);
	params.addFloat(param.scax.value, 1.0f);
	params.addFloat(param.scay.value, 1.0f);
	params.addFloat(param.tran.value, 1.0f);

	//params.addInt(0, 0);	// 未対応：頂点変形
	//params.addInt(0, 0);	// 未対応：頂点変形

	saverutil::ParameterBuffer udat_params;
	if (param.udat.value.hasData())
	{
		const SsUserDataValue& value = param.udat.value;
		//少数丸め、整数書式かを合わせるため多少強引な方法をとります。
		if (value.number)
		{
			std::string str = "ni=";
			saverutil::ParameterBuffer udat_params_int;
			udat_params_int.addInt( value.number.get() );
			str+=udat_params_int.toEllipsisString();
			udat_params.addString( str );
		}
		if (value.rect)
		{
			std::string str = "r={";
			saverutil::ParameterBuffer udat_params_rect;

			SsRect rect = value.rect.get();
			udat_params_rect.addInt( rect.getLeft() );
			udat_params_rect.addInt( rect.getTop() );
			udat_params_rect.addInt( rect.getRight() );
			udat_params_rect.addInt( rect.getBottom() );

			str+=udat_params_rect.toEllipsisString();
			str+="}";
			udat_params.addString( str );
		}
		if (value.point)
		{
			std::string str = "p={";
			saverutil::ParameterBuffer udat_params_point;

			SsPoint point = value.point.get();
			udat_params_point.addInt( point.x );
			udat_params_point.addInt( point.y );
			str+=udat_params_point.toEllipsisString();
			str+="}";
			udat_params.addString( str );
		}

		if (value.str)
		{
			std::string str = "s=\"";
			str+=value.str.get();
			str+="\"";
			udat_params.addString( str );
		}
	}
/*
	userdata ={
		["20"] = {ni=-1},
	}
*/
	out << "{ ";
	out << params.toEllipsisString();
	//ユーザーデータ追加 kurooka
	if ( param.udat.value.hasData()  )
	{
		std::string out_str ="[\"";
		out_str += boost::lexical_cast<std::string>(frameNo);
		out_str+="\"]={";
		out_str+=udat_params.toEllipsisString() ;	
		udat+=out_str;
		udat+="},\n";
/*
		out << ",udat={ ";
		out << udat_params.toEllipsisString();
		out << "}";
*/
	}
	out << " }";
}



#if 0
girl_images = {'girl.png'}
#endif

/**
 * 画像ソースの情報を出力する 
 */
void writeImageList(std::ostream& out, ss::SsImageList::ConstPtr imageList, const std::string& prefixLabel, const CoronaSaver::Options& options)
{
	const char* qt = "'";

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

	const char* fmt =
		"%1% = { %2% }" ;

	out << format(fmt) % prefix % ss.str();
	out << std::endl;
}





#include "Cocos2dSaver.h"
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/path.hpp>

#include "SsMotionDecoder.h"
#include "SsMotionUtil.h"
#include "BinaryDataWriter.h"
#include "MathUtil.h"
#include "TextEncoding.h"
#include "DebugUtil.h"
#include <cassert>

using boost::shared_ptr;
using boost::format;
using namespace ss;

// インデントあたりのスペース数
#define SPACE_OF_INDENT		2


namespace {

	static const int		FormatVersion_2 = 2;
	static const int		FormatVersion_3 = 3;		// 2013/10/30 パーツデータに、パーツタイプ、αブレンド方法を追加
	static const int		FormatVersion_4 = 4;		// 2013/11/21 Cocos2d-xでアフィン変換を行うための情報を追加
	static const int		FormatVersion_5 = 5;		// 2014/07/10 X,Y座標の精度をshortからfloatに変更

	static const int		CurrentFormatVersion = FormatVersion_5;




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

	std::string toFloatString(float value)
	{
		return (format("%1$ff") % value).str();
	}

	static unsigned int toId(const std::string& id)
	{
		return static_cast<unsigned int>((id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3]);
	}

	static int toCocos2dPartId(int id)
	{
		return 1 + id;
	}


	typedef enum {
		kSSPartTypeNormal,
		kSSPartTypeNull
	} SSPartType;

	/** 内部パーツタイプからCocos2d-xプレイヤー用パーツタイプに変換 */
	static int toCocos2dPartType(SsPart::Type ssPartType)
	{
		switch (ssPartType)
		{
			case SsPart::TypeNormal: return kSSPartTypeNormal;
			case SsPart::TypeNull:   return kSSPartTypeNull;
			default:                 return kSSPartTypeNull;
		}
	}


	typedef enum {
		kSSPartAlphaBlendMix,
		kSSPartAlphaBlendMultiplication,
		kSSPartAlphaBlendAddition,
		kSSPartAlphaBlendSubtraction
	} SSPartAlphaBlend;

	/** 内部パーツαブレンド方法からCocos2d-xプレイヤー用パーツαブレンド方法に変換 */
	static int toCocos2dPartAlphaBlend(SsPart::AlphaBlend ssPartAlphaBlend)
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

}



enum {
	SS_DATA_FLAG_USE_VERTEX_OFFSET	= 1 << 0,
	SS_DATA_FLAG_USE_COLOR_BLEND	= 1 << 1,
	SS_DATA_FLAG_USE_ALPHA_BLEND	= 1 << 2,
	SS_DATA_FLAG_USE_AFFINE_TRANS	= 1 << 3,

	NUM_SS_DATA_FLAGS
};

enum {
	SS_PART_FLAG_FLIP_H				= 1 << 0,
	SS_PART_FLAG_FLIP_V				= 1 << 1,
	SS_PART_FLAG_INVISIBLE			= 1 << 2,
	
	SS_PART_FLAG_ORIGIN_X			= 1 << 4,
	SS_PART_FLAG_ORIGIN_Y			= 1 << 5,
	SS_PART_FLAG_ROTATION			= 1 << 6,
	SS_PART_FLAG_SCALE_X			= 1 << 7,
	SS_PART_FLAG_SCALE_Y			= 1 << 8,
	SS_PART_FLAG_OPACITY			= 1 << 9,
	SS_PART_FLAG_VERTEX_OFFSET_TL	= 1 << 10,
	SS_PART_FLAG_VERTEX_OFFSET_TR	= 1 << 11,
	SS_PART_FLAG_VERTEX_OFFSET_BL	= 1 << 12,
	SS_PART_FLAG_VERTEX_OFFSET_BR	= 1 << 13,
	SS_PART_FLAG_COLOR				= 1 << 14,
	SS_PART_FLAG_VERTEX_COLOR_TL	= 1 << 15,
	SS_PART_FLAG_VERTEX_COLOR_TR	= 1 << 16,
	SS_PART_FLAG_VERTEX_COLOR_BL	= 1 << 17,
	SS_PART_FLAG_VERTEX_COLOR_BR	= 1 << 18,

	NUM_SS_PART_FLAGS,

	SS_PART_FLAGS_VERTEX_OFFSET		= SS_PART_FLAG_VERTEX_OFFSET_TL |
									  SS_PART_FLAG_VERTEX_OFFSET_TR |
									  SS_PART_FLAG_VERTEX_OFFSET_BL |
									  SS_PART_FLAG_VERTEX_OFFSET_BR,

	SS_PART_FLAGS_COLOR_BLEND		= SS_PART_FLAG_COLOR |
									  SS_PART_FLAG_VERTEX_COLOR_TL |
									  SS_PART_FLAG_VERTEX_COLOR_TR |
									  SS_PART_FLAG_VERTEX_COLOR_BL |
									  SS_PART_FLAG_VERTEX_COLOR_BR
};

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



struct Context
{
	std::ostream&			out;
	BinaryDataWriter		bout;
	const bool				sourceFormatMode;
	const bool				binaryFormatMode;
	const textenc::Encoding	outEncoding;
	const Cocos2dSaver::Options	options;

	const std::string		prefix;
	const std::string		dataBase;
	const std::string		imageDataLabel;
	const std::string		frameDataLabel;
	const std::string		partDataLabel;

	Context(std::ostream& out, bool binaryFormatMode, textenc::Encoding outEncoding, const Cocos2dSaver::Options& options, const std::string& prefix)
		: out(out)
		, bout(out)
		, sourceFormatMode(!binaryFormatMode)
		, binaryFormatMode(binaryFormatMode)
		, outEncoding(outEncoding)
		, options(options)
		
		, prefix(prefix)
		, dataBase((format("%1%_partsData") % prefix).str())
		, imageDataLabel((format("%1%_imageData") % prefix).str())
		, frameDataLabel((format("%1%_frameData") % prefix).str())
		, partDataLabel((format("%1%_partData") % prefix).str())
	{
	}

	~Context()
	{
		if (binaryFormatMode)
		{
			bout.fixReferences();
		}
	}
};

static void writeParts(Context& context, ss::SsMotion::Ptr motion);
static int writeFrameParam(Context& context, const SsMotionFrameDecoder::FrameParam& param, const SsMotionFrameDecoder::FrameParam& parentParam, bool relatively);
static void writeUserData(Context& context, const SsMotionFrameDecoder::FrameParam& param);
static void writeImageList(Context& context, ss::SsImageList::ConstPtr imageList);


/**
 * Cocos2dプレイヤー形式で出力する 
 */
void Cocos2dSaver::save(
	std::ostream& out,
	bool binaryFormatMode,
	textenc::Encoding outEncoding,
	const Options& options,
	ss::SsMotion::Ptr motion,
	ss::SsImageList::ConstPtr optImageList, 
	const std::string& prefixLabel,
	const std::string& creatorComment)
{
	Context context(out, binaryFormatMode, outEncoding, options, prefixLabel);
	
	// データ起点
	if (context.sourceFormatMode)
	{
		// creator情報埋め込み
		context.out << format("// %1%") % creatorComment << std::endl;
		context.out << format("extern SSData %1%;") % context.dataBase << std::endl;
		context.out << std::endl;
	}
	else
	{
		// ヘッダー部を予約しておく
		context.bout.fill(0, 64);
		// creator情報埋め込み
		context.bout.writeString(creatorComment);
		context.bout.align(64);
	}

	// 画像ソースの出力
	SsImageList::ConstPtr imageList = motion->getImageList();
	if (imageList->getImages().empty() && optImageList)
	{
		// SsMotionオブジェクトが持つSsImageListが空だったらoptImageListを使用する 
		imageList = optImageList;
	}
	writeImageList(context, imageList);
	if (context.sourceFormatMode) context.out << std::endl;

	// 各パーツのフレームデータと、fpsなどの基盤情報の出力 
	writeParts(context, motion);
	if (context.sourceFormatMode) context.out << std::endl;
}



/** 表示されないパーツの判定 */
static bool isInvisiblePart(const SsMotionFrameDecoder::FrameParam& r)
{
	return SsMotionFrameDecoder::FrameParam::isHidden(r)				// 非表示
		|| SsMotionFrameDecoder::FrameParam::isInvisible(r)				// 完全に透明なパーツ
		|| SsMotionFrameDecoder::FrameParam::isRoot(r)					// ルートパーツ
		|| SsMotionFrameDecoder::FrameParam::isHitTestOrSoundPart(r)	// 当たり判定, サウンドパーツ
	;
}


/**
 * 各パーツの情報を出力する 
 */
void writeParts(Context& context, ss::SsMotion::Ptr motion)
{
	const bool parentageEnabled = false;

	// フラグ初期化
	int ssDataFlags = 0;
	// デフォルトでは継承の計算を行う
	SsMotionFrameDecoder::InheritCalcuationType inheritCalc = SsMotionFrameDecoder::InheritCalcuation_Calculate;

	// アフィン変換をCocos2d-x側で行う場合
	if (context.options.useTragetAffineTransformation)
	{
		// アフィン変換を行うことを設定
		ssDataFlags |= SS_DATA_FLAG_USE_AFFINE_TRANS;
		// 継承の計算は行わない
		inheritCalc = SsMotionFrameDecoder::InheritCalcuation_NotCalculate;
	}

	// 各パーツのフレームごとのパラメータ値
	std::vector<int> framesPartCounts;
	std::vector<int> framesUserDataCounts;
	for (int frameNo = 0; frameNo < motion->getTotalFrame(); frameNo++)
	{
		// このフレームのパラメータを計算する
		std::vector<SsMotionFrameDecoder::FrameParam> r;
		SsMotionFrameDecoder::decodeNodes(r, motion, frameNo, inheritCalc);

//		if (context.sourceFormatMode)
//		{
//			debug::dumpPrefix(context.out, frameNo);
//			debug::dumpFrameParamList(context.out, r);
//			debug::dumpSuffix(context.out);
//		}

		if (!parentageEnabled)
		{
			// 優先順位でソート 
			std::sort(r.begin(), r.end(), SsMotionFrameDecoder::FrameParam::priorityComparator);
		}
		

		// ユーザーデータが含まれているものだけ抜き出す
		std::vector<SsMotionFrameDecoder::FrameParam> userDataInc;
		BOOST_FOREACH( const SsMotionFrameDecoder::FrameParam& v, r )
		{
			if (v.udat.value.hasData())
			{
				userDataInc.push_back(v);
			}
		}
		
		// このフレームのユーザーデータを出力する
		if (!userDataInc.empty())
		{
			std::string label = (format("%1%_userData_%2%") % context.prefix % frameNo).str();
		
			if (context.sourceFormatMode)
			{
				context.out << format("static const ss_u16 %1%[] = {") % label;
				context.out << std::endl;
			}
			else
			{
				context.bout.setReference(label);
			}

			bool first = true;
			BOOST_FOREACH( const SsMotionFrameDecoder::FrameParam& v, userDataInc )
			{
				Indenting _;

				if (context.sourceFormatMode)
				{
					if (!first) context.out << "," << std::endl;
					first = false;
					context.out << indent;
				}

				writeUserData(context, v);
			}
		
			if (context.sourceFormatMode)
			{
				context.out << std::endl;

				context.out << "};";
				context.out << std::endl;
			}
		}
		framesUserDataCounts.push_back(static_cast<int>(userDataInc.size()));


		// 継承計算を行う場合、表示されないものはリストから削除する
		if (!context.options.useTragetAffineTransformation)
		{
			std::vector<SsMotionFrameDecoder::FrameParam>::iterator removes =
				std::remove_if(r.begin(), r.end(), isInvisiblePart);
			r.erase(removes, r.end());
		}
        
		int partCount = static_cast<int>(r.size());
		framesPartCounts.push_back(partCount);

		if (!r.empty())
		{
			std::string label = (format("%1%_partFrameData_%2%") % context.prefix % frameNo).str();
		
			if (context.sourceFormatMode)
			{
				context.out << format("static const ss_u16 %1%[] = {") % label;
				context.out << std::endl;
			}
			else
			{
				context.bout.setReference(label);
			}

			SsMotionFrameDecoder::FrameParam dummy;

			int partCount = 0;
			BOOST_FOREACH( SsMotionFrameDecoder::FrameParam& param, r )
			{
				Indenting _;

				if (context.sourceFormatMode)
				{
					if (partCount > 0) context.out << "," << std::endl;
					context.out << indent;
				}
				partCount++;

				if (param.node->isRoot())
				{
					ssDataFlags |= writeFrameParam(context, param, dummy, parentageEnabled);
				}
				else
				{
					//int paramIndex = toCocos2dPartId(param.node->getParentId());
					ssDataFlags |= writeFrameParam(context, param, dummy, parentageEnabled);
				}
			}
			
			if (context.sourceFormatMode)
			{
				context.out << std::endl;

				context.out << "};";
				context.out << std::endl;
			}
		}
	}


	// フレームごとのパラメータ値を参照するためのインデックス 
	if (context.sourceFormatMode)
	{
		context.out << format("static const SSFrameData %1%[] = {") % context.frameDataLabel;
		context.out << std::endl;
	}
	else
	{
		context.bout.setReference(context.frameDataLabel);
	}
	
	for (int frameNo = 0; frameNo < motion->getTotalFrame(); frameNo++)
	{
		Indenting _;

		if (context.sourceFormatMode)
		{
			if (frameNo > 0) context.out << "," << std::endl;
			context.out << indent;
		}

		//typedef struct {
		//	ss_offset	partFrameData;
		//	ss_offset	userData;
		//	ss_s16		numParts;
		//	ss_s16		numUserData;
		//} SSFrameData;

		int partCount = framesPartCounts.at(frameNo);
		int userDataCount = framesUserDataCounts.at(frameNo);
		
		std::string partFrameDataLabel = (format("%1%_partFrameData_%2%") % context.prefix % frameNo).str();
		std::string userDataLabel = (format("%1%_userData_%2%") % context.prefix % frameNo).str();

		if (context.sourceFormatMode)
		{
			context.out << "{ ";

			if (partCount)
			{
				context.out << format("(ss_offset)((char*)%1% - (char*)&%2%)") % partFrameDataLabel % context.dataBase;
			}
			else
			{
				context.out << "0";
			}

			context.out << ", ";
			
			if (userDataCount)
			{
				context.out << format("(ss_offset)((char*)%1% - (char*)&%2%)") % userDataLabel % context.dataBase;
			}
			else
			{
				context.out << "0";
			}
		
			context.out << format(", %1%, %2%") % partCount % userDataCount;
			context.out << " }";
		}
		else
		{
			if (partCount)
			{
				context.bout.writeReference(partFrameDataLabel);
			}
			else
			{
				context.bout.writeInt(0);
			}

			if (userDataCount)
			{
				context.bout.writeReference(userDataLabel);
			}
			else
			{
				context.bout.writeInt(0);
			}
		
			context.bout.writeShort(partCount);
			context.bout.writeShort(userDataCount);
		}
	}

	if (context.sourceFormatMode)
	{
		context.out << std::endl;
		context.out << "};";
		context.out << std::endl;
	}


	std::vector<SsNode::ConstPtr> nodes = utilities::listTreeNodes(motion->getRootNode());

	// パーツ名 
	{
		int partCount = 0;
		BOOST_FOREACH( SsNode::ConstPtr node, nodes )
		{
			std::string label = (format("%1%_partName%2%") % context.prefix % partCount).str();
			std::string encodedName = textenc::convert(node->getName(), textenc::SHIFT_JIS, context.outEncoding);
			if (context.sourceFormatMode)
			{
				context.out << format("static const char %1%[] = \"%2%\";") % label % encodedName;
				context.out << std::endl;
			}
			else
			{
				context.bout.setReference(label);
				context.bout.writeString(encodedName);
			}
			partCount++;
		}
	}

	// パーツごとの基本情報 
	{
		if (context.sourceFormatMode)
		{
			context.out << format("static const SSPartData %1%[] = {") % context.partDataLabel;
			context.out << std::endl;
		}
		else
		{
			context.bout.setReference(context.partDataLabel);
		}

		int partCount = 0;
		BOOST_FOREACH( SsNode::ConstPtr node, nodes )
		{
			Indenting _;

			std::string label = (format("%1%_partName%2%") % context.prefix % partCount).str();
			int id = toCocos2dPartId(node->getId());
			int parentId = toCocos2dPartId(node->getParentId());
			int imageNo = node->getPicId();
			int type = toCocos2dPartType(node->getType());
			int alphaBlend = toCocos2dPartAlphaBlend(node->getAlphaBlend());
			
			if (alphaBlend != kSSPartAlphaBlendMix)
			{
				ssDataFlags |= SS_DATA_FLAG_USE_ALPHA_BLEND;
			}

			//typedef struct {
			//	ss_offset	name;
			//	ss_s16		id;
			//	ss_s16		parentId;
			//	ss_s16		imageNo;
			//	ss_u16		type;			// enum SSPartType
			//	ss_u16		alphaBlend;		// enum SSPartAlphaBlend
			//	ss_s16		reserved;
			//} SSPartData;

			if (context.sourceFormatMode)
			{
				if (partCount > 0) context.out << "," << std::endl;
				context.out << indent;
				context.out << "{ ";
				context.out << format("(ss_offset)((char*)%1% - (char*)&%2%)") % label % context.dataBase;
				context.out << format(", %1%, %2%, %3%, %4%, %5%") % id % parentId % imageNo % type % alphaBlend;
				context.out << " }";
			}
			else
			{
				context.bout.writeReference(label);
				context.bout.writeShort(id);
				context.bout.writeShort(parentId);
				context.bout.writeShort(imageNo);
				context.bout.writeShort(type);
				context.bout.writeShort(alphaBlend);
				context.bout.writeShort(0);
			}
			partCount++;
		}

		if (context.sourceFormatMode)
		{
			context.out << std::endl;
			context.out << "};";
			context.out << std::endl;
		}
	}


	// すべての情報を束ねるデータ本体 
	const unsigned int version = CurrentFormatVersion;

	const unsigned int id0 = 0xffffffff;
	const unsigned int id1 = toId("SSBA");
	const unsigned int flags = ssDataFlags;
	int fps = motion->getBaseTickTime();
	int numParts = motion->getRootNode()->countTreeNodes();
	int numFrames = motion->getTotalFrame();

	//typedef struct {
	//	ss_u32		id[2];
	//	ss_u32		version;
	//	ss_u32		flags;
	//	ss_offset	partData;
	//	ss_offset	frameData;
	//	ss_offset	imageData;
	//	ss_s16		numParts;
	//	ss_s16		numFrames;
	//	ss_s16		fps;
	//} SSData;

	if (context.sourceFormatMode)
	{
		context.out << format("SSData %1% = {") % context.dataBase;
		context.out << std::endl;
		{
			Indenting _;

			context.out << indent << format("{0x%1$08x, 0x%2$08x},") % id0 % id1 << std::endl;
			context.out << indent << format("%1%,") % version << std::endl;
			context.out << indent << format("%1%,") % flags << std::endl;

			context.out << indent << format("(ss_offset)((char*)%1% - (char*)&%2%),") % context.partDataLabel % context.dataBase << std::endl;
			context.out << indent << format("(ss_offset)((char*)%1% - (char*)&%2%),") % context.frameDataLabel % context.dataBase << std::endl;
			context.out << indent << format("(ss_offset)((char*)%1% - (char*)&%2%),") % context.imageDataLabel % context.dataBase << std::endl;

			context.out << indent << format("%1%,") % numParts << std::endl;
			context.out << indent << format("%1%,") % numFrames << std::endl;
			context.out << indent << format("%1%") % fps << std::endl;

			context.out << "};";
			context.out << std::endl;
		}
	}
	else
	{
		// ファイル先頭に書き込む
		context.bout.seekp(0);
		
		context.bout.writeInt(id0);
		context.bout.writeInt(id1);
		context.bout.writeInt(version);
		context.bout.writeInt(flags);

		context.bout.writeReference(context.partDataLabel);
		context.bout.writeReference(context.frameDataLabel);
		context.bout.writeReference(context.imageDataLabel);

		context.bout.writeShort(numParts);
		context.bout.writeShort(numFrames);
		context.bout.writeShort(fps);
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
	
	static unsigned int calcBlendColor(const SsColor& color)
	{
		return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
	}
	
	

	class DataWriter
	{
		Context&	_context;
	public:
		DataWriter(Context& context) : _context(context) {}

		void writeShort(int data, bool addAheadComma = true)
		{
			if (_context.sourceFormatMode)
			{
				unsigned short v = static_cast<unsigned short>(data);
			
				if (addAheadComma) _context.out << ",";
				_context.out << std::showbase << std::hex;
				_context.out << v;
				_context.out << std::noshowbase;
			}
			else
			{
				_context.bout.writeShort(data);
			}
		}
		
		void writeInt(int data, bool addAheadComma = true)
		{
			if (_context.sourceFormatMode)
			{
				unsigned short u = static_cast<unsigned short>(data >> 16);
				unsigned short l = static_cast<unsigned short>(data);
			
				if (addAheadComma) _context.out << ",";
				_context.out << std::showbase << std::hex;
				_context.out << l << "," << u;
				_context.out << std::noshowbase;
			}
			else
			{
				_context.bout.writeInt(data);
			}
		}
		
		void writeFloat(float data, bool addAheadComma = true)
		{
			union {
				float	f;
				int		i;
			} c;
			c.f = data;
			writeInt(c.i);
		}

		void writeShortPoint(const SsPoint& data, bool addAheadComma = true)
		{
			writeShort(data.x, addAheadComma);
			writeShort(data.y);
		}

		void writeIntPoint(const SsPoint& data, bool addAheadComma = true)
		{
			writeInt(data.x, addAheadComma);
			writeInt(data.y);
		}

		void writeIntRect(const SsRect& data, bool addAheadComma = true)
		{
			writeInt(data.getLeft(), addAheadComma);
			writeInt(data.getTop());
			writeInt(data.getRight());
			writeInt(data.getBottom());
		}
		
		void writeString(const std::string& data, bool addAheadComma = true)
		{
			// 内部コード(SJIS)から指定された文字コードにエンコードし直す
			std::string str = textenc::convert(data, textenc::SHIFT_JIS, _context.outEncoding);

			// 一端テンポラリにデータを準備し、それを書き込む
			size_t strAndNullSize = str.size() + 1;		// +1はnull終端分
			size_t bufSize = (strAndNullSize + 1) & ~1;
			assert(bufSize % 1 == 0);

			unsigned char* buf = new unsigned char[bufSize];
			std::memset(buf, 0, bufSize);
			std::memcpy(buf, str.c_str(), str.size());

			// 文字列バイト数を出力
			writeShort(static_cast<short>(str.size()), addAheadComma);
			
			// 文字列本体を出力
			unsigned short* sbuf = reinterpret_cast<unsigned short*>(buf);
			size_t n = bufSize / 2;
			for (size_t i = 0; i < n; i++)
			{
				writeShort(*sbuf++);
			}

			delete [] buf;
		}
	};

};


/**
 * １パーツ分のフレーム情報を出力する 
 */
static int writeFrameParam(Context& context, const SsMotionFrameDecoder::FrameParam& param, const SsMotionFrameDecoder::FrameParam& parentParam, bool parentageEnabled)
{
	SsNode::ConstPtr node = param.node;

	SsRect   souRect  = getPicRect(param);
	SsPointF position = getPosition(param);
	SsPoint  origin   = getOrigin(param);

#if 0   // ※Cocos2dの親子階層に沿う形でコンバートする要望が出たときに使えるかもしれないコード
	if (parentageEnabled && !param.node->isRoot())
	{
		SsRect  parentSouRect  = getPicRect(parentParam);
		SsPoint parentPosition = getPosition(parentParam);
		SsPoint parentOrigin   = getOrigin(parentParam);

		// 親の左下から親の中心点までの距離
		int ox = parentOrigin.x;
		int oy = parentSouRect.getHeight() - parentOrigin.y;

		position.x += ox;
		position.y = -position.y + oy;
	}
#endif

	origin.y = souRect.getHeight() - origin.y;
	const int opacity = static_cast<int>(255 * param.tran.value);
	const float angle = mathutil::radianToDegree(param.angl.value);

	// 出力が必要な要素を判断
	unsigned int flags = 0;
	if (param.flph.value != 0)	flags |= SS_PART_FLAG_FLIP_H;
	if (param.flpv.value != 0)	flags |= SS_PART_FLAG_FLIP_V;
	if (isInvisiblePart(param))	flags |= SS_PART_FLAG_INVISIBLE;
	if (origin.x != souRect.getWidth()/2)  flags |= SS_PART_FLAG_ORIGIN_X;
	if (origin.y != souRect.getHeight()/2) flags |= SS_PART_FLAG_ORIGIN_Y;
	if (angle != 0)				flags |= SS_PART_FLAG_ROTATION;
	if (param.scax.value != 1)	flags |= SS_PART_FLAG_SCALE_X;
	if (param.scay.value != 1)	flags |= SS_PART_FLAG_SCALE_Y;
	if (opacity < 255)			flags |= SS_PART_FLAG_OPACITY;
	
	if (!param.vert.value.v[SS_VERTEX_TOP_LEFT].isZero())		flags |= SS_PART_FLAG_VERTEX_OFFSET_TL;
	if (!param.vert.value.v[SS_VERTEX_TOP_RIGHT].isZero())		flags |= SS_PART_FLAG_VERTEX_OFFSET_TR;
	if (!param.vert.value.v[SS_VERTEX_BOTTOM_LEFT].isZero())	flags |= SS_PART_FLAG_VERTEX_OFFSET_BL;
	if (!param.vert.value.v[SS_VERTEX_BOTTOM_RIGHT].isZero())	flags |= SS_PART_FLAG_VERTEX_OFFSET_BR;

	if (param.pcol.value.blend != SsColorBlendValue::BLEND_VOID)
	{
		if (param.pcol.value.type == SsColorBlendValue::TYPE_COLORTYPE_PARTS)
		{
			if (param.pcol.value.colors[SS_VERTEX_TOP_LEFT].a > 0)		flags |= SS_PART_FLAG_COLOR;
		}
		else if (param.pcol.value.type == SsColorBlendValue::TYPE_COLORTYPE_VERTEX)
		{
			if (param.pcol.value.colors[SS_VERTEX_TOP_LEFT].a > 0)		flags |= SS_PART_FLAG_VERTEX_COLOR_TL;
			if (param.pcol.value.colors[SS_VERTEX_TOP_RIGHT].a > 0)		flags |= SS_PART_FLAG_VERTEX_COLOR_TR;
			if (param.pcol.value.colors[SS_VERTEX_BOTTOM_LEFT].a > 0)	flags |= SS_PART_FLAG_VERTEX_COLOR_BL;
			if (param.pcol.value.colors[SS_VERTEX_BOTTOM_RIGHT].a > 0)	flags |= SS_PART_FLAG_VERTEX_COLOR_BR;
		}
	}


	// 各要素を出力する
	DataWriter w(context);
	
	w.writeInt(flags, false);
	w.writeShort(toCocos2dPartId(node->getId()));
	w.writeShort(souRect.getLeft());
	w.writeShort(souRect.getTop());
	w.writeShort(souRect.getWidth());
	w.writeShort(souRect.getHeight());
	w.writeFloat(position.x);
	w.writeFloat(position.y);

	if (flags & SS_PART_FLAG_ORIGIN_X) w.writeShort(origin.x);
	if (flags & SS_PART_FLAG_ORIGIN_Y) w.writeShort(origin.y);
	if (flags & SS_PART_FLAG_ROTATION) w.writeFloat(angle);
	if (flags & SS_PART_FLAG_SCALE_X) w.writeFloat(param.scax.value);
	if (flags & SS_PART_FLAG_SCALE_Y) w.writeFloat(param.scay.value);
	if (flags & SS_PART_FLAG_OPACITY) w.writeShort(opacity);
	if (flags & SS_PART_FLAG_VERTEX_OFFSET_TL) w.writeShortPoint(param.vert.value.v[SS_VERTEX_TOP_LEFT]);
	if (flags & SS_PART_FLAG_VERTEX_OFFSET_TR) w.writeShortPoint(param.vert.value.v[SS_VERTEX_TOP_RIGHT]);
	if (flags & SS_PART_FLAG_VERTEX_OFFSET_BL) w.writeShortPoint(param.vert.value.v[SS_VERTEX_BOTTOM_LEFT]);
	if (flags & SS_PART_FLAG_VERTEX_OFFSET_BR) w.writeShortPoint(param.vert.value.v[SS_VERTEX_BOTTOM_RIGHT]);

	if (flags & SS_PART_FLAGS_COLOR_BLEND)
	{
		int blendNo = 0;
		switch (param.pcol.value.blend)
		{
			case SsColorBlendValue::BLEND_MIX:		blendNo = 0; break;
			case SsColorBlendValue::BLEND_MULTIPLE:	blendNo = 1; break;
			case SsColorBlendValue::BLEND_ADD:		blendNo = 2; break;
			case SsColorBlendValue::BLEND_SUBTRACT:	blendNo = 3; break;
			default:
				throw std::logic_error("Not blend type");
		}
		w.writeShort(blendNo);

		if (flags & SS_PART_FLAG_COLOR) w.writeInt(calcBlendColor(param.pcol.value.colors[0]));
		if (flags & SS_PART_FLAG_VERTEX_COLOR_TL) w.writeInt(calcBlendColor(param.pcol.value.colors[SS_VERTEX_TOP_LEFT]));
		if (flags & SS_PART_FLAG_VERTEX_COLOR_TR) w.writeInt(calcBlendColor(param.pcol.value.colors[SS_VERTEX_TOP_RIGHT]));
		if (flags & SS_PART_FLAG_VERTEX_COLOR_BL) w.writeInt(calcBlendColor(param.pcol.value.colors[SS_VERTEX_BOTTOM_LEFT]));
		if (flags & SS_PART_FLAG_VERTEX_COLOR_BR) w.writeInt(calcBlendColor(param.pcol.value.colors[SS_VERTEX_BOTTOM_RIGHT]));
	}


	int resultFlags = 0;
	if (flags & SS_PART_FLAGS_VERTEX_OFFSET) resultFlags |= SS_DATA_FLAG_USE_VERTEX_OFFSET;
	if (flags & SS_PART_FLAGS_COLOR_BLEND) resultFlags |= SS_DATA_FLAG_USE_COLOR_BLEND;
	return resultFlags;
}


/**
 * ユーザーデータを出力する
 */
static void writeUserData(Context& context, const SsMotionFrameDecoder::FrameParam& param)
{
	SsNode::ConstPtr node = param.node;
	
	// 出力が必要な要素を判断
	unsigned int flags = 0;
	if (param.udat.value.hasData())
	{
		const SsUserDataValue& value = param.udat.value;
		if (value.number)	flags |= SS_USER_DATA_FLAG_NUMBER;
		if (value.rect)		flags |= SS_USER_DATA_FLAG_RECT;
		if (value.point)	flags |= SS_USER_DATA_FLAG_POINT;
		if (value.str)		flags |= SS_USER_DATA_FLAG_STRING;
	}

	// 各要素を出力する
	DataWriter w(context);
	
	w.writeShort(flags, false);
	w.writeShort(toCocos2dPartId(node->getId()));
	
	if (flags & SS_USER_DATA_FLAGS)
	{
		const SsUserDataValue& value = param.udat.value;
		if (flags & SS_USER_DATA_FLAG_NUMBER) w.writeInt(value.number.get());
		if (flags & SS_USER_DATA_FLAG_RECT)   w.writeIntRect(value.rect.get());
		if (flags & SS_USER_DATA_FLAG_POINT)  w.writeIntPoint(value.point.get());
		if (flags & SS_USER_DATA_FLAG_STRING) w.writeString(value.str.get());
	}
}


#if 0
static const char girl_images_0[] = "girl.png";
const char* girl_images[] = {
  girl_images_0
};
#endif

/**
 * 画像ソースの情報を出力する 
 */
void writeImageList(Context& context, ss::SsImageList::ConstPtr imageList)
{
	// ファイル名文字列定義
	BOOST_FOREACH( SsImage::ConstPtr image, imageList->getImages() )
	{
		int index = image->getId();
		std::string label = (format("%1%_image_%2%") % context.prefix % index).str();
		boost::filesystem::path path = image->getPath();
		boost::filesystem::path filename = path;
		if (!context.options.notModifyImagePath)
		{
			// ファイル名部のみ出力
			filename = path.filename();
		}

		if (context.sourceFormatMode)
		{
			context.out << format("static const char %1%[] = \"%2%\";") % label % filename.generic_string();
			context.out << std::endl;
		}
		else
		{
			context.bout.setReference(label);
			context.bout.writeString(filename.generic_string());
		}
	}

	// 旧ファイル名テーブル（旧バージョン同様の扱い方ができるよう残している）
	if (context.sourceFormatMode)
	{
		context.out << format("const char* %1%_images[] = {") % context.prefix;
		context.out << std::endl;
		{
			Indenting _;
			BOOST_FOREACH( SsImage::ConstPtr image, imageList->getImages() )
			{
				int index = image->getId();
				std::string label = (format("%1%_image_%2%") % context.prefix % index).str();

				context.out << indent;
				context.out << format("%1%,") % label;
				context.out << std::endl;
			}
			context.out << indent;
			context.out << 0;
			context.out << std::endl;
		}
		context.out << "};";
		context.out << std::endl;
	}

	// SSData用のファイル名テーブル
	if (context.sourceFormatMode)
	{
		context.out << format("static const ss_offset %1%[] = {") % context.imageDataLabel;
		context.out << std::endl;
	}
	else
	{
		context.bout.setReference(context.imageDataLabel);
	}

	{
		Indenting _;
		BOOST_FOREACH( SsImage::ConstPtr image, imageList->getImages() )
		{
			int index = image->getId();
			std::string label = (format("%1%_image_%2%") % context.prefix % index).str();

			if (context.sourceFormatMode)
			{
				context.out << indent;
				context.out << format("(ss_offset)((char*)%1% - (char*)&%2%),") % label % context.dataBase;
				context.out << std::endl;
			}
			else
			{
				context.bout.writeReference(label);
			}
		}

		if (context.sourceFormatMode)
		{
			context.out << indent;
			context.out << 0;
			context.out << std::endl;
		}
		else
		{
			context.bout.writeInt(0);
		}
	}

	if (context.sourceFormatMode)
	{
		context.out << "};";
		context.out << std::endl;
	}
}


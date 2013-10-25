
#include "SsaxLoader.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include "XmlUtil.h"
#include "TextEncoding.h"
#include "FileUtil.h"
#include "MathUtil.h"

using namespace ss;
using boost::property_tree::ptree;
using boost::optional;
using boost::shared_ptr;


static shared_ptr<SsMotion::Param> readHeader(const ptree& pt);
static std::vector<SsImage::Ptr> readImageList(const ptree& pt);
static shared_ptr<SsImage> readImage(const ptree& pt);
static shared_ptr<SsPart> readPart(const ptree& pt, textenc::Encoding encoding, int motionEndFrameNo);
static shared_ptr<SsAttribute> readAttribute(const ptree& pt, textenc::Encoding encoding);
static shared_ptr<SsKeyframe> readKey(const ptree& pt, const SsAttributeTag::Tag& tag, textenc::Encoding encoding);
static bool readColors(SsColor& color, const ptree& pt);
static bool readPoint(SsPoint& point, const ptree& pt);
static bool readRect(SsRect& rect, const ptree& pt, bool normalizeEnabled);



/**
 * ssaxファイルをロード
 */
ss::SsMotion::Ptr SsaxLoader::load(const std::string& filename, ResultCode* outResult)
{
    ResultCode result = SUCCESS;

	do
	{
		// XMLのエンコーディング判定
		std::string enc = xmlutil::getEncoding(filename);
		textenc::Encoding encoding = textenc::findEncoding(enc);

		// ssaxをxmlとしてパース
		ptree pt;
        try {
            read_xml(filename, pt);
        } catch (boost::property_tree::xml_parser_error& /*ex*/) {
            result = XML_PARSE_ERROR;
            break;
        }

		shared_ptr<SsMotion::Param> param;
		std::vector<SsImage::Ptr> images;
		BOOST_FOREACH( const ptree::value_type& v, pt.get_child("SpriteStudioMotion") )
		{
			const std::string& key = v.first;
			if (key == "Header")
			{
				param = readHeader(v.second);
			}
			else if (key == "ImageList")
			{
				images = readImageList(v.second);
			}
		}
		// <Header>部の読み取り失敗 
		if (!param) break;

		// モーション全体の最終フレーム番号 
		int motionEndFrameNo = param->endFrameNo;

		std::vector<SsPart::Ptr> parts;
		BOOST_FOREACH( const ptree::value_type& v, pt.get_child("SpriteStudioMotion.Parts") )
		{
			if (v.first == "Part")
			{
				const ptree& part = v.second;
				shared_ptr<SsPart> sspart = readPart(part, encoding, motionEndFrameNo);
				parts.push_back(sspart);
			}
		}

		SsNode::Ptr rootNode = SsNode::createNodes(parts);
		SsImageList::Ptr imageList = SsImageList::Ptr(new SsImageList(images));

        if (outResult) *outResult = result;
		return boost::shared_ptr<ss::SsMotion>(
			new SsMotion(rootNode, *param, imageList));

	} while (false);

    if (outResult) *outResult = result;
	return boost::shared_ptr<ss::SsMotion>();
}



/**
 * <Header>部の読み取り
 */
static shared_ptr<SsMotion::Param> readHeader(const ptree& pt)
{
	do
	{
		shared_ptr<SsMotion::Param> param = shared_ptr<SsMotion::Param>(new SsMotion::Param());

		// ※とりあえず必要なパラメータのみ取得しています 

		// 必須(?)パラメータ 
		optional<int> endFrameNo = pt.get_optional<int>("EndFrame");
		if (!endFrameNo) break;
		optional<int> baseTickTime = pt.get_optional<int>("BaseTickTime");
		if (!baseTickTime) break;

		param->endFrameNo = endFrameNo.get();
		param->baseTickTime = baseTickTime.get();

		return param;

	} while (false);
	return shared_ptr<SsMotion::Param>();
}


/**
 * <ImageList>部の読み取り
 */
static std::vector<SsImage::Ptr> readImageList(const ptree& pt)
{
	do
	{
		std::vector<SsImage::Ptr> images;

		// <Image>群の読み込み 
		BOOST_FOREACH( const ptree::value_type& v, pt.get_child("") )
		{
			if (v.first == "Image")
			{
				const ptree& key = v.second;
				shared_ptr<SsImage> image = readImage(key);
				if (!image) break;
				images.push_back(image);
			}
		}

		return images;

	} while (false);
	return std::vector<SsImage::Ptr>();
}


/**
 * <Image>からSsImageを構築する 
 */
static shared_ptr<SsImage> readImage(const ptree& pt)
{
	do
	{
		// <Image Id="1" Path=".\number.png" Width="512" Height="512" Bpp="8"/>

		// 必須パラメータ 
		optional<int> id = pt.get_optional<int>("<xmlattr>.Id");
		if (!id) break;
		optional<std::string> path = pt.get_optional<std::string>("<xmlattr>.Path");
		if (!path) break;
		optional<int> width = pt.get_optional<int>("<xmlattr>.Width");
		if (!width) break;
		optional<int> height = pt.get_optional<int>("<xmlattr>.Height");
		if (!height) break;
		optional<int> bpp = pt.get_optional<int>("<xmlattr>.Bpp");
		if (!bpp) break;

        // バックスラッシュを/に置き換える
        std::string npath = FileUtil::replaceBackslash(path.get());

		SsSize size(width.get(), height.get());

		return shared_ptr<SsImage>(new SsImage(id.get(), npath, size, bpp.get()));

	} while (false);
	return shared_ptr<SsImage>();
}


/**
 * <Part>からパーツを構築する 
 */
static shared_ptr<SsPart> readPart(const ptree& pt, textenc::Encoding encoding, int motionEndFrameNo)
{
	do
	{
		shared_ptr<SsPart> part = shared_ptr<SsPart>(new SsPart());

		// 必須パラメータ 
		optional<int> type = pt.get_optional<int>("Type");
		if (!type) break;

		// オプションパラメータ 
		optional<int> root = pt.get_optional<int>("<xmlattr>.Root");
		optional<std::string> name = pt.get_optional<std::string>("Name");
		optional<int> id = pt.get_optional<int>("ID");
		optional<int> parentId = pt.get_optional<int>("ParentID");

		optional<int> picId = pt.get_optional<int>("PicID");
		optional<int> picAreaLeft = pt.get_optional<int>("PictArea.Left");
		optional<int> picAreaTop = pt.get_optional<int>("PictArea.Top");
		optional<int> picAreaRight = pt.get_optional<int>("PictArea.Right");
		optional<int> picAreaBottom = pt.get_optional<int>("PictArea.Bottom");
		optional<int> originX = pt.get_optional<int>("OriginX");
		optional<int> originY = pt.get_optional<int>("OriginY");

		optional<int> transBlendType = pt.get_optional<int>("TransBlendType");
		optional<int> inheritType = pt.get_optional<int>("InheritType");


		part->type = static_cast<SsPart::Type>(type.get());

		if (name)
		{
			std::string n = textenc::convert(name.get(), encoding, textenc::SHIFT_JIS);
			part->name = n;
		}
		if (id) part->id = id.get();
		if (parentId) part->parentId = parentId.get();

		if (picId) part->picId = picId.get();
		if (picAreaLeft && picAreaTop && picAreaRight && picAreaBottom)
		{
			part->picArea = SsRect(
				picAreaLeft.get(), 
				picAreaTop.get(),
				picAreaRight.get(),
				picAreaBottom.get());
		}
		if (originX && originY)
		{
			part->origin = SsPoint(
				originX.get(),
				originY.get());
		}
		if (inheritType)
		{
			part->inheritEach = inheritType.get() != 0;
		}

		std::vector<SsAttribute::Ptr> attributes;
		optional<const ptree&> attrs = pt.get_child_optional("Attributes");
		if (attrs)
		{
			BOOST_FOREACH( const ptree::value_type& v2, pt.get_child("Attributes") )
			{
				if (v2.first == "Attribute")
				{
					const ptree& attr = v2.second;
					shared_ptr<SsAttribute> attribute = readAttribute(attr, encoding);
					if (!attribute) continue;
					attributes.push_back(attribute);
				}
			}
		}
		part->attributes = SsAttributes(attributes, motionEndFrameNo);

		return part;

	} while (false);
	return shared_ptr<SsPart>();
}


/**
 * <Attribute>からアニメーションアトリビュートを構築する 
 */
static shared_ptr<SsAttribute> readAttribute(const ptree& pt, textenc::Encoding encoding)
{
	do
	{
		// 必須パラメータ 
		optional<std::string> tagStr = pt.get_optional<std::string>("<xmlattr>.Tag");
		if (!tagStr) break;

		// Tag文字列からタグを求める 
		SsAttributeTag::Tag tag = SsAttributeTag::toEnum(tagStr.get());
		if (tag == SsAttributeTag::Unknown) break;

		// オプションパラメータ 
		optional<float> inheritedPercent = pt.get_optional<float>("<xmlattr>.Inherit");

		// <Key>群の読み込み 
		std::vector<SsKeyframe> keyframes;
		BOOST_FOREACH( const ptree::value_type& v, pt.get_child("") )
		{
			if (v.first == "Key")
			{
				const ptree& key = v.second;
				shared_ptr<SsKeyframe> keyframe = readKey(key, tag, encoding);
				if (!keyframe) break;
				keyframes.push_back(*keyframe);
			}
		}

		// タイムラインを構築する 
		shared_ptr<SsKeyframeTimeline> timeline = 
			shared_ptr<SsKeyframeTimeline>(new SsKeyframeTimeline(keyframes));

		return shared_ptr<SsAttribute>(
			new SsAttribute(tag, inheritedPercent, timeline));

	} while (false);
	return shared_ptr<SsAttribute>();
}


/**
 * <Key>からキーフレームを構築する
 */
static shared_ptr<SsKeyframe> readKey(const ptree& pt, const SsAttributeTag::Tag& tag, textenc::Encoding encoding)
{
	do
	{
		// 必須パラメータ 
		optional<int> time = pt.get_optional<int>("<xmlattr>.Time");
		if (!time) break;

		boost::shared_ptr<SsValue> ssValue;
		if (tag == SsAttributeTag::VERT)
		{
			// 頂点変形
			optional<int> topLeftX = pt.get_optional<int>("TopLeft.X");
			optional<int> topLeftY = pt.get_optional<int>("TopLeft.Y");
			optional<int> topRightX = pt.get_optional<int>("TopRight.X");
			optional<int> topRightY = pt.get_optional<int>("TopRight.Y");
			optional<int> bottomLeftX = pt.get_optional<int>("BottomLeft.X");
			optional<int> bottomLeftY = pt.get_optional<int>("BottomLeft.Y");
			optional<int> bottomRightX = pt.get_optional<int>("BottomRight.X");
			optional<int> bottomRightY = pt.get_optional<int>("BottomRight.Y");
			if (!topLeftX || !topLeftY) break;
			if (!topRightX || !topRightY) break;
			if (!bottomLeftX || !bottomLeftY) break;
			if (!bottomRightX || !bottomRightY) break;
			
			SsVertex4Value* value = new SsVertex4Value();
			value->v[SS_VERTEX_TOP_LEFT].x = topLeftX.get();
			value->v[SS_VERTEX_TOP_LEFT].y = topLeftY.get();
			value->v[SS_VERTEX_TOP_RIGHT].x = topRightX.get();
			value->v[SS_VERTEX_TOP_RIGHT].y = topRightY.get();
			value->v[SS_VERTEX_BOTTOM_LEFT].x = bottomLeftX.get();
			value->v[SS_VERTEX_BOTTOM_LEFT].y = bottomLeftY.get();
			value->v[SS_VERTEX_BOTTOM_RIGHT].x = bottomRightX.get();
			value->v[SS_VERTEX_BOTTOM_RIGHT].y = bottomRightY.get();
			
			ssValue = boost::shared_ptr<SsValue>(value);
		}
		else if (tag == SsAttributeTag::PCOL)
		{
			// カラーブレンド
			optional<int> type = pt.get_optional<int>("Type");
			optional<int> blend = pt.get_optional<int>("Blend");
			if (!type || !blend) break;
			if (type.get() < 0 || type.get() >= SsColorBlendValue::END_OF_TYPE) break;
			if (blend.get() < 0 || blend.get() >= SsColorBlendValue::END_OF_BLEND) break;

			SsColorBlendValue value;
			value.type = static_cast<SsColorBlendValue::Type>(type.get());
			value.blend = static_cast<SsColorBlendValue::Blend>(SsColorBlendValue::BLEND_MIX + blend.get());
			
			if (value.type == SsColorBlendValue::TYPE_COLORTYPE_PARTS)
			{
				// 単色
				optional<const ptree&> v = pt.get_child_optional("Value");
				if (!v) break;

				bool result = readColors(value.colors[0], v.get());
				if (!result) break;
				// 後の処理で扱いやすいように他の頂点ワークにも同じ値を設定しておく
				readColors(value.colors[1], v.get());
				readColors(value.colors[2], v.get());
				readColors(value.colors[3], v.get());
			}
			else if (value.type == SsColorBlendValue::TYPE_COLORTYPE_VERTEX)
			{
				// 頂点カラー
				optional<const ptree&> tl = pt.get_child_optional("TopLeft");
				optional<const ptree&> tr = pt.get_child_optional("TopRight");
				optional<const ptree&> bl = pt.get_child_optional("BottomLeft");
				optional<const ptree&> br = pt.get_child_optional("BottomRight");
				if (!tl || !tr || !bl || !br) break;

				bool result;
				result = readColors(value.colors[SS_VERTEX_TOP_LEFT], tl.get());
				if (!result) break;
				result = readColors(value.colors[SS_VERTEX_TOP_RIGHT], tr.get());
				if (!result) break;
				result = readColors(value.colors[SS_VERTEX_BOTTOM_LEFT], bl.get());
				if (!result) break;
				result = readColors(value.colors[SS_VERTEX_BOTTOM_RIGHT], br.get());
				if (!result) break;
			}
			else if (value.type == SsColorBlendValue::TYPE_COLORTYPE_NONE)
			{
				// カラー指定無し
				value.colors[0] =
				value.colors[1] =
				value.colors[2] =
				value.colors[3] = SsColor(0, 0, 0, 0);
			}
			else
			{
				throw std::logic_error("Not support color type");
			}
			
			ssValue = boost::shared_ptr<SsValue>(new SsColorBlendValue(value));
		}
		else if (tag == SsAttributeTag::UDAT)
		{
			// ユーザーデータ
			SsUserDataValue value;
			
			// Number
			optional<long> number = pt.get_optional<long>("Number");
			if (number)
			{
				int n = number.get() >= INT_MAX ? INT_MAX : static_cast<int>(number.get());
				value.number = n;
			}

			// Rect
			optional<const ptree&> rectPt = pt.get_child_optional("Rect");
			if (rectPt)
			{
				SsRect rect;
				if (readRect(rect, rectPt.get(), false))	// 座標の正規化は行わない
				{
					value.rect = rect;
				}
			}

			// Point
			optional<const ptree&> pointPt = pt.get_child_optional("Point");
			if (pointPt)
			{
				SsPoint point;
				if (readPoint(point, pointPt.get()))
				{
					value.point = point;
				}
			}

			// String
			optional<std::string> str = pt.get_optional<std::string>("String");
			if (str)
			{
				std::string s = textenc::convert(str.get(), encoding, textenc::SHIFT_JIS);
				value.str = s;
			}
		
			ssValue = boost::shared_ptr<SsValue>(new SsUserDataValue(value));
		}
		else
		{
			optional<float> value = pt.get_optional<float>("Value");
			if (!value) break;
			
			float v = value.get();

			// 回転角は度で定義されているが、内部ではラジアンで持つ
			if (tag == SsAttributeTag::ANGL)
			{
				v = mathutil::degreeToRadian(v);
			}

			ssValue = boost::shared_ptr<SsValue>(new SsFloatValue(v));
		}
		if (!ssValue) break;
		

		// CurveTypeアトリビュート、無ければデフォルトはNone
		SsCurve::Type curveType = SsCurve::None;
		optional<int> curveTypeValue = pt.get_optional<int>("<xmlattr>.CurveType");
		if (curveTypeValue)
		{
            int type = curveTypeValue.get();
            if (type < 0 || type >= SsCurve::END_OF_TYPE) type = SsCurve::Linear;   // 不正値
			curveType = static_cast<SsCurve::Type>(type);
		}

		optional<float> curveStartT = pt.get_optional<float>("<xmlattr>.CurveStartT");
		optional<float> curveStartV = pt.get_optional<float>("<xmlattr>.CurveStartV");
		optional<float> curveEndT = pt.get_optional<float>("<xmlattr>.CurveEndT");
		optional<float> curveEndV = pt.get_optional<float>("<xmlattr>.CurveEndV");
        
		return shared_ptr<SsKeyframe>(
			curveStartT ?
			new SsKeyframe(time.get(), ssValue, curveType, curveStartT.get(), curveStartV.get(), curveEndT.get(), curveEndV.get()):
			new SsKeyframe(time.get(), ssValue, curveType)
		);

	} while (false);
	return shared_ptr<SsKeyframe>();
}


/**
 * カラーパラメータ読み込み
 */
static bool readColors(SsColor& color, const ptree& pt)
{
	do
	{
		optional<int> red   = pt.get_optional<int>("Red");
		optional<int> green = pt.get_optional<int>("Green");
		optional<int> blue  = pt.get_optional<int>("Blue");
		optional<int> alpha = pt.get_optional<int>("Alpha");
		if (!red || !green || !blue || !alpha) break;
		
		color.r = static_cast<SsColor::ColorType>(red.get());
		color.g = static_cast<SsColor::ColorType>(green.get());
		color.b = static_cast<SsColor::ColorType>(blue.get());
		color.a = static_cast<SsColor::ColorType>(alpha.get());
		return true;
	}
	while (false);
	return false;
}


/**
 * Point読み込み
 */
static bool readPoint(SsPoint& point, const ptree& pt)
{
	do
	{
		optional<int> x = pt.get_optional<int>("X");
		optional<int> y = pt.get_optional<int>("Y");
		if (!x || !y) break;

		point.x = x.get();
		point.y = y.get();
		return true;
	}
	while (false);
	return false;
}


/**
 * Rect読み込み
 */
static bool readRect(SsRect& rect, const ptree& pt, bool normalizeEnabled)
{
	do
	{
		optional<int> top    = pt.get_optional<int>("Top");
		optional<int> left   = pt.get_optional<int>("Left");
		optional<int> bottom = pt.get_optional<int>("Bottom");
		optional<int> right  = pt.get_optional<int>("Right");
		if (!top || !left || !bottom || !right) break;
		
		SsRect r(left.get(), top.get(), right.get(), bottom.get(), normalizeEnabled);
		rect = r;
		return true;
	}
	while (false);
	return false;
}



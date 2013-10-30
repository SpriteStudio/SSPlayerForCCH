
#include "SsMotion.h"

#include <cassert>
#include <sstream>
#include <algorithm>
#include <boost/foreach.hpp>

namespace ss
{

/**************************************************
 * 基本型 
 **************************************************/
	
/**
 * XY座標値
 */
SsPoint::SsPoint(void)
	: x(0), y(0)
{}

SsPoint::SsPoint(int x, int y)
	: x(x), y(y)
{}
	
SsPoint::~SsPoint()
{}

bool SsPoint::isZero() const
{
	return x == 0 && y == 0;
}

std::string SsPoint::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << "x=" << x << ",y=" << y;
	ss << "]";
	return ss.str();
}


SsPointF::SsPointF(void)
	: x(0), y(0)
{}

SsPointF::SsPointF(float x, float y)
	: x(x), y(y)
{}
	
SsPointF::~SsPointF()
{}

bool SsPointF::isZero() const
{
	return x == 0 && y == 0;
}

std::string SsPointF::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << "x=" << x << ",y=" << y;
	ss << "]";
	return ss.str();
}


/**
 * サイズ 
 */
SsSize::SsSize(void)
	: _width(0), _height(0)
{}

SsSize::SsSize(int width, int height)
	: _width(width), _height(height)
{}

SsSize::~SsSize()
{}

std::string SsSize::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << "w=" << _width << ",h=" << _height;
	ss << "]";
	return ss.str();
}


/**
 * 矩形座標値
 */
SsRect::SsRect(void)
	: _left(0), _top(0), _right(0), _bottom(0)
{}

SsRect::SsRect(int left, int top, int right, int bottom, bool normalizeEnabled)
	: _left(left), _top(top), _right(right), _bottom(bottom)
{
	if (normalizeEnabled)
	{
		normalize();
	}
}
	
SsRect::~SsRect()
{}

void SsRect::normalize()
{
	if (_right < _left) std::swap(_left, _right);
	if (_bottom < _top) std::swap(_top, _bottom);
}

std::string SsRect::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << "rect=" << _left;
	ss << "," << _top;
	ss << "," << _right;
	ss << "," << _bottom;
	ss << "]";
	return ss.str();
}

SsRect SsRect::intersect(const SsRect& rect1, const SsRect& rect2)
{
	if (rect1.getRight() < rect2.getLeft()
	 || rect2.getRight() < rect1.getLeft()
	 || rect1.getBottom() < rect2.getTop()
	 || rect2.getBottom() < rect1.getTop())
	{
		// 交差していない 
		return SsRect();
	}

	int left   = std::max(rect1.getLeft(),   rect2.getLeft());
	int top    = std::max(rect1.getTop(),    rect2.getTop());
	int right  = std::min(rect1.getRight(),  rect2.getRight());
	int bottom = std::min(rect1.getBottom(), rect2.getBottom());
	return SsRect(left, top, right, bottom);
}

bool operator<(const SsRect& lhs, const SsRect& rhs)
{
    if (lhs._left < rhs._left) return true;
    if (lhs._left > rhs._left) return false;

    if (lhs._top < rhs._top) return true;
    if (lhs._top > rhs._top) return false;

    if (lhs._right < rhs._right) return true;
    if (lhs._right > rhs._right) return false;
    
    if (lhs._bottom < rhs._bottom) return true;
    if (lhs._bottom > rhs._bottom) return false;
    
    return false;
}


/**
 * カラー
 */
SsColor::SsColor()
	: r(0), g(0), b(0), a(0)
{}

SsColor::SsColor(ColorType r, ColorType g, ColorType b, ColorType a)
	: r(r), g(g), b(b), a(a)
{}

SsColor::~SsColor()
{}

SsColor::ColorType SsColor::clip(int value)
{
	if (value <= 0) return 0;
	else if (value >= 255) return 255;
	return static_cast<ColorType>(value);
}

SsColor::ColorType SsColor::clip(float value)
{
	return clip(static_cast<int>(value + 0.5f));
}


/**************************************************
 * キーフレーム関連 
 **************************************************/

/**
 * キーフレームの補間方法およびパラメータ
 */
SsCurve::SsCurve(Type type)
	: _type(type)
	, _startT(0)
	, _startV(0)
	, _endT(0)
	, _endV(0)
{
}

SsCurve::SsCurve(Type type, float startT, float startV, float endT, float endV)
	: _type(type)
	, _startT(startT)
	, _startV(startV)
	, _endT(endT)
	, _endV(endV)
{
}

SsCurve::SsCurve(void)
	: _type(None)
	, _startT(0)
	, _startV(0)
	, _endT(0)
	, _endV(0)
{
}

SsCurve::~SsCurve()
{
}

std::string SsCurve::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << _type;
	if (_startT != 0 || _startV != 0 || _endT != 0 || _endV != 0)
	{
		ss << ":" << _startT;
		ss << "," << _startV;
		ss << "," << _endT;
		ss << "," << _endV;
	}
	ss << "]";
	return ss.str();
}


/**
 * キーフレーム 
 */
SsKeyframe::SsKeyframe(int frameNo, SsKeyframe::ValuePtr value, SsCurve::Type curveType)
	: _frameNo(frameNo)
	, _value(value)
	, _curve(curveType)
{
}

SsKeyframe::SsKeyframe(int frameNo, SsKeyframe::ValuePtr value, SsCurve::Type curveType, float curveStartT, float curveStartV, float curveEndT, float curveEndV)
	: _frameNo(frameNo)
	, _value(value)
	, _curve(curveType, curveStartT, curveStartV, curveEndT, curveEndV)
{
}

SsKeyframe::SsKeyframe(void)
	: _frameNo(0)
	, _value(ValuePtr())
	, _curve()
{
}

SsKeyframe::~SsKeyframe()
{
}

std::string SsKeyframe::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << _frameNo;
	ss << ":" << _value->toString();
	ss << "," << _curve.toString();
	ss << "]";
	return ss.str();
}


/**
 * キーフレームタイムライン 
 */
SsKeyframeTimeline::SsKeyframeTimeline(const std::vector<SsKeyframe>& keyframes)
{
	BOOST_FOREACH( const SsKeyframe& keyframe, keyframes )
	{
		_timeline[keyframe.getFrameNo()] = keyframe;
	}
}

SsKeyframeTimeline::SsKeyframeTimeline(void)
{
}

SsKeyframeTimeline::~SsKeyframeTimeline()
{
}

std::string SsKeyframeTimeline::toString() const
{
	std::stringstream ss;
	ss << "[";
	bool first = true;
	BOOST_FOREACH( const Timeline::value_type& i, _timeline )
	{
		if (!first) ss << ",";
		ss << i.second.toString();
		first = false;
	}
	ss << "]";
	return ss.str();
}

bool SsKeyframeTimeline::hasTimeline() const
{
	return !_timeline.empty();
}

int SsKeyframeTimeline::getFirstFrameNo() const
{
	return !_timeline.empty() ? _timeline.begin()->first : -1;
}

int SsKeyframeTimeline::getLastFrameNo() const
{
	return !_timeline.empty() ? _timeline.rbegin()->first : -1;
}

/** 指定フレーム以前のキーフレームを探す。見つからないときはInvalidを返す */
SsKeyframe::ConstPtr SsKeyframeTimeline::findForward(int frameNo) const
{
	std::pair<int, SsKeyframe> i;
	BOOST_REVERSE_FOREACH( i, _timeline )
	{
		if (i.first <= frameNo)
		{
			return &_timeline.at(i.first);
		}
	}
	return Invalid;
}

/** 指定フレーム以後のキーフレームを探す。見つからないときはInvalidを返す */
SsKeyframe::ConstPtr SsKeyframeTimeline::findBackward(int frameNo) const
{
	std::pair<int, SsKeyframe> i;
	BOOST_FOREACH( i, _timeline )
	{
		if (i.first >= frameNo)
		{
			return &_timeline.at(i.first);
		}
	}
	return Invalid;
}

/** 指定フレームのキーフレームを探す。見つからないときはInvalidを返す */
SsKeyframe::ConstPtr SsKeyframeTimeline::find(int frameNo) const
{
	std::pair<int, SsKeyframe> i;
	BOOST_FOREACH( i, _timeline )
	{
		if (i.first == frameNo)
		{
			return &_timeline.at(i.first);
		}
	}
	return Invalid;
}

/** 無効なキーフレーム */
SsKeyframe::ConstPtr const SsKeyframeTimeline::Invalid = new SsKeyframe(-1, SsKeyframe::ValuePtr(), SsCurve::None);


/**************************************************
 *
 **************************************************/

SsValue::SsValue()
{}

SsValue::~SsValue()
{}

std::string SsValue::toString() const
{
	return "";
}


SsFloatValue::SsFloatValue()
	: value(0)
{}

SsFloatValue::SsFloatValue(float value)
	: value(value)
{}

SsFloatValue::~SsFloatValue()
{}

std::string SsFloatValue::toString() const
{
	return "";
}


/**
 * 4頂点座標(2D)
 */
SsVertex4Value::SsVertex4Value()
{}

SsVertex4Value::SsVertex4Value(const SsVertex4Value& ref)
{
	for (size_t i = 0; i < 4; i++) v[i] = ref.v[i];
}

SsVertex4Value::~SsVertex4Value()
{}


/**
 * カラーブレンド
 */
SsColorBlendValue::SsColorBlendValue()
	: type(TYPE_COLORTYPE_NONE)
	, blend(BLEND_VOID)
{}

SsColorBlendValue::SsColorBlendValue(const SsColorBlendValue& ref)
	: type(ref.type)
	, blend(ref.blend)
{
	for (size_t i = 0; i < 4; i++) colors[i] = ref.colors[i];
}

SsColorBlendValue::~SsColorBlendValue()
{}


/**
 * ユーザーデータ
 */
SsUserDataValue::SsUserDataValue()
{}

SsUserDataValue::SsUserDataValue(const SsUserDataValue& ref)
	: number(ref.number)
	, rect(ref.rect)
	, point(ref.point)
	, str(ref.str)
{}

SsUserDataValue::~SsUserDataValue()
{}

bool SsUserDataValue::hasData() const
{
	return number || rect || point || str;
}


/**************************************************
 * アニメーション アトリビュート関連 
 **************************************************/

/**
 * SsAttributeType
 */

SsAttributeType::SsAttributeType(Category category, float defaultValue, bool inheritable, float defaultInheritPercent)
	: _category(category)
	, _defaultValue(new SsFloatValue(defaultValue))
	, _inheritable(inheritable)
    , _defaultInheritPercent(inheritable ? defaultInheritPercent : 0)
{}

SsAttributeType::SsAttributeType(Category category, SsValue* defaultValue, bool inheritable, float defaultInheritPercent)
	: _category(category)
	, _defaultValue(defaultValue)
	, _inheritable(inheritable)
    , _defaultInheritPercent(inheritable ? defaultInheritPercent : 0)
{}

SsAttributeType::SsAttributeType(void)
	: _category(Number)
	, _defaultValue(0)
	, _inheritable(false)
    , _defaultInheritPercent(0)
{}

SsAttributeType::~SsAttributeType()
{}


/**
 * アトリビュートタグ
 */
// 参考：http://www.smork.info/blog/index.php?entry=entry110809-105615

// enum値から文字列を取得
const std::string SsAttributeTag::toString(const Tag tag)
{
	#define SS_ATTRIBUTE_TAG_TO_STR(unused, data, elem) if (tag == elem) return BOOST_PP_STRINGIZE(elem);
	BOOST_PP_SEQ_FOR_EACH(SS_ATTRIBUTE_TAG_TO_STR, ~, SS_ATTRIBUTE_TAG_SEQ)
	return "Unknown";
}

// 文字列からenum値を取得
SsAttributeTag::Tag SsAttributeTag::toEnum(const std::string& s)
{
	#define SS_ATTRIBUTE_TAG_TO_INT(unused, data, elem) if (s == BOOST_PP_STRINGIZE(elem)) return elem;
	BOOST_PP_SEQ_FOR_EACH(SS_ATTRIBUTE_TAG_TO_INT, ~, SS_ATTRIBUTE_TAG_SEQ)
	return SsAttributeTag::Unknown;
}

const SsAttributeType& SsAttributeTag::getType(const Tag tag)
{
	static const SsAttributeType types[] =
	{
		SsAttributeType(),
	//	(POSX)(POSY)(ANGL)(SCAX)(SCAY)(TRAN)
		SsAttributeType(SsAttributeType::Coordinates, 0.0f, true, 100),			/** X座標 */
		SsAttributeType(SsAttributeType::Coordinates, 0.0f, true, 100),			/** Y座標 */
		SsAttributeType(SsAttributeType::Radian, 0.0f, true, 100),				/** 角度（ラジアン） */
		SsAttributeType(SsAttributeType::Scale, 1.0f, true, 100),				/** 横拡大率 */
		SsAttributeType(SsAttributeType::Scale, 1.0f, true, 100),				/** 縦拡大率 */
		SsAttributeType(SsAttributeType::Ratio, 1.0f, true, 100),				/** 不透明度 */
	//	(PRIO)(FLPH)(FLPV)(HIDE)
		SsAttributeType(SsAttributeType::Number, 0.0f),							/** 優先度 */
		SsAttributeType(SsAttributeType::Flag, 0.0f, true, 0),					/** 左右反転フラグ */
		SsAttributeType(SsAttributeType::Flag, 0.0f, true, 0),					/** 上下反転フラグ */
		SsAttributeType(SsAttributeType::Flag, 0.0f, true, 0),					/** 非表示フラグ */
	//	(IMGX)(IMGY)(IMGW)(IMGH)(ORFX)(ORFY)
		SsAttributeType(SsAttributeType::Coordinates, 0.0f),					/** 参照元 Xオフセット */
		SsAttributeType(SsAttributeType::Coordinates, 0.0f),					/** 参照元 Yオフセット */
		SsAttributeType(SsAttributeType::Coordinates, 0.0f),					/** 参照元 Wオフセット */
		SsAttributeType(SsAttributeType::Coordinates, 0.0f),					/** 参照元 Hオフセット */
		SsAttributeType(SsAttributeType::Coordinates, 0.0f),					/** 原点 Xオフセット */
		SsAttributeType(SsAttributeType::Coordinates, 0.0f),					/** 原点 Yオフセット */
	//	(PCOL)(PALT)(VERT)(UDAT)
		SsAttributeType(SsAttributeType::Composite, new SsColorBlendValue()),	/** カラーブレンド */
		SsAttributeType(SsAttributeType::Number, 0.0f),							/** パレットチェンジ */
		SsAttributeType(SsAttributeType::Composite, new SsVertex4Value()),		/** 頂点変形 */
		SsAttributeType(SsAttributeType::Composite, new SsUserDataValue())		/** ユーザーデータ */
	};
	static const int numTypes = sizeof(types) / sizeof(SsAttributeType);

	int tagNo = tag;
	if (tagNo < 0 || tagNo >= numTypes) tagNo = 0;
	return types[tagNo];
}


/**
 * SsAttribute
 */
SsAttribute::SsAttribute(
		SsAttributeTag::Tag tag,
		const boost::optional<float>& inheritedPercent,
		boost::shared_ptr<SsKeyframeTimeline> timeline)
	: _tag(tag)
	, _inheritedPercent(inheritedPercent)
	, _timeline(timeline)
{
}

SsAttribute::~SsAttribute()
{
}

std::string SsAttribute::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << "tag=" << SsAttributeTag::toString(_tag);
	ss << ",";
	if (_inheritedPercent) ss << "inherited%=" << _inheritedPercent.get(); else ss << "noinherited%";
	ss << ",timeline=" << _timeline->toString();
	ss << "]";
	return ss.str();
}


/**
 * SsAttributes
 */
SsAttributes::SsAttributes(const std::vector<SsAttribute::Ptr>& attributes, int motionEndFrameNo)
{
	for (size_t i = 0; i < SsAttributeTag::NumTags; i++)
	{
		_attributes[i] = SsAttribute::Ptr();
	}

	int firstFrameNo = INT_MAX;
	int lastFrameNo = -1;
	BOOST_FOREACH( const SsAttribute::Ptr& ptr, attributes )
	{
		int index = ptr->getTag();
		_attributes[index] = ptr;
		if (ptr->getTimeline()->hasTimeline())
		{
			firstFrameNo = std::min(firstFrameNo, ptr->getTimeline()->getFirstFrameNo());
			lastFrameNo = std::max(lastFrameNo, ptr->getTimeline()->getLastFrameNo());
		}
	}
	
	_firstFrameNo = firstFrameNo;

	_lastFrameNo = lastFrameNo;
	_motionEndFrameNo = motionEndFrameNo;
}

SsAttributes::SsAttributes()
	: _firstFrameNo(-1)
	, _lastFrameNo(-1)
{
}

SsAttributes::~SsAttributes()
{
}

bool SsAttributes::hasFrame(int frameNo) const
{
//	return frameNo >= _firstFrameNo && frameNo <= _lastFrameNo;
	// モーション全体の最終フレーム番号をこのアトリビュート群の最終フレーム番号とする 
	return frameNo >= _firstFrameNo && frameNo <= _motionEndFrameNo;
}

SsAttribute::ConstPtr SsAttributes::getAttribute(SsAttributeTag::Tag tag) const
{
	int tagNo = tag;
	assert(tagNo >= 0 && tagNo < SsAttributeTag::NumTags);
	return _attributes[tagNo];
}



/**************************************************
 * パーツ関連 
 **************************************************/

/**
 * SsPart
 */

SsPart::SsPart()
	: type(TypeNormal)
	, name()
	, id(-1)
	, parentId(-1)
	, picId(0)
	, picArea()
	, alphaBlend(AlphaBlendMix)
	, origin()
	, inheritEach(false)
{}

SsPart::~SsPart()
{}

std::string SsPart::toString() const
{
	std::stringstream ss;
	ss << "[";
	ss << type;
	ss << ":" << name;
	ss << "," << id;
	ss << "," << parentId;
	ss << ",pic=" << picId << "," << picArea.toString();
	ss << ",alphaBlend=" << alphaBlend;
	ss << ",origin=" << origin.toString();
	ss << ",inheritEach=" << inheritEach;
	ss << "]";
	return ss.str();
}



/**
 * SsNode
 */

SsNode::SsNode(SsPart::Ptr part)
	: _part(part)
{}

SsNode::~SsNode()
{}


SsNode::Ptr SsNode::createNodes(std::vector<SsPart::Ptr>& parts)
{
	// ルートパーツを探す 
	SsPart::Ptr rootPart;
	BOOST_FOREACH( SsPart::Ptr& part, parts )
	{
		if (part->type == SsPart::TypeRoot)
		{
			rootPart = part;
			rootPart->inheritEach = 1;	// 親の継承を参照しない 
			break;
		}
	}

	// ルートパーツが見つからない 
	if (!rootPart) return Ptr();

	// ルートノード構築 
	Ptr rootNode = Ptr(new SsNode(rootPart));

	createChildren(rootNode, parts);

	return rootNode;
}

void SsNode::createChildren(Ptr node, std::vector<SsPart::Ptr>& parts)
{
	int nodeId = node->_part->id;
	BOOST_FOREACH( SsPart::Ptr& part, parts )
	{
		if (part->id < 0) continue;
		if (part->parentId == nodeId)
		{
			Ptr childNode = Ptr(new SsNode(part));

			childNode->_parent = node;
			node->_children.push_back(childNode);

			createChildren(childNode, parts);
		}
	}
}

SsNode::ConstPtr SsNode::getParent() const
{
	return _parent;
}

const SsNode::ChildrenList& SsNode::getChildren() const
{
	return _children;
}

static void countTreeNodesSub(int& count, const SsNode* node)
{
	count++;
	BOOST_FOREACH( SsNode::ConstPtr child, node->getChildren() )
	{
		countTreeNodesSub(count, child.get());
	}
}

int SsNode::countTreeNodes() const
{
	int count = 0;
	countTreeNodesSub(count, this);
	return count;
}

bool SsNode::hasFrame(int frameNo) const
{
	return _part->attributes.hasFrame(frameNo);
}

SsAttribute::ConstPtr SsNode::getAttribute(SsAttributeTag::Tag tag) const
{
	return _part->attributes.getAttribute(tag);
}



/**************************************************
 * 画像情報 
 **************************************************/

/**
 * SsImage
 */

SsImage::SsImage(int id, const std::string& path, const SsSize& size, int bpp)
	: _id(id)
	, _path(path)
	, _size(size)
	, _bpp(bpp)
{}

SsImage::~SsImage()
{}


/**
 * SsImageList
 */

SsImageList::SsImageList(std::vector<SsImage::Ptr>& images)
{
	BOOST_FOREACH( SsImage::Ptr image, images )
	{
		_images.push_back(image);
	}
}

SsImageList::~SsImageList()
{}



/**************************************************
 * 集約データ 
 **************************************************/

/**
 * SsMotion
 */

SsMotion::SsMotion(SsNode::Ptr rootNode, const Param& param, SsImageList::Ptr imageList)
	: _rootNode(rootNode)
	, _param(param)
	, _imageList(imageList)
{}

SsMotion::~SsMotion()
{}



}	// namespace ss


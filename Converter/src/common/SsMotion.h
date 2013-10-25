#ifndef _SS_MOTION_H_
#define _SS_MOTION_H_

#include <string>
#include <vector>
#include <map>
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/preprocessor.hpp>

namespace ss
{

/**************************************************
 * 基本型 
 **************************************************/

/**
 * XY座標値
 */
struct SsPoint
{
public:
	int	x;
	int	y;

	SsPoint(void);
	SsPoint(int x, int y);
	~SsPoint();
	
	bool isZero() const;

	std::string toString() const;
};


/**
 * XY座標値
 */
struct SsPointF
{
public:
	float	x;
	float	y;

	SsPointF(void);
	SsPointF(float x, float y);
	~SsPointF();
	
	bool isZero() const;

	std::string toString() const;
};


/**
 * サイズ 
 */
struct SsSize
{
public:
	SsSize(void);
	SsSize(int width, int height);
	~SsSize();

	int getWidth() const	{ return _width; }
	int getHeight() const	{ return _height; }

	std::string toString() const;

private:
	int	_width;
	int	_height;
};


/**
 * 矩形座標値
 */
struct SsRect
{
public:
	SsRect(void);
	SsRect(int left, int top, int right, int bottom, bool normalizeEnabled = true);
	~SsRect();

	int getLeft() const		{ return _left; }
	int getTop() const		{ return _top; }
	int getRight() const	{ return _right; }
	int getBottom() const	{ return _bottom; }

	int getWidth() const	{ return _right - _left; }
	int getHeight() const	{ return _bottom - _top; }

	std::string toString() const;

	static SsRect intersect(const SsRect& rect1, const SsRect& rect2);
    
    friend bool operator<(const SsRect& lhs, const SsRect& rhs);

private:
	void normalize();

	int	_left;
	int	_top;
	int	_right;
	int	_bottom;
};

bool operator<(const SsRect& lhs, const SsRect& rhs);


/**
 * カラー
 */
struct SsColor
{
public:
	typedef unsigned char	ColorType;
	
	ColorType	r;
	ColorType	g;
	ColorType	b;
	ColorType	a;

	SsColor();
	SsColor(ColorType r, ColorType g, ColorType b, ColorType a);
	~SsColor();

	static ColorType clip(int value);
	static ColorType clip(float value);
};


/**************************************************
 * キーフレーム関連 
 **************************************************/

class SsValue;

/**
 * キーフレームの補間方法およびパラメータ
 */
class SsCurve
{
public:
	/**
	 * キーフレームの補間方法 
	 */
	enum Type
	{
		None,		/**< 補間無し */
		Linear,		/**< 直線補間 */
		Hermite,	/**< エルミート曲線 */
		Bezier,		/**< ベジェ曲線 */
        END_OF_TYPE
	};

	SsCurve(Type type);
	SsCurve(Type type, float startT, float startV, float endT, float endV);
	SsCurve(void);
	~SsCurve();

	Type getType() const	{ return _type; }
	float getStartT() const	{ return _startT; }
	float getStartV() const	{ return _startV; }
	float getEndT() const	{ return _endT; }
	float getEndV() const	{ return _endV; }

	std::string toString() const;

private:
	Type	_type;
	float	_startT;
	float	_startV;
	float	_endT;
	float	_endV;
};


/**
 * キーフレーム 
 */
class SsKeyframe
{
public:
	typedef SsKeyframe* Ptr;
	typedef const SsKeyframe* ConstPtr;
    typedef boost::shared_ptr<SsValue> ValuePtr;
    typedef boost::shared_ptr<const SsValue> ConstValuePtr;

	SsKeyframe(int frameNo, ValuePtr value, SsCurve::Type curveType);
	SsKeyframe(int frameNo, ValuePtr value, SsCurve::Type curveType, float curveStartT, float curveStartV, float curveEndT, float curveEndV);
	SsKeyframe(void);
	~SsKeyframe();

	int getFrameNo() const				{ return _frameNo; }
    const SsValue* getValue() const		{ return _value.get(); }
	const SsCurve& getCurve() const		{ return _curve; }
    
	std::string toString() const;

private:
	int			_frameNo;
	ValuePtr	_value;
	SsCurve		_curve;
};


/**
 * キーフレームタイムライン 
 */
class SsKeyframeTimeline
{
private:
	typedef std::map<int, SsKeyframe> Timeline;

public:
	typedef boost::shared_ptr<SsKeyframeTimeline> Ptr;
	typedef boost::shared_ptr<const SsKeyframeTimeline> ConstPtr;

	SsKeyframeTimeline(const std::vector<SsKeyframe>& keyframes);
	SsKeyframeTimeline(void);
	~SsKeyframeTimeline();

	bool hasTimeline() const;
	int getFirstFrameNo() const;
	int getLastFrameNo() const;

	/** 指定フレーム以前のキーフレームを探す。見つからないときはInvalidを返す */
	SsKeyframe::ConstPtr findForward(int frameNo) const;
	/** 指定フレーム以後のキーフレームを探す。見つからないときはInvalidを返す */
	SsKeyframe::ConstPtr findBackward(int frameNo) const;
	/** 指定フレームのキーフレームを探す。見つからないときはInvalidを返す */
	SsKeyframe::ConstPtr find(int frameNo) const;

	/** 無効なキーフレーム */
	static SsKeyframe::ConstPtr const Invalid;

	std::string toString() const;

private:
	Timeline	_timeline;
};



/**************************************************
 * キーフレームの値
 **************************************************/
    
class SsValue
{
public:
    virtual ~SsValue() = 0;
    
	virtual std::string toString() const;
    
protected:
    SsValue();
};


class SsFloatValue : public SsValue
{
public:
    SsFloatValue();
    explicit SsFloatValue(float value);
    virtual ~SsFloatValue();
    
	virtual std::string toString() const;

    float value;
};


enum SsVertex4Index
{
	SS_VERTEX_TOP_LEFT,
	SS_VERTEX_TOP_RIGHT,
	SS_VERTEX_BOTTOM_LEFT,
	SS_VERTEX_BOTTOM_RIGHT
};


/**
 * 4頂点座標(2D)
 */
class SsVertex4Value : public SsValue
{
public:
	SsVertex4Value();
	SsVertex4Value(const SsVertex4Value& ref);
	virtual ~SsVertex4Value();
	
	SsPoint	v[4];
};


/**
 * カラーブレンド
 */
class SsColorBlendValue : public SsValue
{
public:
	enum Type
	{
		TYPE_COLORTYPE_NONE,	/**< カラー指定無し */
		TYPE_COLORTYPE_PARTS,	/**< 単色 */
		TYPE_COLORTYPE_VERTEX,	/**< 頂点カラー */
		END_OF_TYPE
	};
	
	enum Blend
	{
		BLEND_VOID,				/**< 無効 */
		BLEND_MIX,				/**< ブレンド */
		BLEND_MULTIPLE,			/**< 乗算 */
		BLEND_ADD,				/**< 加算 */
		BLEND_SUBTRACT,			/**< 減算 */
		END_OF_BLEND
	};

	SsColorBlendValue();
	SsColorBlendValue(const SsColorBlendValue& ref);
	virtual ~SsColorBlendValue();

	Type	type;
	Blend	blend;
	SsColor	colors[4];
};


/**
 * ユーザーデータ
 */
class SsUserDataValue : public SsValue
{
public:
	SsUserDataValue();
	SsUserDataValue(const SsUserDataValue& ref);
	virtual ~SsUserDataValue();
	
	bool hasData() const;

	boost::optional<int>			number;
	boost::optional<SsRect>			rect;
	boost::optional<SsPoint>		point;
	boost::optional<std::string>	str;
};


/**************************************************
 * アニメーション アトリビュート関連 
 **************************************************/

/**
 * SsAttributeType
 */
class SsAttributeType
{
public:
	enum Category
	{
		Number,			/** 数値 */
		Coordinates,	/** 座標 */
		Scale,			/** スケール */
		Ratio,			/** 0〜1.0 */
		Flag,			/** 0, 1フラグ */
		Radian,			/** 角度（ラジアン） */
		Composite		/** 混成データ */
	};

	SsAttributeType(Category category, float defaultValue, bool inheritable = false, float defaultInheritPercent = 0);
	SsAttributeType(Category category, SsValue* defaultValue, bool inheritable = false, float defaultInheritPercent = 0);
	SsAttributeType(void);
	~SsAttributeType();

	Category getCategory() const            { return _category; }
	const SsValue* getDefaultValue() const  { return _defaultValue; }
	bool isInheritable() const              { return _inheritable; }
    float getDefaultInheritPercent() const  { return _defaultInheritPercent; }

private:
	Category	_category;
	SsValue*	_defaultValue;
	bool		_inheritable;
    float       _defaultInheritPercent;
};


/**
 * アトリビュートタグ
 */
class SsAttributeTag
{
public:
	#undef SS_ATTRIBUTE_TAG_SEQ

	// タグの種類
	//（※追加、並べ替え、削除した際はgetType()の実装も変更すること） 
	#define SS_ATTRIBUTE_TAG_SEQ \
		(POSX)(POSY)(ANGL)(SCAX)(SCAY)(TRAN)\
		(PRIO)(FLPH)(FLPV)(HIDE)\
		(IMGX)(IMGY)(IMGW)(IMGH)(ORFX)(ORFY)\
		(PCOL)(PALT)(VERT)(UDAT)\

	enum Tag
	{
		Unknown,
		BOOST_PP_SEQ_ENUM(SS_ATTRIBUTE_TAG_SEQ)
		,
		NumTags
	};

	/** enum値から文字列を取得 */
	static const std::string toString(const Tag tag);

	/** 文字列からenum値を取得 */
	static Tag toEnum(const std::string& s);


	static const SsAttributeType& getType(const Tag tag);

private:
	// インスタンス構築不可
	SsAttributeTag(void);
};


/**
 * SsAttribute
 */
class SsAttribute
{
public:
	typedef boost::shared_ptr<SsAttribute> Ptr;
	typedef boost::shared_ptr<const SsAttribute> ConstPtr;

	SsAttribute(
		SsAttributeTag::Tag tag,
		const boost::optional<float>& inheritedPercent,
		boost::shared_ptr<SsKeyframeTimeline> timeline
		);
	~SsAttribute();

	SsAttributeTag::Tag getTag() const							{ return _tag; }
	const boost::optional<float>& getInheritedPercent() const	{ return _inheritedPercent; }
	SsKeyframeTimeline::ConstPtr getTimeline() const			{ return _timeline; }

	std::string toString() const;

private:
	SsAttributeTag::Tag		_tag;
	boost::optional<float>	_inheritedPercent;
	SsKeyframeTimeline::Ptr	_timeline;
};


/**
 * SsAttributes
 */
class SsAttributes
{
public:
	SsAttributes(const std::vector<SsAttribute::Ptr>& attributes, int motionEndFrameNo);
	SsAttributes(void);
	~SsAttributes();

	bool hasFrame(int frameNo) const;
	SsAttribute::ConstPtr getAttribute(SsAttributeTag::Tag tag) const;
	int getLastFrameNo() const { return _lastFrameNo; }

private:
	SsAttribute::Ptr	_attributes[SsAttributeTag::NumTags];
	int					_firstFrameNo;
	int					_lastFrameNo;
	int					_motionEndFrameNo;
};



/**************************************************
 * パーツ関連 
 **************************************************/

/**
 * SsPart
 */
struct SsPart
{
public:
	typedef boost::shared_ptr<SsPart> Ptr;
	typedef boost::shared_ptr<const SsPart> ConstPtr;

	enum Type
	{
		PartNormal,		/**< 通常パーツ */
		PartRoot,		/**< ルートパーツ */
		PartNull,		/**< NULLパーツ */
		PartHitTest,	/**< 当たり判定パーツ */
		PartSound		/**< サウンドパーツ */
	};

	Type			type;
	std::string		name;
	int				id;
	int				parentId;
	int				picId;
	SsRect			picArea;
	SsPoint			origin;
	bool			inheritEach;
	SsAttributes	attributes;

	SsPart();
	~SsPart();

	std::string toString() const;
};


/**
 * SsNode
 */
class SsNode
{
public:
	typedef boost::shared_ptr<SsNode> Ptr;
	typedef boost::shared_ptr<const SsNode> ConstPtr;
	typedef std::vector<ConstPtr> ChildrenList;

	static Ptr createNodes(std::vector<SsPart::Ptr>& parts);
	~SsNode();

	ConstPtr getParent() const;
	const ChildrenList& getChildren() const;
	int countTreeNodes() const;

	bool hasFrame(int frameNo) const;
	SsAttribute::ConstPtr getAttribute(SsAttributeTag::Tag tag) const;

	bool isRoot() const						{ return _part->type == SsPart::PartRoot; }
	SsPart::Type getType() const			{ return _part->type; }
	const std::string& getName() const		{ return _part->name; }
	int getId() const						{ return _part->id; }
	int getParentId() const					{ return _part->parentId; }
	int getPicId() const					{ return _part->picId; }
	const SsRect& getPicArea() const		{ return _part->picArea; }
	const SsPoint& getOrigin() const		{ return _part->origin; }
	bool isInheritEach() const				{ return _part->inheritEach; }
	int getLastFrameNo() const				{ return _part->attributes.getLastFrameNo(); }

private:
	SsNode(SsPart::Ptr part);

	static void createChildren(Ptr node, std::vector<SsPart::Ptr>& parts);

	SsPart::Ptr		_part;
	Ptr				_parent;
	ChildrenList	_children;
};



/**************************************************
 * 画像情報 
 **************************************************/

/**
 * SsImage
 */
class SsImage
{
public:
	typedef boost::shared_ptr<SsImage> Ptr;
	typedef boost::shared_ptr<const SsImage> ConstPtr;

	SsImage(int id, const std::string& path, const SsSize& size, int bpp);
	~SsImage();

	int getId() const						{ return _id; }
	const std::string& getPath() const		{ return _path; }

	bool hasSize() const					{ return _size.getWidth() != 0 && _size.getHeight() != 0; }
	const SsSize& getSize() const			{ return _size; }

private:
	int			_id;
	std::string	_path;
	SsSize		_size;
	int			_bpp;
};


/**
 * SsImageList
 */
class SsImageList
{
public:
	typedef boost::shared_ptr<SsImageList> Ptr;
	typedef boost::shared_ptr<const SsImageList> ConstPtr;
	typedef std::vector<SsImage::ConstPtr> ImageList;

	SsImageList(std::vector<SsImage::Ptr>& images);
	~SsImageList();

	const ImageList& getImages() const		{ return _images; }

private:
	ImageList	_images;
};



/**************************************************
 * 集約データ 
 **************************************************/

/**
 * SsMotion
 */
class SsMotion
{
public:
	typedef boost::shared_ptr<SsMotion> Ptr;
	typedef boost::shared_ptr<const SsMotion> ConstPtr;

	struct Param
	{
		int		endFrameNo;
		int		baseTickTime;
	};

	SsMotion(SsNode::Ptr rootNode, const Param& param, SsImageList::Ptr imageList);
	~SsMotion();

	SsNode::ConstPtr getRootNode() const		{ return _rootNode; }
    int getBaseTickTime() const                 { return _param.baseTickTime; }

	bool hasImageList() const					{ return _imageList && !_imageList->getImages().empty(); }
	SsImageList::ConstPtr getImageList() const	{ return _imageList; }

	int getTotalFrame() const					{ return _param.endFrameNo + 1; }

private:
	SsNode::Ptr			_rootNode;
	Param				_param;
	SsImageList::Ptr	_imageList;
};



}	// namespace ss

#endif	// ifndef _SS_MOTION_H_

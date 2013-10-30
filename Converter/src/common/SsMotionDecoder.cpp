
#include "SsMotionDecoder.h"
#include "SsMotionUtil.h"
#include <boost/foreach.hpp>
#include <cassert>
#include <cmath>
#include <memory>

namespace ss
{

static const double PI = 3.14159265358979323846;


/**************************************************
 * 基本的な計算を行う関数郡 
 **************************************************/


struct InterpolationParams
{
	float	ratio;
	SsCurve	curve;
	int		forwardFrameNo;
	int		backwardFrameNo;
};


static float calcTimeRatio(int forwardFrameNo, int backwardFrameNo, int frameNo)
{
	assert(forwardFrameNo < backwardFrameNo);
	assert(forwardFrameNo <= frameNo);

	float r = static_cast<float>(frameNo - forwardFrameNo) / static_cast<float>(backwardFrameNo - forwardFrameNo);
	if (r < 0.0f) r = 0.0f;
	if (r > 1.0f) r = 1.0f;
	return r;
}


/** 補間なし*/
static float calcCurveNone(const InterpolationParams& /*params*/, float forwardValue, float /*backwardValue*/)
{
	return forwardValue;
}


/** 直線補間 */
static float calcCurveLinear(const InterpolationParams& params, float forwardValue, float backwardValue)
{
	float v = (backwardValue - forwardValue) * params.ratio + forwardValue;
	return v;
}


#if 0
// ■ エルミート曲線計算 */
/* POSPARAM *ppos;  補間パラメータ */
/* SSDOUBLE fTime;  正規化されたフレーム時間 */

static SSDOUBLE ssaCalcHermite(POSPARAM *ppos, SSDOUBLE fTime)
{
	SSDOUBLE fTemp1,fTemp2,fTemp3;
	SSDOUBLE y;

	if (ppos == NULL)
	{
		return 0.0f;  /* エラー */
	}

	/* 計算範囲クリッピング */
	if (fTime < 0.0f)
	{
		fTime = 0.0f;
	}
	if (fTime > 1.0f)
	{
		fTime = 1.0f;
	}

	fTemp1 = fTime;
	fTemp2 = fTemp1 * fTemp1;
	fTemp3 = fTemp2 * fTemp1;

	/* 座標計算 */
	y = (2 * fTemp3 - 3 * fTemp2 + 1) * ppos->fStartV +
		(-2 * fTemp3 + 3 * fTemp2) * ppos->fEndV +
		(fTemp3 - 2 * fTemp2 + fTemp1) * (ppos->fSParamV - ppos->fStartV) +
		(fTemp3 - fTemp2) * (ppos->fEParamV - ppos->fEndV);
	return y;
}
#endif

/** エルミート曲線補間（sswviewで使われているものをひな形にしています） */
static float calcCurveHermite(const InterpolationParams& params, float forwardValue, float backwardValue)
{
	const float fTemp1 = params.ratio;
	const float fTemp2 = fTemp1 * fTemp1;
	const float fTemp3 = fTemp2 * fTemp1;

	/* 座標計算 */
	float y = (2 * fTemp3 - 3 * fTemp2 + 1) * forwardValue +
		(-2 * fTemp3 + 3 * fTemp2) * backwardValue +
		(fTemp3 - 2 * fTemp2 + fTemp1) * (params.curve.getStartV() - forwardValue) +
		(fTemp3 - fTemp2) * (params.curve.getEndV() - backwardValue);
	return y;
}


#if 0
// ■ ベジェ曲線計算 */
/* POSPARAM *ppos;  補間パラメータ */
/* SSDOUBLE fTime;  正規化されたフレーム時間 */

static SSDOUBLE ssaCalcBezier(POSPARAM *ppos, SSDOUBLE fTime)
{
	SSDOUBLE fCurrent,fCurrentPos,fCalcRange;
	SSDOUBLE fTemp1,fTemp2,fTemp3;
	SSDOUBLE x,y;
	int iLoop;

	if (ppos == NULL)
	{
		return 0.0f;  /* エラー */
	}

	/* 計算範囲クリッピング */
	if (fTime < 0.0f)
	{
		fTime = 0.0f;
	}
	if (fTime > 1.0f)
	{
		fTime = 1.0f;
	}

	fCurrentPos = (ppos->fEndT - ppos->fStartT) * fTime + ppos->fStartT;
	fCurrent = 0.5f;
	fCalcRange = 0.5f;

	for (iLoop = 0; iLoop < 8; iLoop++)  /* ループ回数を多くすれば精度が上がる */
	{
		/* X座標計算 */
		fTemp1 = 1.0f - fCurrent;
		fTemp2 = fTemp1 * fTemp1;
		fTemp3 = fTemp2 * fTemp1;
		x = (fTemp3 * ppos->fStartT) +
			(3 * fTemp2 * fCurrent * (ppos->fSParamT + ppos->fStartT)) +
			(3 * fTemp1 * fCurrent * fCurrent * (ppos->fEParamT + ppos->fEndT)) +
			(fCurrent * fCurrent * fCurrent * ppos->fEndT);

		fCalcRange *= 0.5f;
		if (x > fCurrentPos)
		{
			fCurrent -= fCalcRange;
		}
		else
		{
			fCurrent += fCalcRange;
		}
	}

	/* 最終的に算出された位置でY座標計算 */
	fTemp1 = 1.0f - fCurrent;
	fTemp2 = fTemp1 * fTemp1;
	fTemp3 = fTemp2 * fTemp1;
	y = (fTemp3 * ppos->fStartV) +
		(3 * fTemp2 * fCurrent * (ppos->fSParamV + ppos->fStartV)) +
		(3 * fTemp1 * fCurrent * fCurrent * (ppos->fEParamV + ppos->fEndV)) +
		(fCurrent * fCurrent * fCurrent * ppos->fEndV);

	return y;
}
#endif

/** ベジェ曲線補間（sswviewで使われているものをひな形にしています） */
static float calcCurveBezier(const InterpolationParams& params, float forwardValue, float backwardValue)
{
	const float fCurrentPos = (params.backwardFrameNo - params.forwardFrameNo) * params.ratio + params.forwardFrameNo;
	float fCurrent = 0.5f;
	float fCalcRange = 0.5f;

	float fTemp1;
	float fTemp2;
	float fTemp3;

	for (int iLoop = 0; iLoop < 8; iLoop++)  /* ループ回数を多くすれば精度が上がる */
	{
		/* X座標計算 */
		fTemp1 = 1.0f - fCurrent;
		fTemp2 = fTemp1 * fTemp1;
		fTemp3 = fTemp2 * fTemp1;
		float x = (fTemp3 * params.forwardFrameNo) +
			(3 * fTemp2 * fCurrent * (params.curve.getStartT() + params.forwardFrameNo)) +
			(3 * fTemp1 * fCurrent * fCurrent * (params.curve.getEndT() + params.backwardFrameNo)) +
			(fCurrent * fCurrent * fCurrent * params.backwardFrameNo);

		fCalcRange *= 0.5f;
		if (x > fCurrentPos)
		{
			fCurrent -= fCalcRange;
		}
		else
		{
			fCurrent += fCalcRange;
		}
	}

	/* 最終的に算出された位置でY座標計算 */
	fTemp1 = 1.0f - fCurrent;
	fTemp2 = fTemp1 * fTemp1;
	fTemp3 = fTemp2 * fTemp1;
	float y = (fTemp3 * forwardValue) +
		(3 * fTemp2 * fCurrent * (params.curve.getStartV() + forwardValue)) +
		(3 * fTemp1 * fCurrent * fCurrent * (params.curve.getEndV() + backwardValue)) +
		(fCurrent * fCurrent * fCurrent * backwardValue);

	return y;
}


/** 前後のキーフレームから補間値を返す */
static float calcCurve(const InterpolationParams& params, float forwardValue, float backwardValue)
{
	float v;
	switch (params.curve.getType())
	{
	case SsCurve::None:
		v = calcCurveNone(params, forwardValue, backwardValue);
		break;
	case SsCurve::Linear:
		v = calcCurveLinear(params, forwardValue, backwardValue);
		break;
	case SsCurve::Hermite:
		v = calcCurveHermite(params, forwardValue, backwardValue);
		break;
	case SsCurve::Bezier:
		v = calcCurveBezier(params, forwardValue, backwardValue);
		break;
	default:
		assert(false);
	}
	return v;
}

/** 前後のキーフレームから補間値を返す（カラーデータ版) */
static SsColor::ColorType calcCurveColor(const InterpolationParams& params, SsColor::ColorType forwardValue, SsColor::ColorType backwardValue)
{
	float v = calcCurve(params, forwardValue, backwardValue);
	return SsColor::clip(v);
}



/** キーフレームタイムラインからフレームの補間値を返す */
static float calcFloatAttribute(SsAttribute::ConstPtr attribute, int frameNo)
{
	const SsAttributeType& attributeType = SsAttributeTag::getType(attribute->getTag());
	SsKeyframe::ConstPtr forward = attribute->getTimeline()->findForward(frameNo);
	SsKeyframe::ConstPtr backward = attribute->getTimeline()->findBackward(frameNo);
	
	float forwardValue = (forward == SsKeyframeTimeline::Invalid)
			? 0 : static_cast<const SsFloatValue*>(forward->getValue())->value;
	float backwardValue = (backward == SsKeyframeTimeline::Invalid)
			? 0 : static_cast<const SsFloatValue*>(backward->getValue())->value;

	if (forward == SsKeyframeTimeline::Invalid)
	{
        if (attribute->getTag() == SsAttributeTag::HIDE)
        {
            // タイムラインが始まるまでは常に非表示にする
            return 1.0f;
        }
        else
        {
            return (backward != SsKeyframeTimeline::Invalid) ?
				backwardValue :
				static_cast<const SsFloatValue*>(attributeType.getDefaultValue())->value;
        }
	}
	else if (backward == SsKeyframeTimeline::Invalid)
	{
		return forwardValue;
	}
	else if (forward == backward)
	{
		return forwardValue;
	}

	// 前後のキーフレームから補間値を返す 
	InterpolationParams params;
	params.forwardFrameNo = forward->getFrameNo();
	params.backwardFrameNo = backward->getFrameNo();
	params.ratio = calcTimeRatio(params.forwardFrameNo, params.backwardFrameNo, frameNo);
	params.curve = forward->getCurve();
	return calcCurve(params, forwardValue, backwardValue);
}


static SsVertex4Value calcVertex4Attribute(SsAttribute::ConstPtr attribute, int frameNo)
{
	const SsAttributeType& attributeType = SsAttributeTag::getType(attribute->getTag());
	SsKeyframe::ConstPtr forward = attribute->getTimeline()->findForward(frameNo);
	SsKeyframe::ConstPtr backward = attribute->getTimeline()->findBackward(frameNo);
	
	SsVertex4Value forwardValue;
	if (forward != SsKeyframeTimeline::Invalid)
		forwardValue = *(static_cast<const SsVertex4Value*>(forward->getValue()));

	SsVertex4Value backwardValue;
	if (backward != SsKeyframeTimeline::Invalid)
		backwardValue = *(static_cast<const SsVertex4Value*>(backward->getValue()));

	if (forward == SsKeyframeTimeline::Invalid)
	{
		return (backward != SsKeyframeTimeline::Invalid) ?
			backwardValue :
			*(static_cast<const SsVertex4Value*>(attributeType.getDefaultValue()));
	}
	else if (backward == SsKeyframeTimeline::Invalid)
	{
		return forwardValue;
	}
	else if (forward == backward)
	{
		return forwardValue;
	}

	// 前後のキーフレームから補間値を返す 
	InterpolationParams params;
	params.forwardFrameNo = forward->getFrameNo();
	params.backwardFrameNo = backward->getFrameNo();
	params.ratio = calcTimeRatio(params.forwardFrameNo, params.backwardFrameNo, frameNo);
	params.curve = forward->getCurve();
	
	SsVertex4Value value;
	for (size_t i = 0; i < 4; i++)
	{
		value.v[i].x = static_cast<int>(calcCurve(params, static_cast<float>(forwardValue.v[i].x), static_cast<float>(backwardValue.v[i].x)));
		value.v[i].y = static_cast<int>(calcCurve(params, static_cast<float>(forwardValue.v[i].y), static_cast<float>(backwardValue.v[i].y)));
	}
	return value;
}


static SsColorBlendValue calcColorBlendAttribute(SsAttribute::ConstPtr attribute, int frameNo)
{
	const SsAttributeType& attributeType = SsAttributeTag::getType(attribute->getTag());
	SsKeyframe::ConstPtr forward = attribute->getTimeline()->findForward(frameNo);
	SsKeyframe::ConstPtr backward = attribute->getTimeline()->findBackward(frameNo);
	
	SsColorBlendValue forwardValue;
	if (forward != SsKeyframeTimeline::Invalid)
		forwardValue = *(static_cast<const SsColorBlendValue*>(forward->getValue()));

	SsColorBlendValue backwardValue;
	if (backward != SsKeyframeTimeline::Invalid)
		backwardValue = *(static_cast<const SsColorBlendValue*>(backward->getValue()));

	if (forward == SsKeyframeTimeline::Invalid)
	{
//		return *(static_cast<const SsColorBlendValue*>(attributeType.getDefaultValue()));
		return (backward != SsKeyframeTimeline::Invalid) ?
			backwardValue :
			*(static_cast<const SsColorBlendValue*>(attributeType.getDefaultValue()));
	}
	else if (backward == SsKeyframeTimeline::Invalid)
	{
		return forwardValue;
	}
	else if (forward == backward)
	{
		return forwardValue;
	}

	// 前後のキーフレームから補間値を返す 
	InterpolationParams params;
	params.forwardFrameNo = forward->getFrameNo();
	params.backwardFrameNo = backward->getFrameNo();
	params.ratio = calcTimeRatio(params.forwardFrameNo, params.backwardFrameNo, frameNo);
	params.curve = forward->getCurve();
	
	SsColorBlendValue value;

	value.blend = forwardValue.blend;

	if (forwardValue.type == SsColorBlendValue::TYPE_COLORTYPE_NONE
	 && backwardValue.type == SsColorBlendValue::TYPE_COLORTYPE_NONE)
	{
		value.type = SsColorBlendValue::TYPE_COLORTYPE_NONE;
	}
	else if (forwardValue.type == SsColorBlendValue::TYPE_COLORTYPE_VERTEX
	 || backwardValue.type == SsColorBlendValue::TYPE_COLORTYPE_VERTEX)
	{
		value.type = SsColorBlendValue::TYPE_COLORTYPE_VERTEX;
	}
	else
	{
		value.type = SsColorBlendValue::TYPE_COLORTYPE_PARTS;
	}
	
	for (size_t i = 0; i < 4; i++)
	{
		if (forwardValue.type == SsColorBlendValue::TYPE_COLORTYPE_NONE)
		{
			value.colors[i].r = calcCurveColor(params, backwardValue.colors[i].r, backwardValue.colors[i].r);
			value.colors[i].g = calcCurveColor(params, backwardValue.colors[i].g, backwardValue.colors[i].g);
			value.colors[i].b = calcCurveColor(params, backwardValue.colors[i].b, backwardValue.colors[i].b);
		}
		else if (backwardValue.type == SsColorBlendValue::TYPE_COLORTYPE_NONE)
		{
			value.colors[i].r = calcCurveColor(params, forwardValue.colors[i].r, forwardValue.colors[i].r);
			value.colors[i].g = calcCurveColor(params, forwardValue.colors[i].g, forwardValue.colors[i].g);
			value.colors[i].b = calcCurveColor(params, forwardValue.colors[i].b, forwardValue.colors[i].b);
		}
		else
		{
			value.colors[i].r = calcCurveColor(params, forwardValue.colors[i].r, backwardValue.colors[i].r);
			value.colors[i].g = calcCurveColor(params, forwardValue.colors[i].g, backwardValue.colors[i].g);
			value.colors[i].b = calcCurveColor(params, forwardValue.colors[i].b, backwardValue.colors[i].b);
		}
		
		value.colors[i].a = calcCurveColor(params, forwardValue.colors[i].a, backwardValue.colors[i].a);
	}
	return value;
}


/**************************************************
 * 
 **************************************************/

static void initFloatValue(SsMotionFrameDecoder::FloatValue& value, const SsAttributeTag::Tag tag)
{
	const SsAttributeType& type = SsAttributeTag::getType(tag);

	value.value = static_cast<const SsFloatValue*>(type.getDefaultValue())->value;
	value.inheritPercent.reset();
	if (type.isInheritable()) value.inheritPercent = type.getDefaultInheritPercent();
}

static void decodeFloatValue(SsMotionFrameDecoder::FloatValue& value, SsAttribute::ConstPtr attr, int frameNo)
{
	if (attr)
	{
		value.value = calcFloatAttribute(attr, frameNo);
		value.inheritPercent = attr->getInheritedPercent();
	}
}


static void initVertex4Value(SsMotionFrameDecoder::Vertex4Value& value, const SsAttributeTag::Tag tag)
{
	const SsAttributeType& type = SsAttributeTag::getType(tag);

	value.value = *(static_cast<const SsVertex4Value*>(type.getDefaultValue()));
	value.inheritPercent.reset();
	if (type.isInheritable()) value.inheritPercent = type.getDefaultInheritPercent();
}

static void decodeVertex4Value(SsMotionFrameDecoder::Vertex4Value& value, SsAttribute::ConstPtr attr, int frameNo)
{
	if (attr)
	{
		value.value = calcVertex4Attribute(attr, frameNo);
		value.inheritPercent = attr->getInheritedPercent();
	}
}


static void initColorBlendValue(SsMotionFrameDecoder::ColorBlendValue& value, const SsAttributeTag::Tag tag)
{
	const SsAttributeType& type = SsAttributeTag::getType(tag);

	value.value = *(static_cast<const SsColorBlendValue*>(type.getDefaultValue()));
	value.inheritPercent.reset();
	if (type.isInheritable()) value.inheritPercent = type.getDefaultInheritPercent();
}

static void decodeColorBlendValue(SsMotionFrameDecoder::ColorBlendValue& value, SsAttribute::ConstPtr attr, int frameNo)
{
	if (attr)
	{
		value.value = calcColorBlendAttribute(attr, frameNo);
		value.inheritPercent = attr->getInheritedPercent();
	}
}


static void initUserDataValue(SsMotionFrameDecoder::UserDataValue& value, const SsAttributeTag::Tag tag)
{
	const SsAttributeType& type = SsAttributeTag::getType(tag);

	value.value = *(static_cast<const SsUserDataValue*>(type.getDefaultValue()));
}

static void decodeUserDataValue(SsMotionFrameDecoder::UserDataValue& value, SsAttribute::ConstPtr attr, int frameNo)
{
	if (attr)
	{
		SsKeyframe::ConstPtr keyframe = attr->getTimeline()->find(frameNo);
		if (keyframe != SsKeyframeTimeline::Invalid)
		{
			value.value = *(static_cast<const SsUserDataValue*>(keyframe->getValue()));
		}
	}
}


SsMotionFrameDecoder::FrameParam::FrameParam(void)
{
	clear();
}

void SsMotionFrameDecoder::FrameParam::clear()
{
	node.reset();

	initFloatValue(posx, SsAttributeTag::POSX);
	initFloatValue(posy, SsAttributeTag::POSY);
	initFloatValue(angl, SsAttributeTag::ANGL);
	initFloatValue(scax, SsAttributeTag::SCAX);
	initFloatValue(scay, SsAttributeTag::SCAY);
	initFloatValue(tran, SsAttributeTag::TRAN);
	initFloatValue(prio, SsAttributeTag::PRIO);
	initFloatValue(flph, SsAttributeTag::FLPH);
	initFloatValue(flpv, SsAttributeTag::FLPV);
	initFloatValue(hide, SsAttributeTag::HIDE);
	initFloatValue(imgx, SsAttributeTag::IMGX);
	initFloatValue(imgy, SsAttributeTag::IMGY);
	initFloatValue(imgw, SsAttributeTag::IMGW);
	initFloatValue(imgh, SsAttributeTag::IMGH);
	initFloatValue(orfx, SsAttributeTag::ORFX);
	initFloatValue(orfy, SsAttributeTag::ORFY);
	initVertex4Value(vert, SsAttributeTag::VERT);
	initColorBlendValue(pcol, SsAttributeTag::PCOL);
	initUserDataValue(udat, SsAttributeTag::UDAT);
}

void SsMotionFrameDecoder::FrameParam::decode(SsNode::ConstPtr node_, int frameNo)
{
	clear();
	
	node = node_;

	decodeFloatValue(posx, node->getAttribute(SsAttributeTag::POSX), frameNo);
	decodeFloatValue(posy, node->getAttribute(SsAttributeTag::POSY), frameNo);
	decodeFloatValue(angl, node->getAttribute(SsAttributeTag::ANGL), frameNo);
	decodeFloatValue(scax, node->getAttribute(SsAttributeTag::SCAX), frameNo);
	decodeFloatValue(scay, node->getAttribute(SsAttributeTag::SCAY), frameNo);
	decodeFloatValue(tran, node->getAttribute(SsAttributeTag::TRAN), frameNo);
	decodeFloatValue(prio, node->getAttribute(SsAttributeTag::PRIO), frameNo);
	decodeFloatValue(flph, node->getAttribute(SsAttributeTag::FLPH), frameNo);
	decodeFloatValue(flpv, node->getAttribute(SsAttributeTag::FLPV), frameNo);
	decodeFloatValue(hide, node->getAttribute(SsAttributeTag::HIDE), frameNo);
	decodeFloatValue(imgx, node->getAttribute(SsAttributeTag::IMGX), frameNo);
	decodeFloatValue(imgy, node->getAttribute(SsAttributeTag::IMGY), frameNo);
	decodeFloatValue(imgw, node->getAttribute(SsAttributeTag::IMGW), frameNo);
	decodeFloatValue(imgh, node->getAttribute(SsAttributeTag::IMGH), frameNo);
	decodeFloatValue(orfx, node->getAttribute(SsAttributeTag::ORFX), frameNo);
	decodeFloatValue(orfy, node->getAttribute(SsAttributeTag::ORFY), frameNo);
	decodeVertex4Value(vert, node->getAttribute(SsAttributeTag::VERT), frameNo);
	decodeColorBlendValue(pcol, node->getAttribute(SsAttributeTag::PCOL), frameNo);
	decodeUserDataValue(udat, node->getAttribute(SsAttributeTag::UDAT), frameNo);
}





/** 優先順位を比較する、優先順位が同じ場合はIDを比較する */
bool SsMotionFrameDecoder::FrameParam::priorityComparator(const FrameParam& lhs, const FrameParam& rhs)
{
	// TODO:当たり判定パーツは最高優先順位（最前面）で扱う 

	float lprio = lhs.prio.value;
	float rprio = rhs.prio.value;
	if (lprio <  rprio) return true;
	if (lprio == rprio && lhs.node->getId() < rhs.node->getId()) return true;
	return false;
}

/** 非表示か判定する */
bool SsMotionFrameDecoder::FrameParam::isHidden(const FrameParam& r)
{
    return r.hide.value != 0 ;
}

/** 完全に透明か判定する */
bool SsMotionFrameDecoder::FrameParam::isInvisible(const FrameParam& r)
{
    return r.tran.value == 0;
}
    
/** ルートパーツか判定する */
bool SsMotionFrameDecoder::FrameParam::isRoot(const FrameParam& r)
{
	return r.node->isRoot() ;
}
    
/** NULLパーツか判定する */
bool SsMotionFrameDecoder::FrameParam::isNullPart(const FrameParam& r)
{
	return r.node->getType() == ss::SsPart::TypeNull;
}

/** 当たり判定、サウンドパーツか判定する */
bool SsMotionFrameDecoder::FrameParam::isHitTestOrSoundPart(const FrameParam& r)
{
	return r.node->getType() == ss::SsPart::TypeHitTest
		|| r.node->getType() == ss::SsPart::TypeSound;
}

/** スケールが０か判定する */
bool SsMotionFrameDecoder::FrameParam::isScaleZero(const FrameParam& r)
{
    return r.scax.value == 0 || r.scay.value == 0;
}

/** ユーザーデータがあるか */
bool SsMotionFrameDecoder::FrameParam::hasUserData(const FrameParam& r)
{
	return r.udat.value.hasData();
}



/** 継承するか判定を行う */
class Inheritance
{
	SsMotionFrameDecoder::FrameParam&		param;
	const SsMotionFrameDecoder::FrameParam&	parentParam;

public:
	Inheritance(SsMotionFrameDecoder::FrameParam& param, const SsMotionFrameDecoder::FrameParam& parentParam)
		: param(param), parentParam(parentParam)
	{
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::posx);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::posy);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::angl);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::scax);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::scay);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::tran, true);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::flph, true);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::flpv, true);
		fixSelfInherit(&SsMotionFrameDecoder::FrameParam::hide, true);
	}
	
	typedef SsMotionFrameDecoder::FloatValue SsMotionFrameDecoder::FrameParam::*FloatValuePtr;
	
	// 親の継承もしくは個別指定されている継承設定から、この時点の継承値を確定させる
	void fixSelfInherit(FloatValuePtr ptr, bool canIndividually = false) const
	{
		if (param.node->isInheritEach())
		{
			// 継承の個別指定
			if (canIndividually)
			{
				// 設定されていなかったら継承しないことにする
				if (!(param.*ptr).inheritPercent) (param.*ptr).inheritPercent = 0.0f;
			}
		}
		else
		{
			// 親の継承値を使用、親の継承値を自信の値とする
			(param.*ptr).inheritPercent = (parentParam.*ptr).inheritPercent;
		}
	}

	bool isInherit(FloatValuePtr ptr) const
	{
		return ((param.*ptr).inheritPercent).get() != 0;
	}
};


/** 親のパラメータをもとに座標変換など行う */
static void calcInheritance(SsMotionFrameDecoder::FrameParam& param, const SsMotionFrameDecoder::FrameParam& parentParam)
{
	Inheritance inheritance(param, parentParam);

	float x = param.posx.value;
	float y = param.posy.value;

	// 継承を反映した左右上下反転値
	bool angleReversed = false;

	// xy値に親のスケール値を反映
//	if (param.scax.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::scax))
	{
		x *= parentParam.scax.value;
	}
//	if (param.scay.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::scay))
	{
		y *= parentParam.scay.value;
	}

	// 回転を反映 
//	if (param.angl.inheritPercent && parentParam.angl.value != 0)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::angl) && parentParam.angl.value != 0)
	{
		float dangle = parentParam.angl.value;
		double a = dangle * -1;
		double asin = std::sin(a);
		double acos = std::cos(a);
		float rx = static_cast<float>( x * acos - y * asin );
		float ry = static_cast<float>( x * asin + y * acos );
		x = rx;
		y = ry;
	}


//	if (param.posx.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::posx))
	{
		param.posx.value =
			parentParam.posx.value + x;
	}
//	if (param.posy.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::posy))
	{
		param.posy.value =
			parentParam.posy.value + y;
	}

//	if (param.angl.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::angl))
	{
		if (angleReversed)
		{
			param.angl.value =
				parentParam.angl.value -
				param.angl.value;
		}
		else
		{
			param.angl.value +=
				parentParam.angl.value;
		}
	}

//	if (parentParam.flph.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::flph))
	{
		param.flph.value =
			static_cast<float>(
			static_cast<int>(param.flph.value) ^
			static_cast<int>(parentParam.flph.value));
	}
//	if (parentParam.flpv.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::flpv))
	{
		param.flpv.value =
			static_cast<float>(
			static_cast<int>(param.flpv.value) ^
			static_cast<int>(parentParam.flpv.value));
	}

//	if (param.scax.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::scax))
	{
		param.scax.value *= parentParam.scax.value;
	}
//	if (param.scay.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::scay))
	{
		param.scay.value *= parentParam.scay.value;
	}

//	if (param.hide.inheritPercent)
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::hide))
	{
//		if (parentParam.hide.value != 0)
//		{
			param.hide.value = parentParam.hide.value;
//		}
	}
	
	if (inheritance.isInherit(&SsMotionFrameDecoder::FrameParam::tran))
	{
		param.tran.value *= parentParam.tran.value;
	}
}

static void decodeNodesSub(std::vector<SsMotionFrameDecoder::FrameParam>& paramList, SsNode::ConstPtr node, int frameNo, const SsMotionFrameDecoder::FrameParam& parentParam, int depth, bool isRootOrigin)
{
	// 自分自身のパラメータを展開する 
	SsMotionFrameDecoder::FrameParam param;

	if (node->hasFrame(frameNo))
	{
		param.decode(node, frameNo);

        if (depth == 0 && isRootOrigin)
        {
            param.posx.value = 0;
            param.posy.value = 0;
        }
        
		// 継承を反映する
		calcInheritance(param, parentParam);
		paramList.push_back(param);
	}

	BOOST_FOREACH( SsNode::ConstPtr child, node->getChildren() )
	{
		decodeNodesSub(paramList, child, frameNo, param, depth + 1, isRootOrigin);
	}
}

void SsMotionFrameDecoder::decodeNodes(std::vector<FrameParam>& paramList, SsMotion::ConstPtr motion, int frameNo, bool isRootOrigin)
{
	paramList.clear();

	FrameParam rootParam;
	decodeNodesSub(paramList, motion->getRootNode(), frameNo, rootParam, 0, isRootOrigin);
}




}	// namespace ss

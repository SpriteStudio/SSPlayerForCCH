#ifndef _SS_MOTION_DECODER_H_
#define _SS_MOTION_DECODER_H_

#include "SsMotion.h"
#include <boost/optional/optional.hpp>

namespace ss
{

struct SsMotionFrameDecoder
{
public:
	struct FloatValue
	{
		float					value;
		boost::optional<float>	inheritPercent;
	};
	
	struct Vertex4Value
	{
		SsVertex4Value			value;
		boost::optional<float>	inheritPercent;
	};
	
	struct ColorBlendValue
	{
		SsColorBlendValue		value;
		boost::optional<float>	inheritPercent;
	};
	
	struct UserDataValue
	{
		SsUserDataValue			value;
	};


	struct FrameParam
	{
		SsNode::ConstPtr	node;
        
        FloatValue          posx;
        FloatValue          posy;
        FloatValue          angl;
        FloatValue          scax;
        FloatValue          scay;
        FloatValue          tran;
        FloatValue          prio;
        FloatValue          flph;
        FloatValue          flpv;
        FloatValue          hide;
        FloatValue          imgx;
        FloatValue          imgy;
        FloatValue          imgw;
        FloatValue          imgh;
        FloatValue          orfx;
        FloatValue          orfy;
        Vertex4Value		vert;
        ColorBlendValue		pcol;
		UserDataValue		udat;
		

		FrameParam(void);
		void clear();
		void decode(SsNode::ConstPtr node, int frameNo);


		/** 優先順位を比較する、優先順位が同じ場合はIDを比較する */
		static bool priorityComparator(const FrameParam& lhs, const FrameParam& rhs);
        
        /** 非表示か判定する */
        static bool isHidden(const FrameParam& r);
        
        /** 完全に透明か判定する */
        static bool isInvisible(const FrameParam& r);
        
        /** ルートパーツか判定する */
        static bool isRoot(const FrameParam& r);
		
		/** NULLパーツか判定する */
		static bool isNullPart(const FrameParam& r);
		
		/** 当たり判定、サウンドパーツか判定する */
		static bool isHitTestOrSoundPart(const FrameParam& r);
        
        /** スケールが０か判定する */
        static bool isScaleZero(const FrameParam& r);
		
		/** ユーザーデータがあるか */
		static bool hasUserData(const FrameParam& r);
	};

	/** キーフレームタイムラインからフレームの補間値を返す */
	static float decode(SsAttribute::ConstPtr attribute, int frameNo);

	static void decodeNodes(std::vector<FrameParam>& paramList, SsMotion::ConstPtr motion, int frameNo, bool isRootOrigin = false);
};

}

#endif	// ifndef _SS_MOTION_DECODER_H_

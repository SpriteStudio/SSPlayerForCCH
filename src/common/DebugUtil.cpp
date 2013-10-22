//
//  DebugUtil.cpp
//

#include "DebugUtil.h"
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include "MathUtil.h"
#include "TextEncoding.h"

using boost::format;


namespace ss
{

namespace debug
{
	
	void dumpPrefix(std::ostream& out, int frameNo)
	{
		out << "/* " << std::endl;
		out << format("frameNo %1%") % frameNo << std::endl;
	}

	void dumpSuffix(std::ostream& out)
	{
		out << "*/" << std::endl;
	}

	void dumpFrameParamList(std::ostream& out, const std::vector<SsMotionFrameDecoder::FrameParam>& paramList)
	{
		BOOST_FOREACH( const SsMotionFrameDecoder::FrameParam& param, paramList )
		{
			debug::dumpFrameParam(out, param);
		}
	}

	void dumpFrameParam(std::ostream& out, const ss::SsMotionFrameDecoder::FrameParam& param)
	{
		out << format("part[%1%,%2%], ") % param.node->getId() % param.node->getName();
		out << format("posx[%1%], ") % param.posx.value;
		out << format("posy[%1%], ") % param.posy.value;
		out << format("scax[%1%], ") % param.scax.value;
		out << format("scay[%1%], ") % param.scay.value;
		out << format("angl[rad:%1% deg:%2%], ") % param.angl.value % mathutil::radianToDegree(param.angl.value);
		out << format("hide[%1%,%2%], ") % param.hide.value % (param.hide.inheritPercent ? param.hide.inheritPercent.get() : -1);
		out << format("tran[%1%,%2%], ") % param.tran.value % (param.tran.inheritPercent ? param.tran.inheritPercent.get() : -1);
		out << format("flph[%1%,%2%], ") % param.flph.value % (param.flph.inheritPercent ? param.flph.inheritPercent.get() : -1);
		out << format("flpv[%1%,%2%], ") % param.flpv.value % (param.flpv.inheritPercent ? param.flpv.inheritPercent.get() : -1);
		
		if (param.udat.value.hasData())
		{
			const SsUserDataValue& value = param.udat.value;
		
			out << "udat[";
			if (value.number)
			{
				out << format("number(%1%) ") % value.number.get();
			}
			if (value.rect)
			{
				const SsRect rect = value.rect.get();
				out << format("rect(%1%,%2%,%3%,%4%) ") % rect.getLeft() % rect.getTop() % rect.getRight() % rect.getBottom();
			}
			if (value.point)
			{
				const SsPoint point = value.point.get();
				out << format("point(%1%, %2%) ") % point.x % point.y;
			}
			if (value.str)
			{
				std::string s = textenc::convert(value.str.get(), textenc::SHIFT_JIS, textenc::UTF8);
				out << format("string(%1%)") % s;
			}
			out << "]";
		}

		out << std::endl;
	}


}	// namespace debug

}	// namespace ss


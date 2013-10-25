#include "MathUtil.h"

namespace mathutil
{
	static const double	PI = 3.14159265358979323846;

	float degreeToRadian(float degree)
	{
		return degree * static_cast<float>(PI) / 180.0f;
	}

	float radianToDegree(float radian)
	{
		return radian * 180.0f / static_cast<float>(PI);
	}

};



#pragma once

__inline BYTE FarColor_3_2(const FarColor& Color3)
{
	BYTE Color2 = 0;
	if ((Color3.Flags & (FCF_FG_4BIT|FCF_BG_4BIT)) == (FCF_FG_4BIT|FCF_BG_4BIT))
	{
		Color2 = (Color3.ForegroundColor & 0xF) | ((Color3.BackgroundColor & 0xF) << 4);
	}
	else
	{
		// #include "..\common\MAssert.h"
		_ASSERTE((Color3.Flags & (FCF_FG_4BIT|FCF_BG_4BIT)) == (FCF_FG_4BIT|FCF_BG_4BIT) || Color3.Flags == 0);
	}
	return Color2;
}

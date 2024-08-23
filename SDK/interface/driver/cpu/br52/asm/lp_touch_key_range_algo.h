#ifndef  __LP_TOUCH_KEY_RANGE_ALGO_H__
#define  __LP_TOUCH_KEY_RANGE_ALGO_H__


#include "typedef.h"


void TouchRangeAlgo_Init(u8 ch, u16 min, u16 max);
void TouchRangeAlgo_Update(u8 ch, u16 x);
void TouchRangeAlgo_Reset(u8 ch, u16 min, u16 max);

u16 TouchRangeAlgo_GetRange(u8 ch, u8 *valid);
void TouchRangeAlgo_SetRange(u8 ch, u16 range);

s32 TouchRangeAlgo_GetSigma(u8 ch);
void TouchRangeAlgo_SetSigma(u8 ch, s32 sigma);


#endif


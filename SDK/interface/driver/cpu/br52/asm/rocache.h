#ifndef __Q32DSP_DCACHE__
#define __Q32DSP_DCACHE__

//*********************************************************************************//
// Module name : rocache.h                                                         //
// Description : q32DSP dcache control head file                                   //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

//  ------  ------   ------
//  | c0 |  | c2 |   | pp |
//  ------  ------   ------
//    |_______|________|
//            |
//        ---------
//        |  L1i  |
//        ---------
//            |
//        ---------
//        | flash |
//        ---------

#define INCLUDE_DCU_RPT      0
#define INCLUDE_DCU_EMU      0

//------------------------------------------------------//
// dcache function
//------------------------------------------------------//
void DcuEnable(void);
void DcuDisable(void);
void DcuInitial(void);
void DcuWaitIdle(void);
void DcuSetWayNum(unsigned int way);

void DcuFlushinvAll(void);
void DcuFlushinvRegion(unsigned int *beg, unsigned int len);            // note len!=0
void DcuUnlockAll(void);
void DcuUnlockRegion(unsigned int *beg, unsigned int len);              // note len!=0
void DcuPfetchRegion(unsigned int *beg, unsigned int len);              // note len!=0
void DcuLockRegion(unsigned int *beg, unsigned int len);                // note len!=0

void DcuReportEnable(void);
void DcuReportDisable(void);
void DcuReportPrintf(void);
void DcuReportClear(void);

void DcuEmuEnable(void);
void DcuEmuDisable(void);
void DcuEmuMessage(void);

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
#endif


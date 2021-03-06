/* @(#) pf_cglue.c 98/02/11 1.4 */
/***************************************************************
** 'C' Glue support for Forth based on 'C'
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, David Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
***************************************************************/

#include "pf_all.h"

extern CFunc0 CustomFunctionTable[];

/***************************************************************/
cell_t CallUserFunction(cell_t Index, int32_t ReturnMode, int32_t NumParams)
{
  cell_t P1, P2, P3, P4, P5;
  cell_t Result = 0;
  CFunc0 CF;

  printf("CallUserFunction: Index = %d, ReturnMode = %d, NumParams = %d\n",
         Index, ReturnMode, NumParams);

  CF = CustomFunctionTable[Index];

  switch (NumParams) {
  case 0:
    Result = ((CFunc0)CF)();
    break;
  case 1:
    P1 = POP_DATA_STACK;
    Result = ((CFunc1)CF)(P1);
    break;
  case 2:
    P2 = POP_DATA_STACK;
    P1 = POP_DATA_STACK;
    Result = ((CFunc2)CF)(P1, P2);
    break;
  case 3:
    P3 = POP_DATA_STACK;
    P2 = POP_DATA_STACK;
    P1 = POP_DATA_STACK;
    Result = ((CFunc3)CF)(P1, P2, P3);
    break;
  case 4:
    P4 = POP_DATA_STACK;
    P3 = POP_DATA_STACK;
    P2 = POP_DATA_STACK;
    P1 = POP_DATA_STACK;
    Result = ((CFunc4)CF)(P1, P2, P3, P4);
    break;
  case 5:
    P5 = POP_DATA_STACK;
    P4 = POP_DATA_STACK;
    P3 = POP_DATA_STACK;
    P2 = POP_DATA_STACK;
    P1 = POP_DATA_STACK;
    Result = ((CFunc5)CF)(P1, P2, P3, P4, P5);
    break;
  default:
    pfReportError("CallUserFunction", PF_ERR_NUM_PARAMS);
    EXIT(1);
  }

  /* Push result on Forth stack if requested. */
  if (ReturnMode == C_RETURNS_VALUE)
    PUSH_DATA_STACK(Result);

  return Result;
}

//#if (!defined(PF_NO_INIT)) && (!defined(PF_NO_SHELL))
/***************************************************************/
Err CreateGlueToC(const char* CName, ucell_t Index, cell_t ReturnMode, int32_t NumParams)
{
  printf("CreateGlueToC %s\n", CName);
  ucell_t Packed;
  char FName[40];

  CStringToForth(FName, CName, sizeof(FName));
  Packed = (Index & 0xFFFF) | 0 | (NumParams << 24) | (ReturnMode << 31);
  DBUG(("Packed = 0x%8x\n", Packed));

  ffCreateSecondaryHeader(FName);
  CODE_COMMA(ID_CALL_C);
  CODE_COMMA(Packed);
  ffFinishSecondary();

  return 0;
}
//#endif

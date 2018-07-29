/* @(#) pfcustom.c 98/01/26 1.3 */

#include "pf_all.h"

#ifndef PF_USER_CUSTOM

/***************************************************************
** Call Custom Functions for pForth
**
** Create a file similar to this and compile it into pForth
** by setting -DPF_USER_CUSTOM="mycustom.c"
**
** Using this, you could, for example, call X11 from Forth.
** See "pf_cglue.c" for more information.
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

static cell_t CTest0(cell_t Val);
static void CTest1(cell_t Val1, cell_t Val2);
static cell_t WGet(cell_t curl, cell_t cfilename);

/****************************************************************
** Step 1: Put your own special glue routines here
**     or link them in from another file or library.
****************************************************************/
static cell_t CTest0(cell_t Val)
{
  MSG_NUM_D("CTest0: Val = ", Val);
  return Val + 10;
}

static void CTest1(cell_t Val1, cell_t Val2)
{
  MSG("CTest1: Val1 = ");
  ffDot(Val1);
  MSG_NUM_D(", Val2 = ", Val2);
}

extern char* HttpGet(const char* url);

static cell_t WGet(cell_t curl, cell_t cfilename)
{
  const char* url = (const char*)url;
  //  const char *filename = (const char *)cfilename;

  char* data = HttpGet(url);

  char* src = data;
  while (*src && *src != '\n' && src[1] != '\n')
    src++;
  if (!*src) {
    return 0;
  }
  src++;
  printf("---\n%s\n---\n", src);
  return -1;
}

/****************************************************************
** Step 2: Create CustomFunctionTable.
**     Do not change the name of CustomFunctionTable!
**     It is used by the pForth kernel.
****************************************************************/

#ifdef PF_NO_GLOBAL_INIT
/******************
** If your loader does not support global initialization, then you
** must define PF_NO_GLOBAL_INIT and provide a function to fill
** the table. Some embedded system loaders require this!
** Do not change the name of LoadCustomFunctionTable()!
** It is called by the pForth kernel.
*/
#define NUM_CUSTOM_FUNCTIONS (3)
CFunc0 CustomFunctionTable[NUM_CUSTOM_FUNCTIONS];

Err LoadCustomFunctionTable(void)
{
  CustomFunctionTable[0] = (CFunc0)WGet;
  CustomFunctionTable[1] = (CFunc0)CTest0;
  CustomFunctionTable[2] = (CFunc0)CTest1;
  return 0;
}

#else
// gets executed
/******************
** If your loader supports global initialization (most do.) then just
** create the table like this.
*/
CFunc0 CustomFunctionTable[] = {
  (CFunc0)WGet,   //
  (CFunc0)CTest0, //
  (CFunc0)CTest1, //
};
#endif

/****************************************************************
** Step 3: Add custom functions to the dictionary.
**     Do not change the name of CompileCustomFunctions!
**     It is called by the pForth kernel.
****************************************************************/

//#if (!defined(PF_NO_INIT)) && (!defined(PF_NO_SHELL))
Err CompileCustomFunctions(void)
{
  printf("CompileCustomFunctions\n");
  Err err;
  int i = 0;
  /* Compile Forth words that call your custom functions.
  ** Make sure order of functions matches that in LoadCustomFunctionTable().
  ** Parameters are: Name in UPPER CASE, Function, Index, Mode, NumParams
  */
  err = CreateGlueToC("WGET", i++, C_RETURNS_VALUE, 2);
  if (err < 0) {
    printf("CreateGlueToC WGET failed\n");
    return err;
  }
  err = CreateGlueToC("CTEST0", i++, C_RETURNS_VALUE, 1);
  if (err < 0)
    return err;
  err = CreateGlueToC("CTEST1", i++, C_RETURNS_VOID, 2);
  if (err < 0)
    return err;
  return 0;
}
//#else
// gets executed
// Err CompileCustomFunctions(void)
//{
//  printf("CompileCustomFunctions stub\n");
//  return 0;
//}
//#endif

/****************************************************************
** Step 4: Recompile using compiler option PF_USER_CUSTOM
**         and link with your code.
**         Then rebuild the Forth using "pforth -i system.fth"
**         Test:   10 Ctest0 ( should print message then '11' )
****************************************************************/

#endif /* PF_USER_CUSTOM */

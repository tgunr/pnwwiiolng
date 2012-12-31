#ifdef TESTMAC

#include <locale.h>
#include "testmac/testmac.h"

__attribute__ ((constructor)) extern void MyInitFunc();
__attribute__ ((destructor)) extern void MyTermFunc();

__attribute__ ((constructor)) void MyInitFunc()
{
  setlocale(LC_NUMERIC, "en_US.utf-8"); /* important */
  init_testmac();
}

__attribute__ ((destructor)) void MyTermFunc()
{
  exit_testmac();
}

#endif

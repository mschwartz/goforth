#include <stdio.h>

extern void pf_main();
extern void init_sdcard(void);

void app_main() {
  init_sdcard();
  // call pforth's main
  pf_main();
}

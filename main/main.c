#include <stdio.h>
#include <driver/uart.h>

extern void pf_main();
extern void init_sdcard(void);
extern void StartWiFi();

void app_main()
{
  uart_set_baudrate(UART_NUM_0, 921600);
  //  StartWiFi();
  init_sdcard();
  // call pforth's main
  pf_main();
}

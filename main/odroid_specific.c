#include <fcntl.h>
#include <freertos/FreeRTOS.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define FTRUE (-1)
#define FFALSE (0)

static int putback = -1;

int sdTerminalOut(char c) { return putchar(c); }
int sdTerminalEcho(char c) {
  putchar(c);
  return 0;
}

int sdTerminalFlush(void) {
  //
  return fflush(stdout);
}

int sdQueryTerminal(void) {
  //  usleep(1);
  if (putback != -1) {
    return FTRUE;
  }
  putback = getchar();
  return putback != -1 ? FTRUE : FFALSE;
}

int sdTerminalIn(void) {
  int result;
  while (putback == -1) {
    putback = getchar();
    //    usleep(1);
  }
  result = putback;
  putback = -1;
  return result;
}
void sdTerminalInit(void) {}
void sdTerminalTerm(void) {}

void *AllocMem(intptr_t numBytes) {
  //
  return heap_caps_malloc(numBytes, MALLOC_CAP_SPIRAM);
}
void FreeMem(void *ptr) { free(ptr); }

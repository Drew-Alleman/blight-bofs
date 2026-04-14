/*
 * test_bof.c — Baseline pipeline sanity check.
 *
 * Compile:
 *   x86_64-w64-mingw32-gcc -c test_bof.c -o ../test_bof.x64.o
 *
 * Expected output (no args):
 *   BOF executed successfully! args_len=0
 *
 * Expected output (with args buffer):
 *   BOF executed successfully! args_len=<N>
 */

#include <windows.h>
#include "beacon.h"

void go(char *args, int args_len) {
    BeaconPrintf(CALLBACK_OUTPUT, "BOF executed successfully! args_len=%d", args_len);
}

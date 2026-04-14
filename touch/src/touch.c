/*
 * touch.c — A BOF to mimic the touch utility
 *  :) made by Drew Alleman <3
 *
 * Usage:
 *   touch C:\file.txt                        create + stamp now
 *   touch C:\file.txt 01131500               -t stamp to MMDDhhmm
 *   touch C:\file.txt "2025-01-13 15:00"     -d stamp to freeform date
 *   touch C:\file.txt "" -c                  stamp now if exists, else nothing
 *   touch C:\file.txt 01131500 -c            stamp if exists, else nothing
 *
 * Compile:
 *   x86_64-w64-mingw32-gcc -c touch.c -o ../touch.x64.o -I../../
 */

#include <windows.h>
#include "../../beacon.h"

DECLSPEC_IMPORT DWORD   WINAPI KERNEL32$GetFileAttributesA(LPCSTR);
DECLSPEC_IMPORT HANDLE  WINAPI KERNEL32$CreateFileA(LPCSTR, DWORD, DWORD, LPVOID, DWORD, DWORD, HANDLE);
DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$CloseHandle(HANDLE);
DECLSPEC_IMPORT DWORD   WINAPI KERNEL32$GetLastError(void);
DECLSPEC_IMPORT void    WINAPI KERNEL32$GetSystemTimeAsFileTime(LPFILETIME);
DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$SetFileTime(HANDLE, const FILETIME*, const FILETIME*, const FILETIME*);
DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$SystemTimeToFileTime(const SYSTEMTIME*, LPFILETIME);
DECLSPEC_IMPORT void    WINAPI KERNEL32$GetLocalTime(LPSYSTEMTIME);
DECLSPEC_IMPORT INT     WINAPI KERNEL32$MultiByteToWideChar(UINT, DWORD, LPCSTR, int, LPWSTR, int);
DECLSPEC_IMPORT HMODULE WINAPI KERNEL32$LoadLibraryA(LPCSTR);
DECLSPEC_IMPORT FARPROC WINAPI KERNEL32$GetProcAddress(HMODULE, LPCSTR);
DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$FreeLibrary(HMODULE);

// BOFs can't link against CRT — inline strlen replacement
static int _len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

BOOL createFile(char *filepath) {
    HANDLE hFile = KERNEL32$CreateFileA(filepath, GENERIC_WRITE, 0, NULL,
                                        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        BeaconPrintf(CALLBACK_ERROR, "[-] Failed to create %s (error: %d)", filepath, KERNEL32$GetLastError());
        return FALSE;
    }
    KERNEL32$CloseHandle(hFile);
    BeaconPrintf(CALLBACK_OUTPUT, "[+] created file: %s\n", filepath);
    return TRUE;
}

BOOL touchNow(char *filepath) {
    HANDLE hFile = KERNEL32$CreateFileA(filepath, FILE_WRITE_ATTRIBUTES, 0, NULL,
                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        BeaconPrintf(CALLBACK_ERROR, "[-] Failed to open %s (error: %d)", filepath, KERNEL32$GetLastError());
        return FALSE;
    }
    FILETIME now;
    KERNEL32$GetSystemTimeAsFileTime(&now);
    KERNEL32$SetFileTime(hFile, &now, &now, &now);
    KERNEL32$CloseHandle(hFile);
    BeaconPrintf(CALLBACK_OUTPUT, "[+] touch %s -> current time", filepath);
    return TRUE;
}

  BOOL parseTimestamp(char *timestamp, FILETIME *ft) {
      char buf[16] = {0};
      int seconds = 0;
      int len = _len(timestamp);

      // Strip .ss suffix if present
      char *dot = NULL;
      for (int i = 0; i < len; i++) {
          if (timestamp[i] == '.') { dot = &timestamp[i]; break; }
      }
      if (dot) {
          seconds = (dot[1] - '0') * 10 + (dot[2] - '0');
          len = (int)(dot - timestamp);
      }

      // Copy just the numeric part
      for (int i = 0; i < len && i < 15; i++) buf[i] = timestamp[i];

      int year, month, day, hour, minute;

      if (len == 8) {
          // MMDDhhmm — use current year
          SYSTEMTIME now;
          KERNEL32$GetLocalTime(&now);
          year   = now.wYear;
          month  = (buf[0] - '0') * 10 + (buf[1] - '0');
          day    = (buf[2] - '0') * 10 + (buf[3] - '0');
          hour   = (buf[4] - '0') * 10 + (buf[5] - '0');
          minute = (buf[6] - '0') * 10 + (buf[7] - '0');
      } else if (len == 10) {
          // YYMMDDhhmm
          year   = 2000 + (buf[0] - '0') * 10 + (buf[1] - '0');
          month  = (buf[2] - '0') * 10 + (buf[3] - '0');
          day    = (buf[4] - '0') * 10 + (buf[5] - '0');
          hour   = (buf[6] - '0') * 10 + (buf[7] - '0');
          minute = (buf[8] - '0') * 10 + (buf[9] - '0');
      } else if (len == 12) {
          // CCYYMMDDhhmm
          year   = (buf[0] - '0') * 1000 + (buf[1] - '0') * 100 +
                   (buf[2] - '0') * 10   + (buf[3] - '0');
          month  = (buf[4] - '0') * 10 + (buf[5] - '0');
          day    = (buf[6] - '0') * 10 + (buf[7] - '0');
          hour   = (buf[8] - '0') * 10 + (buf[9] - '0');
          minute = (buf[10] - '0') * 10 + (buf[11] - '0');
      } else {
          return FALSE;
      }

      // Validate ranges
      if (month < 1 || month > 12 || day < 1 || day > 31 ||
          hour > 23 || minute > 59 || seconds > 59) {
          return FALSE;
      }

      SYSTEMTIME st = {0};
      st.wYear   = year;
      st.wMonth  = month;
      st.wDay    = day;
      st.wHour   = hour;
      st.wMinute = minute;
      st.wSecond = seconds;

      return KERNEL32$SystemTimeToFileTime(&st, ft);
  }

  BOOL parseDateString(char *datestr, FILETIME *ft) {
      HMODULE hOle = KERNEL32$LoadLibraryA("oleaut32.dll");
      if (!hOle) return FALSE;

      typedef HRESULT (WINAPI *fnVarDateFromStr)(LPCOLESTR, LCID, ULONG, DATE*);
      typedef INT (WINAPI *fnVariantTimeToSystemTime)(DATE, LPSYSTEMTIME);

      fnVarDateFromStr pVarDate = (fnVarDateFromStr)KERNEL32$GetProcAddress(hOle, "VarDateFromStr");
      fnVariantTimeToSystemTime pVarTime = (fnVariantTimeToSystemTime)KERNEL32$GetProcAddress(hOle, "VariantTimeToSystemTime");

      if (!pVarDate || !pVarTime) {
          KERNEL32$FreeLibrary(hOle);
          return FALSE;
      }

      wchar_t wide[256] = {0};
      KERNEL32$MultiByteToWideChar(65001, 0, datestr, -1, wide, 256);

      DATE dt;
      if (pVarDate(wide, 0x0409, 0, &dt) != 0) {
          KERNEL32$FreeLibrary(hOle);
          return FALSE;
      }

      SYSTEMTIME st = {0};
      if (!pVarTime(dt, &st)) {
          KERNEL32$FreeLibrary(hOle);
          return FALSE;
      }

      BOOL success = KERNEL32$SystemTimeToFileTime(&st, ft);
      KERNEL32$FreeLibrary(hOle);
      return success;
  }

  BOOL stompFile(char *filepath, char *timestamp) {
      FILETIME ft;
      // Try -t format first (MMDDhhmm), then -d freeform date string
      if (!parseTimestamp(timestamp, &ft) && !parseDateString(timestamp, &ft)) {
          BeaconPrintf(CALLBACK_ERROR, "[-] Invalid timestamp: %s", timestamp);
          return FALSE;
      }

      HANDLE hFile = KERNEL32$CreateFileA(filepath, FILE_WRITE_ATTRIBUTES, 0, NULL,
                                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (hFile == INVALID_HANDLE_VALUE) {
          BeaconPrintf(CALLBACK_ERROR, "[-] Failed to open %s (error: %d)", filepath, KERNEL32$GetLastError());
          return FALSE;
      }

      KERNEL32$SetFileTime(hFile, &ft, &ft, &ft);
      KERNEL32$CloseHandle(hFile);
      BeaconPrintf(CALLBACK_OUTPUT, "[+] touch %s -> %s", filepath, timestamp);
      return TRUE;
  }

void go(char *args, int args_len) {
    datap parser;
    BeaconDataParse(&parser, args, args_len);

    char *filepath  = BeaconDataExtract(&parser, NULL);
    char *timestamp = BeaconDataExtract(&parser, NULL);
    char *flags     = BeaconDataExtract(&parser, NULL);
    BOOL no_create = (flags && flags[0] != '\0');

    if (!filepath || filepath[0] == '\0') {
        BeaconPrintf(CALLBACK_ERROR, "[-] No file path provided");
        return;
    }

    BOOL has_timestamp = (timestamp && timestamp[0] != '\0');

    if (has_timestamp) {
        int len = _len(timestamp);
        // Only enforce minimum length for numeric -t format, not -d freeform
        BOOL is_numeric = TRUE;
        for (int i = 0; i < len; i++) {
            if (timestamp[i] != '.' && (timestamp[i] < '0' || timestamp[i] > '9')) {
                is_numeric = FALSE;
                break;
            }
        }
        if (is_numeric && len < 8) {
            BeaconPrintf(CALLBACK_ERROR,
                "[-] Timestamp must be at least 8 characters: MMDDhhmm "
                "(e.g. 01131500 = January 13, 3:00 PM, current year)");
            return;
        }
    }

    DWORD attrs = KERNEL32$GetFileAttributesA(filepath);
    BOOL file_exists = (attrs != INVALID_FILE_ATTRIBUTES);

      if (has_timestamp) {
          if (!file_exists) {
              if (no_create) return;
              if (!createFile(filepath)) return;
          }
          stompFile(filepath, timestamp);
      } else if (!file_exists) {
          if (no_create) return;
          createFile(filepath);
      } else {
          touchNow(filepath);
      }
}

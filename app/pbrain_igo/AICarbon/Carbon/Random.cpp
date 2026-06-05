#ifndef LINUX
#include <windows.h>
#else
#include <stdio.h>
#include <time.h>
#endif

// We can't use standard function rand() from stdlib because it does not work.
// It returns same value for every move because OXMain.cpp creates new thread for every move.

#ifndef LINUX
static DWORD seed;
#else
static unsigned int seed;

// 返回自系统开机以来的毫秒数（tick）
unsigned long long GetTickCount()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

void _randomize()
{
  seed = GetTickCount();
}

unsigned _random(unsigned x)
{
  seed = seed * 367413989 + 174680251;
  
#ifndef LINUX  
  return (unsigned)(UInt32x32To64(x, seed) >> 32);
#else
  return (unsigned)((x*(unsigned long long)seed) >> 32);
#endif
}

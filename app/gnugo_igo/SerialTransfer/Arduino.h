#ifndef __ARDUINO_H__
#define __ARDUINO_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uint8_t;  /* unsigned 8 bits. */
typedef unsigned long uint16_t;  /* unsigned 16 bits. */
typedef unsigned long uint32_t;  /* unsigned 32 bits. */
typedef unsigned long long uint64_t;  /* unsigned 64 bits. */

#ifdef MIPS
typedef char int8_t;  /* signed 8 bits. */
typedef short int16_t;  /* signed 16 bits. */
typedef long int32_t;  /* signed 32 bits. */
typedef long long int64_t;  /* signed 64 bits. */
#endif

typedef unsigned char byte;

class Stream
{
  public:
  void init(int fd_in)
  {
   fd = fd_in;
   first_byte_read = 0;
  }

  void init(FILE *file_in)
  {
   file = file_in;
   first_byte_read = 0;
  }

  int available();
  unsigned char read();
  int write(unsigned char[], int);

  size_t print(const char[]);
  size_t println(const char[]);
  size_t println(unsigned char);
  size_t println(int);

  private:
  int fd;
  FILE *file;
  int first_byte_read;
  unsigned char c;
};

extern Stream Serial;

#define F(x) x

unsigned int millis();

#endif /*__ARDUINO_H__*/


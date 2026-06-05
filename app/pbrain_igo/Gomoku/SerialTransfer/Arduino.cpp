#include <stdio.h>
#include <time.h>
#include "Arduino.h"

// 返回自系统开机以来的毫秒数（tick）
unsigned int millis()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (unsigned int)((long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

//#define DEBUG

#ifdef DEBUG
static void dump_data(unsigned char *buf,int n)
{
  int i;

   printf("write{");
   for(i=0;i<n;i++){
     if(i%16==0) printf("\n");
      printf("0x%02x ",*(buf+i));
    };
   printf("}\n");
   return;
}
#endif

int Stream::available()
{
  int n;

 if( first_byte_read ) return 1;

  n = ::read(fd,&c,1);
  if( n == 1 ){
  first_byte_read = 1;
  return 1;
  }
  else {
  first_byte_read = 0;
  return 0;
  }
}

unsigned char Stream::read()
{
  int n;
  if( first_byte_read )
  {
  first_byte_read = 0;
#ifdef DEBUG
  printf("read 0x%02x\n",c);
#endif
  return c;
  }
  else
  {
    printf("Error in read\n");
    return 0xFF;
  }
}

int Stream::write(unsigned char buf[], int len)
{
#ifdef DEBUG
  dump_data(buf,len);
#endif

  return ::write(fd, buf, len);
}

size_t Stream::print(const char str[])
{
  fprintf(file,"%s",str);
}

size_t Stream::println(const char str[])
{
  fprintf(file,"%s\n",str);
}

size_t Stream::println(unsigned char c)
{
  fprintf(file,"0x%02x\n", c);
}

size_t Stream::println(int n)
{
  fprintf(file,"%d\n", n);
}


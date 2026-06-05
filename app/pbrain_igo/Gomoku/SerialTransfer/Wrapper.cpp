#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SerialTransfer.h"
#include "Wrapper.h"

SerialTransfer CmdTransfer;
Stream Serial;
Stream CmdSerial;

void TransferBegin(int fd)
{
  Serial.init(stderr);
  CmdSerial.init(fd);
  CmdTransfer.begin(CmdSerial);
}

unsigned char TransferAvailable(void)
{
  return CmdTransfer.available();
}

unsigned char TransferGetPacketID(void)
{
  return CmdTransfer.currentPacketID();
}

unsigned char *TransferGetBuffer(void)
{
  return CmdTransfer.packet.rxBuff;
}

unsigned char TransferGetbytesRead(void)
{
  return CmdTransfer.packet.bytesRead;
}

int TransferSendCommand(int cmd, unsigned char *buf, int len)
{
  unsigned short sendSize = 0;
  unsigned char obj_buf[MAX_PACKET_SIZE];
  
  if( len > MAX_PACKET_SIZE ) return 0;

  memcpy(obj_buf, buf, len);
  sendSize = CmdTransfer.txObj(obj_buf, 0, len);
  return CmdTransfer.sendData(sendSize, cmd); 
}

int TransferGetObj(unsigned char *buf, int len)
{
  unsigned short recvSize = 0;
  unsigned char obj_buf[MAX_PACKET_SIZE];
  
  if( len > MAX_PACKET_SIZE ) return 0;

  recvSize = CmdTransfer.rxObj(obj_buf, 0, len);
  memcpy(buf, obj_buf, recvSize);
  return recvSize;
}

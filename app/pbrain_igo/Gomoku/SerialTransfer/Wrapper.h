#ifdef __cplusplus
extern "C"{
#endif

void TransferBegin(int fd);

unsigned char TransferAvailable(void);

unsigned char TransferGetPacketID(void);

unsigned char *TransferGetBuffer(void);

unsigned char TransferGetbytesRead(void);

int TransferSendCommand(int cmd, unsigned char *buf, int len);

int TransferGetObj(unsigned char *buf, int len);

#ifdef __cplusplus
}
#endif

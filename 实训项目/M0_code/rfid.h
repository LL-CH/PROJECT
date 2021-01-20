unsigned char rfid_checksum(const unsigned char *buf);
int rfid_read(unsigned char *rbuf);
int rfid_write(unsigned char *wbuf);
void delay(int loop);
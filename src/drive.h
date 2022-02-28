#include <linux/cdrom.h>

int getDriveStatus(char* device);
int getDiscMcn(char* device, char* mcnbuffer, int mcnbufflen);
int getMediaChange(char* device);
int openDrive(char* device);
int closeDrive(char* device);
int lockDrive(char* device);
int unlockDrive(char* device);
int getTocData(char* device, struct cdrom_tochdr** tochdr, struct cdrom_tocentry** tocents);

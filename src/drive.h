#include <linux/cdrom.h>

int getDriveStatus(char* device, int *discStatus);
void getDiscMcn(char* device, char* mcnbuffer, int mcnbufflen);
int getMediaChange(char* device);
void openDrive(char* device);
void closeDrive(char* device);
void lockDrive(char* device);
void unlockDrive(char* device);
void getTocData(char* device, struct cdrom_tochdr** tochdr, struct cdrom_tocentry** tocents);

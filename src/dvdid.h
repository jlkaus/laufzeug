//#include <dvdread/dvd_reader.h>
//#include <dvdread/ifo_types.h>
//#include <dvdread/ifo_read.h>

typedef struct {
  int titleNum;
  off_t ifoSize;
  off_t bupSize;
  off_t menuVobSize;
  off_t titleVobSize;
  int titleVobParts;
} DvdToc;

int getDvdDiscId(char* device, char* idbuffer);   // buffer has to be 16 bytes or more
int getDvdVolumeId(char* device, char* idbuffer); // buffer has to be 32 bytes or more
int getDvdSetId(char* device, char* idbuffer);    // buffer has to be 129 bytes or more
int getDvdToc(char* device, DvdToc** titleTocs, int* numTitles);  // *titletocs needs to be free'd by the client!
int getDvdRegion(char* device, uint8_t* regionByte);
void displayDvdToc(DvdToc* titleTocs, int numTitles);
void displayDvdDiscId(char* idbuffer);
void displayDvdVolumeId(char* idbuffer);
void displayDvdSetId(char* idbuffer);

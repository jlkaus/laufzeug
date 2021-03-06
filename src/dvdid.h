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

void getDvdDiscId(char* device, char* idbuffer);   // buffer has to be 16 bytes or more
void getDvdVolumeId(char* device, char* idbuffer); // buffer has to be 32 bytes or more
void getDvdSetId(char* device, char* idbuffer);    // buffer has to be 129 bytes or more
void getDvdToc(char* device, DvdToc** titleTocs, int* numTitles);  // *titletocs needs to be free'd by the client!
void getDvdRegion(char* device, uint8_t* regionByte);
void displayDvdToc(DvdToc* titleTocs, int numTitles);
void displayDvdDiscId(char* idbuffer);
void displayDvdVolumeId(char* idbuffer);
void displayDvdSetId(char* idbuffer);

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <errno.h>
#include <string.h>
#include <error.h>
#include <sysexits.h>
#include <assert.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

#include "dvdid.h"

void getDvdDiscId(char* device, char* idbuffer)
{
  dvd_reader_t* dvd = DVDOpen(device);
  if(dvd) {
    if(DVDDiscID(dvd, (uint8_t *)idbuffer) == -1) {
      error(EX_UNAVAILABLE, errno, "Unable to compute id for DVD in %s", device);
    }
    DVDClose(dvd);
  } else {
    error(EX_NOINPUT, errno, "Unable to open DVD %s", device);
  }
}

void getDvdVolumeId(char* device, char* idbuffer)
{
  dvd_reader_t* dvd = DVDOpen(device);

  for(size_t i=0; i < 32; ++i) {
    idbuffer[i]=0;
  }

  if(dvd) {
    uint8_t otheridbuffer[129];
    if(DVDUDFVolumeInfo(dvd, idbuffer, 32, otheridbuffer, 128) == -1) {
      if(DVDISOVolumeInfo(dvd, idbuffer, 32, otheridbuffer, 128) == -1) {
        error(EX_IOERR, errno, "Unable to read volume id from DVD in %s", device);
      }
    }
    DVDClose(dvd);
  } else {
    error(EX_NOINPUT, errno, "Unable to open DVD %s", device);
  }
}

void getDvdSetId(char* device, char* idbuffer)
{
  dvd_reader_t* dvd = DVDOpen(device);

  for(size_t i=0; i < 129; ++i) {
    idbuffer[i]=0;
  }

  if(dvd) {
    char otheridbuffer[32];
    if(DVDUDFVolumeInfo(dvd, otheridbuffer, 32, (uint8_t *)idbuffer, 128) == -1) {
      if(DVDISOVolumeInfo(dvd, otheridbuffer, 32, (uint8_t *)idbuffer, 128) == -1) {
        error(EX_IOERR, errno, "Unable to read set id from DVD in %s", device);
      }
    }
    DVDClose(dvd);
  } else {
    error(EX_NOINPUT, errno, "Unable to open DVD %s", device);
  }
}

void getDvdToc(char* device, DvdToc** titleTocs, int* numTitles)
{
  *titleTocs = NULL;
  *numTitles = 0;

  dvd_reader_t* dvd = DVDOpen(device);
  if(dvd) {
    dvd_stat_t tStat;

    for(size_t i=0; i < 100; ++i) {
      memset(&tStat,0,sizeof(tStat));
      if(DVDFileStat(dvd, i, DVD_READ_INFO_FILE, &tStat) == -1) {
        *numTitles = i;
        break;
      }
    }

    *titleTocs = malloc(sizeof(DvdToc)* (*numTitles));
    assert(*titleTocs != NULL);

    for(size_t i=0; i < *numTitles; ++i) {
      (*titleTocs)[i].titleNum = i;
      memset(&tStat,0,sizeof(tStat));
      if(DVDFileStat(dvd, i, DVD_READ_INFO_FILE, &tStat) == 0) {
        (*titleTocs)[i].ifoSize = tStat.size/DVD_VIDEO_LB_LEN;
      } else {
        (*titleTocs)[i].ifoSize = 0;
      }

      memset(&tStat,0,sizeof(tStat));
      if(DVDFileStat(dvd, i, DVD_READ_INFO_BACKUP_FILE, &tStat) == 0) {
        (*titleTocs)[i].bupSize = tStat.size/DVD_VIDEO_LB_LEN;
      } else {
        (*titleTocs)[i].bupSize = 0;
      }
      memset(&tStat,0,sizeof(tStat));
      if(DVDFileStat(dvd, i, DVD_READ_MENU_VOBS, &tStat) == 0) {
        (*titleTocs)[i].menuVobSize = tStat.size/DVD_VIDEO_LB_LEN;
      } else {
        (*titleTocs)[i].menuVobSize = 0;
      }
      memset(&tStat,0,sizeof(tStat));
      if(DVDFileStat(dvd, i, DVD_READ_TITLE_VOBS, &tStat) == 0) {
        (*titleTocs)[i].titleVobSize = tStat.size/DVD_VIDEO_LB_LEN;
        (*titleTocs)[i].titleVobParts = tStat.nr_parts;
      } else {
        (*titleTocs)[i].titleVobSize = 0;
        (*titleTocs)[i].titleVobParts = 0;
      }
    }

    DVDClose(dvd);
  } else {
    error(EX_NOINPUT, errno, "Unable to open DVD %s", device);
  }
}

void getDvdRegion(char* device, uint8_t* regionByte)
{
  dvd_reader_t* dvd = DVDOpen(device);
  if(dvd) {
    ifo_handle_t* vmg = ifoOpenVMGI(dvd);

    if(vmg) {
      //      printf("%08x\n",vmg->vmgi_mat->vmg_category);
      *regionByte = vmg->vmgi_mat->vmg_category >> 16;

      ifoClose(vmg);
    } else {
      error(EX_IOERR, errno, "Unable to read region id from DVD in %s", device);
    }

    DVDClose(dvd);
  } else {
    error(EX_NOINPUT, errno, "Unable to open DVD %s", device);
  }
}

void displayDvdToc(DvdToc* titleTocs, int numTitles)
{
  printf("DR-TOC:     %d", numTitles);
  for(size_t i=0; i < numTitles; ++i) {
    printf(" %lu %lu %lu %d %lu", titleTocs[i].ifoSize, titleTocs[i].bupSize, titleTocs[i].menuVobSize, titleTocs[i].titleVobParts, titleTocs[i].titleVobSize);
  }
  printf("\n");
}

void displayDvdDiscId(char* idbuffer)
{
  // this id is 16 bytes long.  Display in hex!
  printf("DR-DISCID:  ");
  for(size_t i=0;i<16;++i) {
    printf("%02hhx",idbuffer[i]);
  }
  printf("\n");
}

void displayDvdVolumeId(char* idbuffer)
{
  printf("DR-VOLID:   %s", idbuffer);
  // this id is 32 bytes long, in utf-8, with null termination
  printf("\n");
}

void displayDvdSetId(char* idbuffer)
{
  printf("DR-SETID:   ");
  for(size_t i=0;i<128;++i) {
    printf("%02hhx",idbuffer[i]);
  }

//  printf("\nDR-SETIDS:  ");
//  for(size_t i=0;i<128;++i) {
//    printf("%c",idbuffer[i]>' '?idbuffer[i]:' ');
//  }

  // this id is 129 bytes long, in utf-8? with null termination
  printf("\n");
}

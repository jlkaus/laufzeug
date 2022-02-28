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

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

#include "dvdid.h"

int getDvdDiscId(char* device, char* idbuffer)
{
  int retCode = EX_OK;
  dvd_reader_t* dvd = DVDOpen(device);

  if(dvd) {
    if(DVDDiscID(dvd, (uint8_t *)idbuffer) == -1) {
      retCode = -ENOSYS;
    }
    DVDClose(dvd);
  } else {
    retCode = -EINVAL;
  }

  return retCode;
}

int getDvdVolumeId(char* device, char* idbuffer)
{
  int retCode = EX_OK;
  dvd_reader_t* dvd = DVDOpen(device);
  int i=0;

  for(i=0; i < 32; ++i) {
    idbuffer[i]=0;
  }

  if(dvd) {
    uint8_t otheridbuffer[129];
    if(DVDUDFVolumeInfo(dvd, idbuffer, 32, otheridbuffer, 128) == -1) {
      if(DVDISOVolumeInfo(dvd, idbuffer, 32, otheridbuffer, 128) == -1) {
        retCode = -ENOSYS;
      }
    }
    DVDClose(dvd);
  } else {
    retCode = -EINVAL;
  }

  return retCode;
}

int getDvdSetId(char* device, char* idbuffer)
{
  int retCode = EX_OK;
  dvd_reader_t* dvd = DVDOpen(device);
  int i=0;

  for(i=0; i < 129; ++i) {
    idbuffer[i]=0;
  }

  if(dvd) {
    char otheridbuffer[32];
    if(DVDUDFVolumeInfo(dvd, otheridbuffer, 32, (uint8_t *)idbuffer, 128) == -1) {
      if(DVDISOVolumeInfo(dvd, otheridbuffer, 32, (uint8_t *)idbuffer, 128) == -1) {
        retCode = -ENOSYS;
      }
    }
    DVDClose(dvd);
  } else {
    retCode = -EINVAL;
  }

  return retCode;
}

int getDvdToc(char* device, DvdToc** titleTocs, int* numTitles)
{
  int retCode = EX_OK;
  dvd_reader_t* dvd = DVDOpen(device);
  *titleTocs = NULL;
  *numTitles = 0;

  if(dvd) {
    dvd_stat_t tStat;
    int i = 0;

    for(i=0; i < 100; ++i) {
      memset(&tStat,0,sizeof(tStat));
      if(DVDFileStat(dvd, i, DVD_READ_INFO_FILE, &tStat) == -1) {
        *numTitles = i;
        break;
      }
    }

    *titleTocs = malloc(sizeof(DvdToc)* (*numTitles));
    if(*titleTocs) {

      for(i=0; i < *numTitles; ++i) {

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
    } else {
      // crap.
      *numTitles=0;
      retCode = -ENOMEM;
    }

    DVDClose(dvd);
  } else {
    retCode = -EINVAL;
  }

  return retCode;
}

int getDvdRegion(char* device, uint8_t* regionByte)
{
  int retCode = EX_OK;
  dvd_reader_t* dvd = DVDOpen(device);

  if(dvd) {
    ifo_handle_t* vmg = ifoOpenVMGI(dvd);

    if(vmg) {
      //      printf("%08x\n",vmg->vmgi_mat->vmg_category);
      *regionByte = vmg->vmgi_mat->vmg_category >> 16;

      ifoClose(vmg);
    } else {
      retCode = -EIO;
    }

    DVDClose(dvd);
  } else {
    retCode = -EINVAL;
  }

  return retCode;
}

void displayDvdToc(DvdToc* titleTocs, int numTitles)
{
  int i=0;
  printf("DR TOC:     %d", numTitles);
  for(i=0; i < numTitles; ++i) {
    printf(" %lu %lu %lu %d %lu", titleTocs[i].ifoSize, titleTocs[i].bupSize, titleTocs[i].menuVobSize, titleTocs[i].titleVobParts, titleTocs[i].titleVobSize);
  }
  printf("\n");
}

void displayDvdDiscId(char* idbuffer)
{
  // this id is 16 bytes long.  Display in hex!
  int i=0;
  printf("DR DISCID:  ");
  for(i=0;i<16;++i) {
    printf("%02hhx",idbuffer[i]);
  }
  printf("\n");
}

void displayDvdVolumeId(char* idbuffer)
{
  printf("DR VOLID:   %s", idbuffer);
  // this id is 32 bytes long, in utf-8, with null termination
  printf("\n");
}

void displayDvdSetId(char* idbuffer)
{
  printf("DR SETID:   ");
  int i=0;
  for(i=0;i<128;++i) {
    printf("%02hhx",idbuffer[i]);
  }

  printf("\nDR SETIDS:  ");

  for(i=0;i<128;++i) {
    printf("%c",idbuffer[i]>' '?idbuffer[i]:' ');
  }

  // this id is 129 bytes long, in utf-8? with null termination
  printf("\n");
}

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <errno.h>
#include <string.h>

#ifndef NOLIBDISCID
#include <discid/discid.h>
#endif
#ifndef NOLIBDVDREAD
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#endif

#ifndef ESUCCESS
#define ESUCCESS 0
#endif

// args: device	command
//		LCKDRV	lock-drive
//		UNLOCK	unlock-drive
//		OPNDRV	open-drive
//		CLSDRV	close-drive
//		MDACHG	media-changed
//  0 disc not changed
//  1 disc changed	
//		GETIDS	get-mcn, musicbrainz id, freedb id, toc
//  MCN:	<mcn data>
//  MB DISCID:	<musicbrainz id>
//  MB TOC:	<lt ft lt tf tocs>
//  FD DISCID:	<freedb id>
//  FD TOC:	<lt tocs ts>
//              DVDINF  retrieve dvd md5sum id, volume identifier, volume set identifier, etc.  Maybe even title lists?
//  DR DISCID:  <libdvdread md5sum dvd disc id>
//  DR VOLID:   <libdvdread volume identifier string>
//  DR SETID:   <libdvdread volume set identifier string>
//  DR TOC:     <libdvdread determined title sizes in blocks and number of splits>
//  DR REGION:  <libdvdread determined region byte>
//		DRVSTS	drive-status, disc-status
//  0 no info			CDS_NO_INFO
//  1 no disc in the drive	CDS_NO_DISC 
//  2 tray open			CDS_TRAY_OPEN
//  3 drive not ready		CDS_DRIVE_NOT_READY
//  4 disc ok			CDS_DISC_OK
//  100 audio or mixed disc	CDS_AUDIO
//
//  status: -ESUCCESS	success
//          -EINVAL	invalid usage, EINVAL
//          -ENOSYS	unsupported ENOSYS, EDRIVE_CANT_DO_THIS
//          -EBUSY	busy
//	    -ENOMEM	out of memory
//	    -EIO	IO error




// So I would like this program to allow my scripts to
// lock the drive, eject disks, retract the tray, read the TOC
// and compute disc ids, and finally, determine the state of the
// drive, including if the disc is present, the UPC of a disc, and
// if a media change has occured.

int getDriveStatus(char* device);
int getDiscMcn(char* device, char* mcnbuffer, int mcnbufflen);
int getMediaChange(char* device);
int openDrive(char* device);
int closeDrive(char* device);
int lockDrive(char* device);
int unlockDrive(char* device);
int getTocData(char* device, struct cdrom_tochdr** tochdr, struct cdrom_tocentry** tocents);

#ifndef NOLIBDVDREAD

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
#endif

// args1 = device
// args2 = command
int main(int argc, char** argv) {
  int retCode = ESUCCESS;

  if(argc == 3) {
    if(!strncmp(argv[2],"GETIDS",6)) {
      if((retCode = getDriveStatus(argv[1])) == CDS_AUDIO) {
	//// Note: MCNs take a while to compute, and my test
	//// discs didn't have one anyway (all 0000000000000)
	//	char mcnbuff[512];
	//	if(getDiscMcn(argv[1], mcnbuff, 512) == 0) {
	//	  printf("MCN:        %s\n",mcnbuff);
	//	} else {
	//	  printf("MCN:        \n");
	//	}
	// compute mb discid
#ifndef NOLIBDISCID
	DiscId *disc = discid_new();    
	if(discid_read(disc, argv[1]) != 0 ) {
	  printf("MB_DISCID:  %s\n", discid_get_id(disc));
	  //printf("Submit via    : %s\n", discid_get_submission_url(disc));
	} else {
	  retCode = -ENOSYS;
	}
	discid_free(disc); 
#endif

	struct cdrom_tochdr* myTocHead;
	struct cdrom_tocentry* myTocEntries;
	int tempRc = getTocData(argv[1], &myTocHead, &myTocEntries);
	if(tempRc == ESUCCESS) {
	  printf("MB TOC:     %d %d %d", 
		 myTocHead->cdth_trk1, 
		 myTocHead->cdth_trk0,
		 myTocHead->cdth_trk1);
	  int i =0;
	  for(i=0; i <= myTocHead->cdth_trk1; ++i) {
	    printf(" %d",
		   (myTocEntries[i].cdte_addr.msf.minute*CD_SECS +
		    myTocEntries[i].cdte_addr.msf.second)*CD_FRAMES +
		   myTocEntries[i].cdte_addr.msf.frame);
	  }
	  printf("\n");

	  // compute the FD discid:
	  // lowest 8bits are the number of tracks
	  unsigned int fdid=myTocHead->cdth_trk1-myTocHead->cdth_trk0+1;
	  // next 16bits are the total seconds on the disc 
	  //    (without the MSF offset)
	  fdid |= (((myTocEntries[0].cdte_addr.msf.minute*CD_SECS +
				myTocEntries[0].cdte_addr.msf.second) - 
			       CD_MSF_OFFSET/CD_FRAMES) << 8);
	  // top 8 bits are the checksum of second offsets in an odd way 
	  //   (each diit)
	  unsigned int n = 0;
	  for(i = myTocHead->cdth_trk0; i<= myTocHead->cdth_trk1; ++i) {
	    int secs = myTocEntries[i].cdte_addr.msf.minute*CD_SECS +
	      myTocEntries[i].cdte_addr.msf.second;
	    while(secs > 0) {
	      n += secs%10;
	      secs /= 10;
	    }
	  }
	  
	  // why is modulo wrong? I dont get it!
	  //  MUST use modulo here instead of &
	  fdid |= (n % 0xff) << 24;

	  printf("FD DISCID:  %08x\n",fdid);
	  printf("FD TOC:     %d",
		 myTocHead->cdth_trk1);
	  for(i=myTocHead->cdth_trk0; i <= myTocHead->cdth_trk1; ++i) {
	    printf(" %d",
		   (myTocEntries[i].cdte_addr.msf.minute*CD_SECS +
		    myTocEntries[i].cdte_addr.msf.second)*CD_FRAMES +
		   myTocEntries[i].cdte_addr.msf.frame);
	  }
	  printf(" %d\n",
		 myTocEntries[0].cdte_addr.msf.minute*CD_SECS +
		 myTocEntries[0].cdte_addr.msf.second);
 
	  free(myTocHead);
	  free(myTocEntries);
	} else {
	  retCode = tempRc;
	}
      }
    } else if(!strncmp(argv[2],"DRVSTS",6)) {
      retCode = getDriveStatus(argv[1]);
    } else if(!strncmp(argv[2],"MDACHG",6)) {
      retCode = getMediaChange(argv[1]);
    } else if(!strncmp(argv[2],"OPNDRV",6)) {
      retCode = openDrive(argv[1]);
    } else if(!strncmp(argv[2],"CLSDRV",6)) {
      retCode = closeDrive(argv[1]);
    } else if(!strncmp(argv[2],"LCKDRV",6)) {
      retCode = lockDrive(argv[1]);
    } else if(!strncmp(argv[2],"UNLOCK",6)) {
      retCode = unlockDrive(argv[1]);
    }
#ifndef NOLIBDVDREAD
    else if(!strncmp(argv[2],"DVDINF",6)) {
      // if((retCode = getDriveStatus(argv[1])) == CDS_DISC_OK) {
	// display everything I have about the DVD (assuming thats what it is.  Can I assume it will look like a data cdrom? Gah! no! could call it on ISOs)

	char discidbuff[16];
	char volidbuff[32];
	char setidbuff[129];
	int numTitles=0;
	DvdToc* titleTocs=NULL;
	uint8_t regionCode;
	
	retCode = getDvdDiscId(argv[1], discidbuff);
	
	if(retCode == ESUCCESS) {
	  displayDvdDiscId(discidbuff);
	  retCode = getDvdVolumeId(argv[1], volidbuff);
	}
	
	if(retCode == ESUCCESS) {
	  displayDvdVolumeId(volidbuff);
	  retCode = getDvdSetId(argv[1], setidbuff);
	}
	
	if(retCode == ESUCCESS) {
	  displayDvdSetId(setidbuff);
	  retCode = getDvdToc(argv[1], &titleTocs, &numTitles);
	}

	if(retCode == ESUCCESS) {
	  displayDvdToc(titleTocs, numTitles);
	  free(titleTocs);
	  retCode = getDvdRegion(argv[1], &regionCode);
	}

	if(retCode == ESUCCESS) {
	  printf("DR REGION:  %02hhx\n",regionCode);
	}

      // }
    }
#endif
    else {
      // no good command
      retCode = -EINVAL;
    }    
  } else {
    // not enough args
    retCode = -EINVAL;
  }

  return retCode;
}


int getDriveStatus(char* device) {
  int retCode = ESUCCESS;
  
  int fd = open(device, O_RDONLY | O_NONBLOCK);
  
  if(fd != -1) {
    
    int tempRv = ioctl(fd, CDROM_DRIVE_STATUS, 0);
    
    if(tempRv == CDS_DISC_OK) {
      
      int disctype = ioctl(fd, CDROM_DISC_STATUS, 0);
      
      if((disctype == CDS_AUDIO) || (disctype == CDS_MIXED)) {
	retCode = CDS_AUDIO;
      } else {
	retCode = CDS_DISC_OK;
      }
    } else if(tempRv < 0) {
      retCode = -errno;
    } else {
      retCode = tempRv;
    }
    
    close(fd);

  } else {
    retCode = -EINVAL;
  }
  return retCode;
}

int getDiscMcn(char* device, char* mcnbuffer, int mcnbufflen)
{
  int retCode = ESUCCESS;
  if(mcnbufflen > 13) {
    int fd = open(device, O_RDONLY | O_NONBLOCK);
  
    if(fd != -1) {
      struct cdrom_mcn mcn;
      int tempRv = ioctl(fd, CDROM_GET_MCN, &mcn);
      if(tempRv == 0) {
	strcpy(mcnbuffer, mcn.medium_catalog_number);
      } else {
	retCode = -errno;
      }
      close(fd);
    } else {
      retCode = -EINVAL;
    }
  } else {
    retCode = -EINVAL;
  }
  return retCode;
}

int getMediaChange(char* device)
{
  int retCode = ESUCCESS;

  int fd = open(device, O_RDONLY | O_NONBLOCK);
  
  if(fd != -1) {

    retCode = ioctl(fd, CDROM_MEDIA_CHANGED, 0);
    if(retCode < 0) {
      retCode = -errno;
    }

    close(fd);
  } else {
    retCode = -EINVAL;
  }
  return retCode;
}

int openDrive(char* device)
{
  int retCode = ESUCCESS;

  int fd = open(device, O_RDONLY | O_NONBLOCK);
  
  if(fd != -1) {

    if(ioctl(fd, CDROMEJECT, 0)) {
      retCode = -errno;
    }

    close(fd);
  } else {
    retCode = -EINVAL;
  }
  return retCode;
}

int closeDrive(char* device)
{
  int retCode = ESUCCESS;

  int fd = open(device, O_RDONLY | O_NONBLOCK);
  
  if(fd != -1) {
    if(ioctl(fd, CDROMCLOSETRAY, 0)) {
      retCode = -errno;
    }

    close(fd);
  } else {
    retCode = -EINVAL;
  }
  return retCode;
}

int lockDrive(char* device)
{
  int retCode = ESUCCESS;

  int fd = open(device, O_RDONLY | O_NONBLOCK);
  
  if(fd != -1) {
    if(ioctl(fd, CDROM_LOCKDOOR, 1)) {
      retCode = -errno;
    }

    close(fd);
  } else {
    retCode = -EINVAL;
  }
  return retCode;
}

int unlockDrive(char* device)
{
  int retCode = ESUCCESS;

  int fd = open(device, O_RDONLY | O_NONBLOCK);
  
  if(fd != -1) {
    if(ioctl(fd, CDROM_LOCKDOOR, 0)) {
      retCode = -errno;
    }

    close(fd);
  } else {
    retCode = -EINVAL;
  }
  return retCode;
}

int getTocData(char* device, struct cdrom_tochdr** tochdr, struct cdrom_tocentry** tocents) {
  int retCode = ESUCCESS;

  *tochdr = malloc(sizeof(struct cdrom_tochdr));
  if(*tochdr) {
    int fd = open(device, O_RDONLY | O_NONBLOCK);
  
    if(fd != -1) {
      int tempRv = ioctl(fd, CDROMREADTOCHDR, *tochdr);
      if(tempRv == 0) {
	*tocents = malloc(sizeof(struct cdrom_tocentry)*((*tochdr)->cdth_trk1+1));
	memset(*tocents, 0, sizeof(struct cdrom_tocentry)*((*tochdr)->cdth_trk1+1));
	if(*tocents) {
	  // get leadout track into 0
	  (*tocents)[0].cdte_track = CDROM_LEADOUT;
	  (*tocents)[0].cdte_format = CDROM_MSF;
	  tempRv = ioctl(fd, CDROMREADTOCENTRY, &(*tocents)[0]);

	  int i=0;
	  for(i=(*tochdr)->cdth_trk0; 
	      (tempRv == 0) && (i <= (*tochdr)->cdth_trk1); 
	      ++i) {
	    (*tocents)[i].cdte_track = i;
	    (*tocents)[i].cdte_format = CDROM_MSF;
	    tempRv = ioctl(fd, CDROMREADTOCENTRY, &(*tocents)[i]);
	  }
	  if(tempRv != 0) {
	    free(*tocents);
	    free(*tochdr);
	    retCode = -errno;
	  }
	} else {
	  free(*tochdr);
	  retCode = -ENOMEM;
	}
      } else {
	free(*tochdr);
	retCode = -errno;
      }
      close(fd);
    } else {
      retCode = -EINVAL;
    }
  } else {
    retCode = -ENOMEM;
  }
  return retCode;
}

#ifndef NOLIBDVDREAD
int getDvdDiscId(char* device, char* idbuffer)
{
  int retCode = ESUCCESS;
  dvd_reader_t* dvd = DVDOpen(device);

  if(dvd) {
    if(DVDDiscID(dvd, idbuffer) == -1) {
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
  int retCode = ESUCCESS;
  dvd_reader_t* dvd = DVDOpen(device);
  int i=0;

  for(i=0; i < 32; ++i) {
    idbuffer[i]=0;
  }

  if(dvd) {
    char otheridbuffer[129];
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
  int retCode = ESUCCESS;
  dvd_reader_t* dvd = DVDOpen(device);
  int i=0;

  for(i=0; i < 129; ++i) {
    idbuffer[i]=0;
  }

  if(dvd) {
    char otheridbuffer[32];
    if(DVDUDFVolumeInfo(dvd, otheridbuffer, 32, idbuffer, 128) == -1) {
      if(DVDISOVolumeInfo(dvd, otheridbuffer, 32, idbuffer, 128) == -1) {
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
  int retCode = ESUCCESS;

  //Oh no! stupid libdvdnav took away my DVDFileStat call and structures!  Why!!!
  /*
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
  */
  return retCode;
}

int getDvdRegion(char* device, uint8_t* regionByte)
{
  int retCode = ESUCCESS;
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
    printf(" %llu %llu %llu %d %llu", titleTocs[i].ifoSize, titleTocs[i].bupSize, titleTocs[i].menuVobSize, titleTocs[i].titleVobParts, titleTocs[i].titleVobSize);
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

#endif

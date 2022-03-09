#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <errno.h>
#include <string.h>
#include <discid/discid.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>

#ifndef ESUCCESS
#define ESUCCESS 0
#endif

#define PROGRAM_NAME "disc-ctl"
#define PROGRAM_VERSION "1.00"
#define DEFAULT_DEVICE "/dev/cdrom"

const char * argp_program_bug_address = "Jonathan Kaus <jlkaus@gmail.com>";

static char doc[] = PROGRAM_NAME " - controls optical drives and reads CD and DVD ids.\v\
NOTES:\n\
  * --media-change and --drive-status cannot both be specified.\n\
  * The exit status when --media-change is specified is one of:\n\
      0 the media has not changed\n\
      1 the media has changed\n\
      128+ an error has occurred\n\
  * The exit status when --drive-status is specified is one of:\n\
      0 no info\n\
      1 no disc in the drive\n\
      2 tray open\n\
      3 drive not ready\n\
      4 disc ok\n\
      100 audio or mixed disc\n\
      128+ an error has occurred\n\
  * If neither --media-change or --drive-status is specified, the exit status will be 0 on success, or 128+ if an error has occurred.\n\
  * --cd-info and --dvd-info cannot both be specified.\n\
  * --cd-info and --dvd-info are unlikely to give useful information for the wrong kind of disc.\n\
  * If multiple (compatible) operations are specified, they are performed in the order that the options are listed above.\n\
";
static char args_doc[] = "";
static struct argp_option options[] = {
  {"device",'d',"DEVICE",0,"Specifies the device to control.  By default, " DEFAULT_DEVICE " is used.",0},
  {"close",'c',0,0,"Close the drive.",1},
  {"lock",'l',0,0,"Lock the drive.",2},
  {"media-change",'m',0,0,"Use the exit status to indicate whether media was changed (see below).",3},
  {"drive-status",'s',0,0,"Use the exit status to indicate the drive status (see below).",3},
  {"cd-info",'i',0,0,"Print the CD identification data.",4},
  {"dvd-info",'I',0,0,"Print the DVD identification data.",4},
  {"unlock",'u',0,0,"Unlock the drive.",5},
  {"open",'o',0,0,"Open the drive.",6},

  {"help", 'h', 0, 0, "Give this help list", -1},
  {0, '?', 0, OPTION_ALIAS},
  {"version", 'V', 0, 0, "Print program version",-1},
  {0}
};

struct arguments_t {
  int lock_flag;
  int unlock_flag;
  int open_flag;
  int close_flag;
  int media_change_flag;
  int drive_status_flag;
  char *device;
  int cd_info_flag;
  int dvd_info_flag;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments_t *arguments = state->input;

  switch(key) {
  case 'l':
    arguments->lock_flag = 1;
    break;

  case 'u':
    arguments->unlock_flag = 1;
    break;

  case 'o':
    arguments->open_flag = 1;
    break;

  case 'c':
    arguments->close_flag = 1;
    break;

  case 'm':
    if(arguments->drive_status_flag) {
      fprintf(stderr, "ERROR: --media-change and --drive-status are mutually exclusive!\n");
      argp_usage(state);
    }
    arguments->media_change_flag = 1;
    break;

  case 's':
    if(arguments->media_change_flag) {
      fprintf(stderr, "ERROR: --media-change and --drive-status are mutually exclusive!\n");
      argp_usage(state);
    }
    arguments->drive_status_flag = 1;
    break;

  case 'd':
    arguments->device = arg;
    break;

  case 'i':
    if(arguments->dvd_info_flag) {
      fprintf(stderr, "ERROR: --cd-info and --dvd-info are mutually exclusive!\n");
      argp_usage(state);
    }
    arguments->cd_info_flag = 1;
    break;

  case 'I':
    if(arguments->cd_info_flag) {
      fprintf(stderr, "ERROR: --cd-info and --dvd-info are mutually exclusive!\n");
      argp_usage(state);
    }
    arguments->dvd_info_flag = 1;
    break;

  case 'h':
  case '?':
    argp_state_help(state, stdout, ARGP_HELP_STD_HELP & ~ARGP_HELP_SHORT_USAGE);
    break;

  case 'V':
    printf(PROGRAM_NAME " v" PROGRAM_VERSION " Built: " __DATE__ "\n");
    argp_state_help(state, stdout, ARGP_HELP_EXIT_OK);
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp the_p = { options, parse_opt, args_doc, doc };


int getDriveStatus(char* device);
int getDiscMcn(char* device, char* mcnbuffer, int mcnbufflen);
int getMediaChange(char* device);
int openDrive(char* device);
int closeDrive(char* device);
int lockDrive(char* device);
int unlockDrive(char* device);
int getTocData(char* device, struct cdrom_tochdr** tochdr, struct cdrom_tocentry** tocents);
int getFreedbDiscId(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents, uint32_t* idbuffer);

int displayMbDiscId(char* device);
int displayFdDiscId(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents);
int displayMbTocInfo(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents);
int displayFdTocInfo(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents);

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

int displayDvdToc(char* device);
int displayDvdDiscId(char* device);
int displayDvdVolumeId(char* device);
int displayDvdSetId(char* device);
int displayDvdRegion(char* device);

int main(int argc, char *argv[]) {
  int rc = ESUCCESS;
  int erc = ESUCCESS;

  struct arguments_t arguments = {0};
  arguments.device = DEFAULT_DEVICE;

  argp_parse(&the_p, argc, argv, ARGP_NO_HELP, 0, &arguments);

  do {
    // close the drive
    if(arguments.close_flag) {
      rc = closeDrive(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }
    }

    // lock the drive
    if(arguments.lock_flag) {
      rc = lockDrive(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }
    }

    // check the media change state and save it off
    if(arguments.media_change_flag) {
      erc = getMediaChange(arguments.device);
      if(erc < 0) {
	break;
      }
    }

    // check the drive status and save it off
    if(arguments.drive_status_flag) {
      erc = getDriveStatus(arguments.device);
      if(erc < 0) {
	break;
      }
    }

    // gather cd info and print it out
    if(arguments.cd_info_flag) {
      rc = displayMbDiscId(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }

      struct cdrom_tochdr* tochdr = NULL;
      struct cdrom_tocentry* tocents = NULL;
      rc = getTocData(arguments.device, &tochdr, &tocents);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      } else {
	// These can't really fail.
	displayMbTocInfo(tochdr, tocents);
	displayFdDiscId(tochdr, tocents);
	displayFdTocInfo(tochdr, tocents);

	free(tocents);
	free(tochdr);
      }
    }

    // gather dvd info and print it out
    if(arguments.dvd_info_flag) {
      rc = displayDvdDiscId(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }

      rc = displayDvdToc(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }

      rc = displayDvdVolumeId(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }

      rc = displayDvdSetId(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }

      rc = displayDvdRegion(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }
    }

    // unlock the drive
    if(arguments.unlock_flag) {
      rc = unlockDrive(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }
    }

    // open the drive
    if(arguments.open_flag) {
      rc = openDrive(arguments.device);
      if(rc != ESUCCESS) {
	erc = rc;
	break;
      }
    }
  } while(0);

  return erc;
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

int getFreedbDiscId(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents, uint32_t* idbuffer)
{
  int rc = 0;

  // compute the FD discid:
  // lowest 8bits are the number of tracks
  unsigned int fdid=tochdr->cdth_trk1-tochdr->cdth_trk0+1;
  // next 16bits are the total seconds on the disc 
  //    (without the MSF offset)
  fdid |= (((tocents[0].cdte_addr.msf.minute*CD_SECS +
	     tocents[0].cdte_addr.msf.second) - 
	    CD_MSF_OFFSET/CD_FRAMES) << 8);
  // top 8 bits are the checksum of second offsets in an odd way 
  //   (each diit)
  unsigned int n = 0;
  int i = 0;
  for(i = tochdr->cdth_trk0; i<= tochdr->cdth_trk1; ++i) {
    int secs = tocents[i].cdte_addr.msf.minute*CD_SECS +
      tocents[i].cdte_addr.msf.second;
    while(secs > 0) {
      n += secs%10;
      secs /= 10;
    }
  }
	  
  // why is modulo wrong? I dont get it!
  //  MUST use modulo here instead of &
  fdid |= (n % 0xff) << 24;

  *idbuffer = fdid;

  return rc;
}

int displayMbDiscId(char* device)
{
  int rc = 0;

  DiscId *disc = discid_new();    
  if(discid_read(disc, device) != 0 ) {
    printf("Musicbrainz-Disc-Id: %s\n", discid_get_id(disc));
    //printf("Submit via    : %s\n", discid_get_submission_url(disc));
  } else {
    rc = -ENOSYS;
  }
  discid_free(disc); 

  return rc;
}

int displayFdDiscId(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents)
{
  uint32_t idbuffer = 0;
  int rc = getFreedbDiscId(tochdr, tocents, &idbuffer);

  if(rc == ESUCCESS) {
    printf("Freedb-Disc-Id: %08x\n",idbuffer);
  }

  return rc;
}

int displayMbTocInfo(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents)
{
  int rc = 0;

  printf("Musicbrainz-Toc: %d %d %d", 
	 tochdr->cdth_trk1, 
	 tochdr->cdth_trk0,
	 tochdr->cdth_trk1);
  int i =0;
  for(i=0; i <= tochdr->cdth_trk1; ++i) {
    printf(" %d",
	   (tocents[i].cdte_addr.msf.minute*CD_SECS +
	    tocents[i].cdte_addr.msf.second)*CD_FRAMES +
	   tocents[i].cdte_addr.msf.frame);
  }
  printf("\n");

  return rc;
}

int displayFdTocInfo(struct cdrom_tochdr* tochdr, struct cdrom_tocentry* tocents)
{
  int rc = 0;

  printf("Freedb-Toc: %d",
	 tochdr->cdth_trk1);
  int i = 0;
  for(i=tochdr->cdth_trk0; i <= tochdr->cdth_trk1; ++i) {
    printf(" %d",
	   (tocents[i].cdte_addr.msf.minute*CD_SECS +
	    tocents[i].cdte_addr.msf.second)*CD_FRAMES +
	   tocents[i].cdte_addr.msf.frame);
  }
  printf(" %d\n",
	 tocents[0].cdte_addr.msf.minute*CD_SECS +
	 tocents[0].cdte_addr.msf.second);
  
  return rc;
}

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

int displayDvdToc(char* device)
{
  DvdToc* titleTocs = NULL;
  int numTitles = 0;
  int rc = getDvdToc(device, &titleTocs, &numTitles);

  if(rc == ESUCCESS) {
    int i=0;
    printf("Dvd-Toc: %d", numTitles);
    for(i=0; i < numTitles; ++i) {
      printf(" %llu %llu %llu %d %llu", titleTocs[i].ifoSize, titleTocs[i].bupSize, titleTocs[i].menuVobSize, titleTocs[i].titleVobParts, titleTocs[i].titleVobSize);
    }
    printf("\n");

    free(titleTocs);
  }

  return rc;
}

int displayDvdDiscId(char* device)
{
  char idbuffer[16] = {0};
  int rc = getDvdDiscId(device, idbuffer);

  if(rc == ESUCCESS) {
    // this id is 16 bytes long.  Display in hex!
    int i=0;
    printf("Dvd-Disc-Id: ");
    for(i=0;i<16;++i) {
      printf("%02hhx",idbuffer[i]);
    }
    printf("\n");
  }

  return rc;
}

int displayDvdVolumeId(char* device)
{
  char idbuffer[32] = {0};
  int rc = getDvdVolumeId(device, idbuffer);

  if(rc == ESUCCESS) {
    printf("Dvd-Volume-Id: %s", idbuffer);
    // this id is 32 bytes long, in utf-8, with null termination
    printf("\n");
  }

  return rc;
}

int displayDvdSetId(char* device)
{
  char idbuffer[129] = {0};
  int rc = getDvdSetId(device, idbuffer);

  if(rc == ESUCCESS) {
    printf("Dvd-Set-Id: ");
    int i=0;
    for(i=0;i<128;++i) {
      printf("%02hhx",idbuffer[i]);
    }

    printf("\nDvd-Set-Id-String: ");

    for(i=0;i<128;++i) {
      printf("%c",idbuffer[i]>' '?idbuffer[i]:' ');
    }

    // this id is 129 bytes long, in utf-8? with null termination
    printf("\n");
  }

  return rc;
}

int displayDvdRegion(char* device)
{
  uint8_t regionCode = 0;
  int rc = getDvdRegion(device, &regionCode);

  if(rc == ESUCCESS) {
    printf("Dvd-Region: %02hhx\n",regionCode);
  }

  return rc;
}

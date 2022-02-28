#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <argp.h>
#include <errno.h>
#include <string.h>
#include <error.h>
#include <sysexits.h>

#include "drive.h"
#include "fdid.h"

#ifndef NOLIBDISCID
#include "mbid.h"
#endif

#ifndef NOLIBDVDREAD
#include "dvdid.h"
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
//  status: -EX_OK	success
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

// args1 = device
// args2 = command
int main(int argc, char** argv) {
  int retCode = EX_OK;

  if(argc == 3) {
    if(!strncmp(argv[2],"GETIDS",6)) {
      if((retCode = getDriveStatus(argv[1])) == CDS_AUDIO) {
        //// Note: MCNs take a while to compute, and my test
        //// discs didn't have one anyway (all 0000000000000)
        //  char mcnbuff[512];
        //  if(getDiscMcn(argv[1], mcnbuff, 512) == 0) {
        //    printf("MCN:        %s\n",mcnbuff);
        //  } else {
        //    printf("MCN:        \n");
        //  }
        struct cdrom_tochdr* myTocHead;
        struct cdrom_tocentry* myTocEntries;
        retCode = getTocData(argv[1], &myTocHead, &myTocEntries);
        if(retCode == EX_OK) {

#ifndef NOLIBDISCID
          // compute mb discid
          retCode = displayMbDiscId(argv[1]);
          displayMbToc(myTocHead, myTocEntries);
#endif

          displayFdDiscId(myTocHead, myTocEntries);
          displayFdToc(myTocHead, myTocEntries);

          free(myTocHead);
          free(myTocEntries);
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

      if(retCode == EX_OK) {
        displayDvdDiscId(discidbuff);
        retCode = getDvdVolumeId(argv[1], volidbuff);
      }

      if(retCode == EX_OK) {
        displayDvdVolumeId(volidbuff);
        retCode = getDvdSetId(argv[1], setidbuff);
      }

      if(retCode == EX_OK) {
        displayDvdSetId(setidbuff);
        retCode = getDvdToc(argv[1], &titleTocs, &numTitles);
      }

      if(retCode == EX_OK) {
        displayDvdToc(titleTocs, numTitles);
        free(titleTocs);
        retCode = getDvdRegion(argv[1], &regionCode);
      }

      if(retCode == EX_OK) {
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



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

#include "drive.h"

int getDriveStatus(char* device, int *discStatus) {
  int retCode = CDS_NO_INFO;
  if(discStatus) {
    *discStatus = CDS_NO_INFO;
  }
  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    retCode = ioctl(fd, CDROM_DRIVE_STATUS, 0);
    if(retCode == CDS_DISC_OK && discStatus) {
      *discStatus = ioctl(fd, CDROM_DISC_STATUS, 0);
    } else if(retCode < 0) {
      // ENOSYS, ENOMEM
      error(EX_UNAVAILABLE, errno, "Unable to get drive status from %s", device);
    }

    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
  return retCode;
}

void getDiscMcn(char* device, char* mcnbuffer, int mcnbufflen)
{
  assert(mcnbufflen > 13);

  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    struct cdrom_mcn mcn;
    int tempRv = ioctl(fd, CDROM_GET_MCN, &mcn);
    if(tempRv == 0) {
      strcpy(mcnbuffer, (const char *)mcn.medium_catalog_number);
    } else {
      // ENOSYS
      error(EX_UNAVAILABLE, errno, "Unable to get MCN from %s", device);
    }
    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
}

int getMediaChange(char* device)
{
  int retCode = 0;
  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    retCode = ioctl(fd, CDROM_MEDIA_CHANGED, 0);
    if(retCode < 0) {
      // ENOSYS
      error(EX_UNAVAILABLE, errno, "Unable to get media change state from %s", device);
    }

    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
  return retCode;
}

void openDrive(char* device)
{
  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    if(ioctl(fd, CDROMEJECT, 0)) {
      // ENOSYS, EBUSY
      error(EX_UNAVAILABLE, errno, "Unable to eject %s", device);
    }

    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
}

void closeDrive(char* device)
{
  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    if(ioctl(fd, CDROMCLOSETRAY, 0)) {
      // ENOSYS, EBUSY
      error(EX_UNAVAILABLE, errno, "Unable to close tray for %s", device);
    }

    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
}

void lockDrive(char* device)
{
  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    if(ioctl(fd, CDROM_LOCKDOOR, 1)) {
      // ENOSYS, EBUSY
      error(EX_UNAVAILABLE, errno, "Unable to lock %s", device);
    }

    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
}

void unlockDrive(char* device)
{
  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    if(ioctl(fd, CDROM_LOCKDOOR, 0)) {
      // ENOSYS, EBUSY
      error(EX_UNAVAILABLE, errno, "Unable to unlock %s", device);
    }

    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
}

void getTocData(char* device, struct cdrom_tochdr** tochdr, struct cdrom_tocentry** tocents) {
  *tochdr = malloc(sizeof(struct cdrom_tochdr));
  assert(*tochdr != NULL);

  int fd = open(device, O_RDONLY | O_NONBLOCK);
  if(fd != -1) {
    if(ioctl(fd, CDROMREADTOCHDR, *tochdr) == 0) {
      *tocents = malloc(sizeof(struct cdrom_tocentry)*((*tochdr)->cdth_trk1+1));
      assert(*tocents != NULL);
      memset(*tocents, 0, sizeof(struct cdrom_tocentry)*((*tochdr)->cdth_trk1+1));
      // get leadout track into 0
      (*tocents)[0].cdte_track = CDROM_LEADOUT;
      (*tocents)[0].cdte_format = CDROM_MSF;
      if(ioctl(fd, CDROMREADTOCENTRY, &(*tocents)[0]) != 0) {
        error(EX_IOERR, errno, "Unable to read TOC entry (LEADOUT) from CD in %s", device);
      }

      for(int i=(*tochdr)->cdth_trk0;
          i <= (*tochdr)->cdth_trk1;
          ++i) {
        (*tocents)[i].cdte_track = i;
        (*tocents)[i].cdte_format = CDROM_MSF;
        if(ioctl(fd, CDROMREADTOCENTRY, &(*tocents)[i]) != 0) {
          error(EX_IOERR, errno, "Unable to read TOC entry %d from CD in %s", i, device);
        }
      }
    } else {
      error(EX_UNAVAILABLE, errno, "Unable to read TOC header from CD in %s", device);
    }

    close(fd);
  } else {
    // EACCES, ENOENT
    error(EX_NOINPUT, errno, "Unable to open device file %s", device);
  }
}

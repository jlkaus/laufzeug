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

#include "drive.h"

int getDriveStatus(char* device) {
  int retCode = EX_OK;
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
  int retCode = EX_OK;
  if(mcnbufflen > 13) {
    int fd = open(device, O_RDONLY | O_NONBLOCK);

    if(fd != -1) {
      struct cdrom_mcn mcn;
      int tempRv = ioctl(fd, CDROM_GET_MCN, &mcn);
      if(tempRv == 0) {
        strcpy(mcnbuffer, (const char *)mcn.medium_catalog_number);
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
  int retCode = EX_OK;
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
  int retCode = EX_OK;
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
  int retCode = EX_OK;
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
  int retCode = EX_OK;
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
  int retCode = EX_OK;
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
  int retCode = EX_OK;

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

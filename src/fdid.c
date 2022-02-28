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

#include "fdid.h"

void displayFdDiscId(struct cdrom_tochdr *hdr, struct cdrom_tocentry *entries) {
  // compute the FD discid:
  // lowest 8bits are the number of tracks
  unsigned int fdid=hdr->cdth_trk1-hdr->cdth_trk0+1;
  // next 16bits are the total seconds on the disc
  //    (without the MSF offset)
  fdid |= (((entries[0].cdte_addr.msf.minute*CD_SECS +
             entries[0].cdte_addr.msf.second) -
            CD_MSF_OFFSET/CD_FRAMES) << 8);
  // top 8 bits are the checksum of second offsets in an odd way
  //   (each diit)
  unsigned int n = 0;
  for(size_t i = hdr->cdth_trk0; i<= hdr->cdth_trk1; ++i) {
    int secs = entries[i].cdte_addr.msf.minute*CD_SECS +
      entries[i].cdte_addr.msf.second;
    while(secs > 0) {
      n += secs%10;
      secs /= 10;
    }
  }

  // why is modulo wrong? I dont get it!
  //  MUST use modulo here instead of &
  fdid |= (n % 0xff) << 24;

  printf("FD DISCID:  %08x\n",fdid);
}

void displayFdToc(struct cdrom_tochdr *hdr, struct cdrom_tocentry *entries) {
  printf("FD TOC:     %d",
         hdr->cdth_trk1);
  for(size_t i=hdr->cdth_trk0; i <= hdr->cdth_trk1; ++i) {
    printf(" %d",
           (entries[i].cdte_addr.msf.minute*CD_SECS +
            entries[i].cdte_addr.msf.second)*CD_FRAMES +
           entries[i].cdte_addr.msf.frame);
  }
  printf(" %d\n",
         entries[0].cdte_addr.msf.minute*CD_SECS +
         entries[0].cdte_addr.msf.second);
}

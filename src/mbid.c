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

#include <discid/discid.h>

#include "mbid.h"

void displayMbToc(struct cdrom_tochdr* hdr, struct cdrom_tocentry* entries) {
  printf("MB-TOC:     %d %d %d",
         hdr->cdth_trk1,
         hdr->cdth_trk0,
         hdr->cdth_trk1);
  for(size_t i=0; i <= hdr->cdth_trk1; ++i) {
    printf(" %d",
           (entries[i].cdte_addr.msf.minute*CD_SECS +
            entries[i].cdte_addr.msf.second)*CD_FRAMES +
           entries[i].cdte_addr.msf.frame);
  }
  printf("\n");
}

void displayMbDiscId(char *device) {
  DiscId *disc = discid_new();
  if(discid_read(disc, device) != 0 ) {
    printf("MB-DISCID:  %s\n", discid_get_id(disc));
    //printf("Submit via    : %s\n", discid_get_submission_url(disc));
  } else {
    error(EX_IOERR, errno, "Unable to compute MB discid for CD in %s", device);
  }
  discid_free(disc);
}


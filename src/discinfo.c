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

#define xstr(s) str(s)
#define str(s) #s

const char *argp_program_version = xstr(EXECNAME) " " xstr(VERSION);
const char *argp_program_bug_address = xstr(PKGMAINT);
static char program_documentation[] = xstr(EXECNAME) " - A tool for something.\
\vMore stuff here.\n";

static char args_doc[] = "";

static struct argp_option program_options[] = {
  {"device",            'd',"DEVICE",   0,"Drive device. Defaults to /dev/cdrom",1},
  {"close",             'c',0,          0,"Close the drive (close tray, if supported).",2},
  {"open",              'o',0,          0,"Open the drive (eject).",2},
  {"eject",             'e',0,          OPTION_ALIAS, 0,2},
  {"lock",              'l',0,          0,"Lock drive.",2},
  {"unlock",            'u',0,          0,"Unlock drive.",2},
  {"mediachange",       'm',0,          0,"Check for media change.",3},
  {"status",            's',0,          0,"Get drive status.",3},
  {"get",               'g',"ITEMS",    0,"Get disc information, as specified by ITEMS.",3},
  {"verbose",           'v',0,          0,"Verbose mode.",-1},
  {0,                   'h',0,          OPTION_HIDDEN, 0,-1},
  {0}
};

#define GET_MCN 1
#define GET_FDID 2
#define GET_FDTOC 4
#define GET_FDINFO (GET_FDID | GET_FDTOC)
#ifndef NOLIBDISCID
#define GET_MBID 8
#define GET_MBTOC 16
#define GET_MBINFO (GET_MBID | GET_MBTOC)
#define GET_CDINFO (GET_MCN | GET_FDINFO | GET_MBINFO)
#else
#define GET_CDINFO (GET_MCN | GET_FDINFO)
#endif
#ifndef NOLIBDVDREAD
#define GET_DRID 32
#define GET_DRTOC 64
#define GET_DRVOL 128
#define GET_DRSET 256
#define GET_DRREG 512
#define GET_DVDINFO (GET_DRID | GET_DRTOC | GET_DRVOL | GET_DRSET | GET_DRREG)
#endif
#define GET_ALL (GET_CDINFO | GET_DVDINFO)

#ifndef NOLIBDISCID
#ifndef NOLIBDVDREAD
#define DEFAULT_GETS (GET_FDID | GET_MBID | GET_DRID)
#else
#define DEFAULT_GETS (GET_FDID | GET_MBID)
#endif
#else
#ifndef NOLIBDVDREAD
#define DEFAULT_GETS (GET_FDID | GET_DRID)
#else
#define DEFAULT_GETS (GET_FDID)
#endif
#endif

struct program_arguments {
  int verbose;
  char *device;
  int do_close;
  int do_lock;
  int do_mediachange;
  int do_status;
  int get_mask;
  int do_unlock;
  int do_open;
  int any_op;
};

int parseGetItems(char *arg, struct argp_state *state) {
  char *curstate = NULL;
  int item_set = 0;
  char *cur_pos = strtok_r(arg, ":,; ", &curstate);
  while(cur_pos) {
    if(strcasecmp(cur_pos, "mcn") == 0) {
      item_set |= GET_MCN;
    } else if(strcasecmp(cur_pos, "fdid") == 0) {
      item_set |= GET_FDID;
    } else if(strcasecmp(cur_pos, "fdtoc") == 0) {
      item_set |= GET_FDTOC;
    } else if(strcasecmp(cur_pos, "fd") == 0) {
      item_set |= GET_FDINFO;
#ifndef NOLIBDISCID
    } else if(strcasecmp(cur_pos, "mbid") == 0) {
      item_set |= GET_MBID;
    } else if(strcasecmp(cur_pos, "mbtoc") == 0) {
      item_set |= GET_MBTOC;
    } else if(strcasecmp(cur_pos, "mb") == 0) {
      item_set |= GET_MBINFO;
#endif
    } else if(strcasecmp(cur_pos, "cd") == 0) {
      item_set |= GET_CDINFO;
#ifndef NOLIBDVDREAD
    } else if(strcasecmp(cur_pos, "drid") == 0) {
      item_set |= GET_DRID;
    } else if(strcasecmp(cur_pos, "drtoc") == 0) {
      item_set |= GET_DRTOC;
    } else if(strcasecmp(cur_pos, "drvol") == 0) {
      item_set |= GET_DRVOL;
    } else if(strcasecmp(cur_pos, "drset") == 0) {
      item_set |= GET_DRSET;
    } else if(strcasecmp(cur_pos, "drreg") == 0) {
      item_set |= GET_DRREG;
    } else if(strcasecmp(cur_pos, "dr") == 0 ||
              strcasecmp(cur_pos, "dvd") == 0) {
      item_set |= GET_DVDINFO;
#endif
    } else if(strcasecmp(cur_pos, "id") == 0 ||
              strcasecmp(cur_pos, "ids") == 0) {
      item_set |= GET_FDID;
#ifndef NOLIBDISCID
      item_set |= GET_MBID;
#endif
#ifndef NOLIBDVDREAD
      item_set |= GET_DRID;
#endif
    } else if(strcasecmp(cur_pos, "all") == 0) {
      item_set |= GET_ALL;
    } else {
      argp_failure(state, EX_USAGE, 0, "Invalid --get ITEM: %s", cur_pos);
    }

    cur_pos = strtok_r(NULL, ":,; ", &curstate);
  }
  return item_set;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct program_arguments *args = state->input;

  switch(key) {
  case '?':
  case 'h':
    argp_state_help(state, stderr, ARGP_HELP_STD_HELP);
    break;
  case 'v':
    args->verbose = 1;
    break;
  case 'd':
    args->device = arg;
    break;
  case 'e':
  case 'o':
    args->do_open = 1;
    args->any_op = 1;
    break;
  case 'c':
    args->do_close = 1;
    args->any_op = 1;
    break;
  case 'l':
    args->do_lock = 1;
    args->any_op = 1;
    break;
  case 'u':
    args->do_unlock = 1;
    args->any_op = 1;
    break;
  case 's':
    args->do_status = 1;
    args->any_op = 1;
    break;
  case 'm':
    args->do_mediachange = 1;
    args->any_op = 1;
    break;
  case 'g':
    args->get_mask |= parseGetItems(arg, state);
    args->any_op = 1;
    break;
  case ARGP_KEY_ARG:
    argp_failure(state, EX_USAGE, 0, "No non-option arguments supported.");
    break;
  case ARGP_KEY_END:
    break;
  case ARGP_KEY_SUCCESS:
    if(!args->any_op) {
      args->get_mask = DEFAULT_GETS;
    }
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp program_arg_parser = {program_options, parse_opt, args_doc, program_documentation};


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
int main(int argc, char* argv[]) {
  program_invocation_name = xstr(EXECNAME);

  // interpret arguments
  struct program_arguments args = {0, "/dev/cdrom", 0, 0, 0, 0, 0, 0, 0, 0};
  argp_parse(&program_arg_parser, argc, argv, 0, 0, &args);

  // Do the work
  if(args.do_close) {
    closeDrive(args.device);
  }

  if(args.do_lock) {
    lockDrive(args.device);
  }

  if(args.do_mediachange) {
    getMediaChange(args.device);
  }

  if(args.do_status) {
    getDriveStatus(args.device);
  }

  if(args.get_mask) {
    if(args.get_mask & GET_CDINFO) {
      if(getDriveStatus(args.device) == CDS_AUDIO) {
        if(args.get_mask & GET_MCN) {
          char mcnbuff[512];
          if(getDiscMcn(args.device, mcnbuff, 512) == 0) {
            printf("MCN:        %s\n",mcnbuff);
          } else {
            printf("MCN:        \n");
          }
        }
        if(args.get_mask & ~GET_MCN & GET_ALL) {
          struct cdrom_tochdr* myTocHead;
          struct cdrom_tocentry* myTocEntries;
          getTocData(args.device, &myTocHead, &myTocEntries);

          if(args.get_mask & GET_FDID) {

          }
          if(args.get_mask & GET_FDTOC) {

          }
          if(args.get_mask & GET_MBID) {

          }
          if(args.get_mask & GET_MBTOC) {

          }

          free(myTocHead);
          free(myTocEntries);
        }
      }
    }
#ifndef NOLIBDVDREAD
    if(args.get_mask & GET_DVDINFO) {
      // if(getDriveStatus(args.device) == CDS_DISC_OK) {

      // display everything I have about the DVD (assuming thats what it is.  Can I assume it will look like a data cdrom? Gah! no! could call it on ISOs)

      char discidbuff[16];
      char volidbuff[32];
      char setidbuff[129];
      int numTitles=0;
      DvdToc* titleTocs=NULL;
      uint8_t regionCode;

      retCode = getDvdDiscId(args.device, discidbuff);

      if(retCode == EX_OK) {
        displayDvdDiscId(discidbuff);
        retCode = getDvdVolumeId(args.device, volidbuff);
      }

      if(retCode == EX_OK) {
        displayDvdVolumeId(volidbuff);
        retCode = getDvdSetId(args.device, setidbuff);
      }

      if(retCode == EX_OK) {
        displayDvdSetId(setidbuff);
        retCode = getDvdToc(args.device, &titleTocs, &numTitles);
      }

      if(retCode == EX_OK) {
        displayDvdToc(titleTocs, numTitles);
        free(titleTocs);
        retCode = getDvdRegion(args.device, &regionCode);
      }

      if(retCode == EX_OK) {
        printf("DR REGION:  %02hhx\n",regionCode);
      }


      // }
    }
#endif
  }

  if(args.do_unlock) {
    unlockDrive(args.device);
  }

  if(args.do_open) {
    openDrive(args.device);
  }

  return EX_OK;
}



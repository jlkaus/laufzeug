#include <linux/cdrom.h>

void displayMbToc(struct cdrom_tochdr* hdr, struct cdrom_tocentry* entries);
void displayMbDiscId(char *device);

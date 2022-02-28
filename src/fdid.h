#include <linux/cdrom.h>

void displayFdDiscId(struct cdrom_tochdr *hdr, struct cdrom_tocentry *entries);
void displayFdToc(struct cdrom_tochdr *hdr, struct cdrom_tocentry *entries);

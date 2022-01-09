#include <stdio.h>

#include "webd_log.h"

bool webd_log_init (const char *fname);
void webd_log_shutdown (void);

void webd_log (const char *srcfile, int srcline, const char *fmts, ...);


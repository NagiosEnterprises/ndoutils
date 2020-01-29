
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifndef FALSE
#  define FALSE 0
#endif
#ifndef TRUE
#  define TRUE  1
#endif

extern int ndo_timing_debugging_enabled;
char ndo_wt_file_opened = FALSE;
FILE * ndo_wt_fp = NULL;

#define NDO_WT_FILE "/usr/local/nagios/var/ndo.timing"

void ndo_write_timing(char * msg)
{
    if (!ndo_timing_debugging_enabled) {
        return;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (ndo_wt_file_opened == FALSE) {
        ndo_wt_fp = fopen(NDO_WT_FILE, "a");
        ndo_wt_file_opened = TRUE;
    }

    if (ndo_wt_fp == NULL) {
        return;
    }

    fprintf(ndo_wt_fp, "%ld.%06ld %s\n", tv.tv_sec, tv.tv_usec, msg);
}

void ndo_close_timing()
{
    if (!ndo_timing_debugging_enabled) {
        return;
    }
    if (ndo_wt_fp != NULL) {
        fclose(ndo_wt_fp);
    }
}

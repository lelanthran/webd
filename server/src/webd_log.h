
#ifndef H_WEBD_LOG
#define H_WEBD_LOG

#include <stdbool.h>

#define WEBD_LOG(...)      webd_log (__FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

   // These two calls are not thread-safe.
   bool webd_log_init (const char *fname);
   void webd_log_shutdown (void);

   // These calls are all thread-safe
   void webd_log (const char *srcfile, int srcline, const char *fmts, ...);


#ifdef __cplusplus
};
#endif


#endif



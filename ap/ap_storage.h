#ifndef AP_STORAGE_H
#define AP_STORAGE_H

#include "ap_malloc.h"

#define AP_EXCEPT_STATIC_CBK
#include "ap_except.h"

/* Defines a persistent data storage that is backed up by one or more files.
	- retains persistance
	- can save or discard current changes (it commits them to the file only if explicitly told so)
	- it avoids data corruption by asking the user to commit changes, as such the data is in a known state
	- it is globaly available for a pogram
*/

/* If you use this storage, then ap_string, ap_hashmap, ap_map, ap_vector will all use this storage
to hold their data */
#define AP_ENABLE_AUTOINIT

extern ap_ctx_t *ap_static_ctx;

/* if you don't set this callback in the init function it will throw on error */
using ap_storage_cbk_t = void (*)(void *usr_ctx, const char *str, ap_except_info_t *exc_inf);

enum {
	AP_STORAGE_REVERT_CHANGES = 1,
	AP_STORAGE_COMMIT_CHANGES = 2,
};

/* this commits or discards the data modified since the last commit */
int ap_storage_do_changes(int action);

int ap_storage_init(const char *ctrl_file, ap_storage_cbk_t cbk, void *ctx);
void ap_storage_uninit();

ap_ctx_t *ap_storage_get_mctx();

#endif

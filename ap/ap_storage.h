#ifndef AP_STORAGE_H
#define AP_STORAGE_H

#include "ap_malloc.h"

/* Defines a persistent data storage that is backed up by one or more files.
	- retains persistance
	- aps_push() - saves uncommited changes to the database
	- aps_vec(), aps_hmap(), aps_bmap(), aps_dlist(), aps_list(), aps_map() ... - constructs this
	type */

int ap_storage_submit_changes(bool reverse_changes = false);

int ap_storage_init(const char *ctrl_file);
void ap_storage_uninit();

ap_ctx_t *ap_storage_get_mctx();

#endif

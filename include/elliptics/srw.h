/*
 * 2008+ Copyright (c) Evgeniy Polyakov <zbr@ioremap.net>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ELLIPTICS_SRW_H
#define __ELLIPTICS_SRW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <elliptics/core.h>
#include <elliptics/packet.h>

#define DNET_SPH_FLAGS_SRC_BLOCK	(1<<0)		/* when set data in @src is valid ID and can be used to send reply data, caller blocks */
#define DNET_SPH_FLAGS_REPLY		(1<<1)		/* this packet is a reply to the blocked request with ID stored in @src */
#define DNET_SPH_FLAGS_FINISH		(1<<2)		/* complete request with ID stored in @src, this packet will unblock client */

struct sph {
	struct dnet_raw_id	src;			/* reply has to be sent to this id */
	uint64_t		data_size;		/* size of text data in @data - located after even string */
	uint64_t		binary_size;		/* size of binary data in @data - located after text data */
	uint64_t		flags;
	int			event_size;		/* size of the event string - it is located first in @data */
	int			status;			/* processing status - negative errno code or zero on success */
	int			key;			/* meta-key - used to map header to particular worker, see pool::worker_process() */
	int			src_key;		/* blocked client generates this key and waits for it to complete */
	char			data[0];
} __attribute__ ((packed));

static inline void dnet_convert_sph(struct sph *e)
{
	e->data_size = dnet_bswap64(e->data_size);
	e->binary_size = dnet_bswap64(e->binary_size);
	e->flags = dnet_bswap64(e->flags);
	e->event_size = dnet_bswap32(e->event_size);
	e->status = dnet_bswap32(e->status);
	e->key = dnet_bswap32(e->key);
	e->src_key = dnet_bswap32(e->src_key);
}

struct srw_init_ctl {
	char			*config;
};

struct dnet_node;
int dnet_srw_update(struct dnet_node *n, int pid);

#ifdef __cplusplus
}
#endif


#endif /* __ELLIPTICS_SRW_H */

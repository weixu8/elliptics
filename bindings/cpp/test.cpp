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

#include "config.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <fstream>

#include "elliptics/cppdef.h"

static void test_log_raw(elliptics_log *l, uint32_t mask, const char *format, ...)
{
	va_list args;
	char buf[1024];
	int buflen = sizeof(buf);

	if (!(l->get_log_mask() & mask))
		return;

	va_start(args, format);
	vsnprintf(buf, buflen, format, args);
	buf[buflen-1] = '\0';
	l->log(mask, buf);
	va_end(args);
}

class elliptics_callback_io : public elliptics_callback {
	public:
		elliptics_callback_io(elliptics_log *l) { log = l; };
		virtual ~elliptics_callback_io() {};

		virtual int		callback(void);

	private:
		elliptics_log		*log;
};

int elliptics_callback_io::callback(void)
{
	int err;
	struct dnet_io_attr *io;
	unsigned long long size;
	void *data;

	if (!cmd || !state) {
		err = -EINVAL;
		goto err_out_exit;
	}

	if (cmd->status || !cmd->size) {
		err = cmd->status;
		goto err_out_exit;
	}

	if (cmd->size <= sizeof(struct dnet_attr) + sizeof(struct dnet_io_attr)) {
		test_log_raw(log, DNET_LOG_ERROR, "%s: read completion error: wrong size: "
				"cmd_size: %llu, must be more than %zu.\n",
				dnet_dump_id(&cmd->id), (unsigned long long)cmd->size,
				sizeof(struct dnet_attr) + sizeof(struct dnet_io_attr));
		err = -EINVAL;
		goto err_out_exit;
	}

	if (!attr) {
		test_log_raw(log, DNET_LOG_ERROR, "%s: no attributes but command size is not null.\n",
				dnet_dump_id(&cmd->id));
		err = -EINVAL;
		goto err_out_exit;
	}

	io = (struct dnet_io_attr *)(attr + 1);
	data = io + 1;

	dnet_convert_io_attr(io);
	err = 0;

	test_log_raw(log, DNET_LOG_INFO, "%s: io completion: offset: %llu, size: %llu.\n",
			dnet_dump_id(&cmd->id), (unsigned long long)io->offset, (unsigned long long)io->size);

err_out_exit:
	if (!cmd || !(cmd->flags & DNET_FLAGS_MORE))
		test_log_raw(log, DNET_LOG_INFO, "%s: io completed: %d.\n", cmd ? dnet_dump_id(&cmd->id) : "nil", err);
	return err;
}

int main()
{
	struct dnet_id id;
	int groups[] = {1, 2, 3};

	elliptics_log_file log("/dev/stderr", 15);

	memset(id.id, 1, DNET_ID_SIZE);
	id.group_id = 0;

	elliptics_node n(log);
	n.set_id(id);
	n.add_groups(groups, ARRAY_SIZE(groups));

	n.add_remote("devfs8", 1025, AF_INET);
#if 1
	elliptics_callback_io callback(&log);

	memset(id.id, 0xff, DNET_ID_SIZE);
	n.read_data(id, 0, 0, callback);
#endif
	n.write_file(id, const_cast<char *>("/tmp/culinaria.txt.bak"), 0, 0, 0);
	n.read_file(id, const_cast<char *>("/tmp/culinaria.txt"), 0, 0);

	n.read_file(reinterpret_cast<void *>(const_cast<char *>("1.xml")), 5,
			const_cast<char *>("/tmp/1.xml.cpp"), 0, 0);

	/* cool, yeah? we have to wait for read_data() to complete actually */
	sleep(10);
}

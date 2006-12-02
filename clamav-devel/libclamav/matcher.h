/*
 *  Copyright (C) 2002 - 2005 Tomasz Kojm <tkojm@clamav.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifndef __MATCHER_H
#define __MATCHER_H

#include <sys/types.h>

#include "clamav.h"
#include "filetypes.h"
#include "others.h"
#include "execs.h"
#include "cltypes.h"
#include "md5.h"

#define CL_TARGET_TABLE_SIZE 7

struct cli_target_info {
    off_t fsize;
    struct cli_exe_info exeinfo;
    int8_t status; /* 0 == not initialised, 1 == initialised OK, -1 == error */
};

int cli_scandesc(int desc, cli_ctx *ctx, unsigned short otfrec, unsigned short ftype, struct cli_matched_type **ftoffset);

int cli_scanbuff(const unsigned char *buffer, unsigned int length, const char **virname, const struct cl_engine *engine, unsigned short ftype);

int cli_validatesig(unsigned short ftype, const char *offstr, off_t fileoff, struct cli_target_info *info, int desc, const char *virname);

struct cli_md5_node *cli_vermd5(const unsigned char *md5, const struct cl_engine *engine);

off_t cli_caloff(const char *offstr, struct cli_target_info *info, int fd, unsigned short ftype, int *ret);

#endif

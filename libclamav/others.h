/*
 *  Copyright (C) 1999 - 2005 Tomasz Kojm <tkojm@clamav.net>
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

#ifndef __OTHERS_H_LC
#define __OTHERS_H_LC

#include <stdio.h>
#include <stdlib.h>
#include "cltypes.h"

#include "clamav.h"
#include "dconf.h"

/*
 * CLI_ISCONTAINED(buf1, size1, buf2, size2) checks if buf2 is contained
 * within buf1.
 *
 * buf1 and buf2 are pointers (or offsets) for the main buffer and the
 * sub-buffer respectively, and size1/2 are their sizes
 *
 * The macro can be used to protect against wraps.
 */
#define CLI_ISCONTAINED(bb, bb_size, sb, sb_size)	\
    (bb_size > 0 && sb_size > 0 && sb_size <= bb_size	\
     && sb >= bb && sb + sb_size <= bb + bb_size && sb + sb_size > bb)

#define CLI_ISCONTAINED2(bb, bb_size, sb, sb_size)	\
    (bb_size > 0 && sb_size >= 0 && sb_size <= bb_size	\
     && sb >= bb && sb + sb_size <= bb + bb_size && sb + sb_size >= bb)

#define CLI_MAX_ALLOCATION 184549376

/* Maximum filenames under various systems - njh */
#ifndef	NAME_MAX	/* e.g. Linux */
# ifdef	MAXNAMELEN	/* e.g. Solaris */
#   define	NAME_MAX	MAXNAMELEN
# else
#   ifdef	FILENAME_MAX	/* e.g. SCO */
#     define	NAME_MAX	FILENAME_MAX
#   else
#     define    NAME_MAX        256
#   endif
# endif
#endif

#if NAME_MAX < 256
#undef NAME_MAX
#define NAME_MAX 256
#endif

/* internal clamav context */
typedef struct {
    const char **virname;
    unsigned long int *scanned;
    const struct cli_matcher *root;
    const struct cl_engine *engine;
    const struct cl_limits *limits;
    unsigned int options;
    unsigned int arec;
    unsigned int mrec;
    struct cli_dconf *dconf;
} cli_ctx;

#define SCAN_ARCHIVE	    (ctx->options & CL_SCAN_ARCHIVE)
#define SCAN_MAIL	    (ctx->options & CL_SCAN_MAIL)
#define SCAN_OLE2	    (ctx->options & CL_SCAN_OLE2)
#define SCAN_PDF	    (ctx->options & CL_SCAN_PDF)
#define SCAN_HTML	    (ctx->options & CL_SCAN_HTML)
#define SCAN_PE		    (ctx->options & CL_SCAN_PE)
#define SCAN_ELF	    (ctx->options & CL_SCAN_ELF)
#define SCAN_ALGO 	    (ctx->options & CL_SCAN_ALGORITHMIC)
#define DETECT_ENCRYPTED    (ctx->options & CL_SCAN_BLOCKENCRYPTED)
#define BLOCKMAX	    (ctx->options & CL_SCAN_BLOCKMAX)
#define DETECT_BROKEN	    (ctx->options & CL_SCAN_BLOCKBROKEN)

#if WORDS_BIGENDIAN == 0
/* new macros from A. Melnikoff */
#define le16_to_host(v)	(v)
#define le32_to_host(v)	(v)
#define le64_to_host(v)	(v)
#define	be16_to_host(v)	((v >> 8) | ((v & 0xFF) << 8))
#define	be32_to_host(v)	((v >> 24) | ((v & 0x00FF0000) >> 8) | \
				((v & 0x0000FF00) << 8) | (v << 24))
#define be64_to_host(v)	((v >> 56) | ((v & 0x00FF000000000000LL) >> 40) | \
				((v & 0x0000FF0000000000LL) >> 24) | \
				((v & 0x000000FF00000000LL) >> 8) |  \
				((v & 0x00000000FF000000LL) << 8) |  \
				((v & 0x0000000000FF0000LL) << 24) | \
				((v & 0x000000000000FF00LL) << 40) | \
				(v << 56))
#else
#define	le16_to_host(v)	((v >> 8) | ((v & 0xFF) << 8))
#define	le32_to_host(v)	((v >> 24) | ((v & 0x00FF0000) >> 8) | \
				((v & 0x0000FF00) << 8) | (v << 24))
#define le64_to_host(v)	((v >> 56) | ((v & 0x00FF000000000000LL) >> 40) | \
				((v & 0x0000FF0000000000LL) >> 24) | \
				((v & 0x000000FF00000000LL) >> 8) |  \
				((v & 0x00000000FF000000LL) << 8) |  \
				((v & 0x0000000000FF0000LL) << 24) | \
				((v & 0x000000000000FF00LL) << 40) | \
				(v << 56))
#define be16_to_host(v)	(v)
#define be32_to_host(v)	(v)
#define be64_to_host(v)	(v)
#endif

/* used by: spin, yc (C) aCaB */
#define ROL(a,b) a = ( a << (b % (sizeof(a)<<3) ))  |  (a >> (  (sizeof(a)<<3)  -  (b % (sizeof(a)<<3 )) ) )
#define ROR(a,b) a = ( a >> (b % (sizeof(a)<<3) ))  |  (a << (  (sizeof(a)<<3)  -  (b % (sizeof(a)<<3 )) ) )

#ifndef	FALSE
#define FALSE (0)
#endif

#ifndef	TRUE
#define TRUE (1)
#endif

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

typedef struct bitset_tag
{
        unsigned char *bitset;
        unsigned long length;
} bitset_t;

#ifdef __GNUC__
void cli_warnmsg(const char *str, ...) __attribute__((format(printf, 1, 2)));
#else
void cli_warnmsg(const char *str, ...);
#endif

#ifdef __GNUC__
void cli_errmsg(const char *str, ...) __attribute__((format(printf, 1, 2)));
#else
void cli_errmsg(const char *str, ...);
#endif

#ifdef __GNUC__
void cli_dbgmsg(const char *str, ...) __attribute__((format(printf, 1, 2)));
#else
void cli_dbgmsg(const char *str, ...);
#endif

void *cli_malloc(size_t nmemb);
void *cli_calloc(size_t nmemb, size_t size);
void *cli_realloc(void *ptr, size_t size);
char *cli_strdup(const char *s);
int cli_rmdirs(const char *dirname);
unsigned char *cli_md5digest(int desc);
char *cli_md5stream(FILE *fs, unsigned char *digcpy);
char *cli_md5file(const char *filename);
int cli_readn(int fd, void *buff, unsigned int count);
int cli_writen(int fd, const void *buff, unsigned int count);
int32_t cli_readint32(const char *buff);
void cli_writeint32(char *offset, uint32_t value);
char *cli_gentemp(const char *dir);
char *cli_gentempdir(const char *dir);
char *cli_gentempdesc(const char *dir, int *fd);
char *cli_gentempstream(const char *dir, FILE **fs);
unsigned int cli_rndnum(unsigned int max);
int cli_filecopy(const char *src, const char *dest);
bitset_t *cli_bitset_init(void);
void cli_bitset_free(bitset_t *bs);
int cli_bitset_set(bitset_t *bs, unsigned long bit_offset);
int cli_bitset_test(bitset_t *bs, unsigned long bit_offset);
void	cli_sanitise_filename(char *name);
#endif

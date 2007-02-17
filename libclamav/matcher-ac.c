/*
 *  C implementation of the Aho-Corasick pattern matching algorithm. It's based
 *  on the ScannerDaemon's version (coded in Java) by Kurt Huwig and
 *  http://www-sr.informatik.uni-tuebingen.de/~buehler/AC/AC.html
 *  Thanks to Kurt Huwig for pointing me to this page.
 *
 *  Copyright (C) 2002 - 2006 Tomasz Kojm <tkojm@clamav.net>
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

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "clamav.h"
#include "others.h"
#include "matcher.h"
#include "matcher-ac.h"
#include "defaults.h"
#include "filetypes.h"
#include "cltypes.h"

struct nodelist {
    struct cli_ac_node *node;
    struct nodelist *next;
};

unsigned short ac_depth = AC_DEFAULT_DEPTH;

int cli_ac_addpatt(struct cli_matcher *root, struct cli_ac_patt *pattern)
{
	struct cli_ac_node *pos, *next;
	int i;

    if(pattern->length < ac_depth)
	return CL_EPATSHORT;

    pos = root->ac_root;

    for(i = 0; i < ac_depth; i++) {
	next = pos->trans[((unsigned char) pattern->pattern[i]) & 0xff]; 

	if(!next) {
	    next = (struct cli_ac_node *) cli_calloc(1, sizeof(struct cli_ac_node));
	    if(!next) {
		cli_errmsg("cli_ac_addpatt(): Unable to allocate AC node (%u bytes)\n", sizeof(struct cli_ac_node));
		return CL_EMEM;
	    }

	    root->ac_nodes++;
	    root->ac_nodetable = (struct cli_ac_node **) cli_realloc(root->ac_nodetable, (root->ac_nodes) * sizeof(struct cli_ac_node *));
	    if(root->ac_nodetable == NULL) {
		cli_errmsg("cli_ac_addpatt(): Unable to realloc nodetable (%u bytes)\n", (root->ac_nodes) * sizeof(struct cli_matcher *));
		return CL_EMEM;
	    }
	    root->ac_nodetable[root->ac_nodes - 1] = next;

	    pos->trans[((unsigned char) pattern->pattern[i]) & 0xff] = next;
	}

	pos = next;
    }

    pos->islast = 1;

    pattern->next = pos->list;
    pos->list = pattern;

    return CL_SUCCESS;
}

static int cli_enqueue(struct nodelist **bfs, struct cli_ac_node *n)
{
	struct nodelist *new;

    new = (struct nodelist *) cli_calloc(1, sizeof(struct nodelist));
    if (new == NULL) {
	cli_errmsg("cli_enqueue(): Unable to allocate node list (%u bytes)\n", sizeof(struct nodelist));
	return CL_EMEM;
    }

    new->next = *bfs;
    new->node = n;
    *bfs = new;
    return CL_SUCCESS;
}

static struct cli_ac_node *cli_dequeue(struct nodelist **bfs)
{
	struct nodelist *handler, *prev = NULL;
	struct cli_ac_node *pt;

    handler = *bfs;

    while(handler && handler->next) {
	prev = handler;
	handler = handler->next;
    }

    if(!handler) {
	return NULL;
    } else {
	pt = handler->node;
	free(handler);
	if(prev)
	    prev->next = NULL;
	else
	    *bfs = NULL;

	return pt;
    }
}

static int cli_maketrans(struct cli_matcher *root)
{
	struct nodelist *bfs = NULL;
	struct cli_ac_node *ac_root = root->ac_root, *child, *node;
	int i, ret;


    ac_root->fail = NULL;
    if((ret = cli_enqueue(&bfs, ac_root)) != 0) {
	return ret;
    }

    while((node = cli_dequeue(&bfs))) {
	if(node->islast)
	    continue;

	for(i = 0; i < 256; i++) {
	    child = node->trans[i];
	    if(!child) {
		if(node->fail)
		    node->trans[i] = (node->fail)->trans[i];
		else
		    node->trans[i] = ac_root;
	    } else {
		if(node->fail)
		    child->fail = (node->fail)->trans[i];
		else
		    child->fail = ac_root;

		if((ret = cli_enqueue(&bfs, child)) != 0) {
		    return ret;
		}
	    }
	}
    }
    return CL_SUCCESS;
}

int cli_ac_buildtrie(struct cli_matcher *root)
{

    if(!root)
	return CL_EMALFDB;

    if(!root->ac_root) {
	cli_dbgmsg("cli_ac_buildtrie(): AC pattern matcher is not initialised\n");
	return CL_SUCCESS;
    }

    return cli_maketrans(root);
}

static void cli_freepatt(struct cli_ac_patt *list)
{
	struct cli_ac_patt *handler, *prev;
	int i;


    handler = list;

    while(handler) {
	if(handler->prefix)
	    free(handler->prefix);
	else
	    free(handler->pattern);
	free(handler->virname);
	if(handler->offset && (!handler->sigid || handler->partno == 1))
	    free(handler->offset);
	if(handler->alt) {
	    free(handler->altn);
	    for(i = 0; i < handler->alt; i++)
		free(handler->altc[i]);
	    free(handler->altc);
	}
	prev = handler;
	handler = handler->next;
	free(prev);
    }
}

void cli_ac_free(struct cli_matcher *root)
{
	unsigned int i;


    for(i = 0; i < root->ac_nodes; i++) {
	cli_freepatt(root->ac_nodetable[i]->list);
	free(root->ac_nodetable[i]);
    }

    if(root->ac_nodetable)
	free(root->ac_nodetable);

    if(root->ac_root)
	free(root->ac_root);
}

inline static int cli_findpos(const unsigned char *buffer, unsigned int depth, unsigned int offset, unsigned int length, const struct cli_ac_patt *pattern)
{
	unsigned int bufferpos = offset + depth;
	unsigned int postfixend = offset + length;
	unsigned int i, j, alt = pattern->alt_pattern, found;


    if(pattern->prefix)
	if(pattern->prefix_length > offset)
	    return 0;

    if(bufferpos >= length)
	bufferpos %= length;

    for(i = depth; i < pattern->length; i++) {

	if(bufferpos == postfixend)
	    return 0;

	if(pattern->pattern[i] == CLI_ALT) {
	    found = 0;
	    for(j = 0; j < pattern->altn[alt]; j++) {
		if(pattern->altc[alt][j] == buffer[bufferpos]) {
		    found = 1;
		    break;
		}
	    }

	    if(!found)
		return 0;
	    alt++;

	} else if(pattern->pattern[i] != CLI_IGN && (unsigned char) pattern->pattern[i] != buffer[bufferpos])
	    return 0;

	bufferpos++;

	if(bufferpos == length)
	    bufferpos = 0;
    }

    if(pattern->prefix) {
	alt = 0;
	bufferpos = offset - pattern->prefix_length;

	for(i = 0; i < pattern->prefix_length; i++) {

	    if(pattern->prefix[i] == CLI_ALT) {
		found = 0;
		for(j = 0; j < pattern->altn[alt]; j++) {
		    if(pattern->altc[alt][j] == buffer[bufferpos]) {
			found = 1;
			break;
		    }
		}

		if(!found)
		    return 0;
		alt++;

	    } else if(pattern->prefix[i] != CLI_IGN && (unsigned char) pattern->prefix[i] != buffer[bufferpos])
		return 0;

	    bufferpos++;
	}
    }

    return 1;
}

int cli_ac_initdata(struct cli_ac_data *data, unsigned int partsigs, unsigned int tracklen)
{
	unsigned int i, j;


    if(!data) {
	cli_errmsg("cli_ac_init(): data == NULL\n");
	return CL_ENULLARG;
    }

    data->partsigs = partsigs;

    if(!partsigs)
	return CL_SUCCESS;

    data->partcnt = (unsigned int *) cli_calloc(partsigs, sizeof(unsigned int));

    if(!data->partcnt) {
	cli_errmsg("cli_ac_init(): unable to cli_calloc(%u, %u)\n", partsigs, sizeof(unsigned int));
	return CL_EMEM;
    }

    data->offcnt = (uint8_t *) cli_calloc(partsigs, sizeof(uint8_t));

    if(!data->offcnt) {
	cli_errmsg("cli_ac_init(): unable to cli_calloc(%u, %u)\n", partsigs, sizeof(uint8_t));
	free(data->partcnt);
	return CL_EMEM;
    }

    data->offidx = (uint8_t *) cli_calloc(partsigs, sizeof(uint8_t));

    if(!data->offidx) {
	cli_errmsg("cli_ac_init(): unable to cli_calloc(%u, %u)\n", partsigs, sizeof(uint8_t));
	free(data->partcnt);
	free(data->offcnt);
	return CL_EMEM;
    }

    data->maxshift = (int *) cli_malloc(partsigs * sizeof(int));

    if(!data->maxshift) {
	cli_errmsg("cli_ac_init(): unable to cli_malloc(%u)\n", partsigs * sizeof(int));
	free(data->partcnt);
	free(data->offcnt);
	free(data->offidx);
	return CL_EMEM;
    }

    memset(data->maxshift, -1, partsigs * sizeof(int));

    data->partoff = (unsigned int **) cli_calloc(partsigs, sizeof(unsigned int *));

    if(!data->partoff) {
	cli_errmsg("cli_ac_init(): unable to cli_calloc(%u, %u)\n", partsigs, sizeof(unsigned int));
	free(data->partcnt);
	free(data->offcnt);
	free(data->offidx);
	free(data->maxshift);
	return CL_EMEM;
    }

    /* The number of multipart signatures is rather small so we already
     * allocate the memory for all parts here instead of using a runtime
     * allocation in cli_ac_scanbuff()
     */

    for(i = 0; i < partsigs; i++) {
	data->partoff[i] = (unsigned int *) cli_calloc(tracklen, sizeof(unsigned int));

	if(!data->partoff[i]) {
	    for(j = 0; j < i; j++)
		free(data->partoff[j]);

	    free(data->partoff);
	    free(data->partcnt);
	    free(data->offcnt);
	    free(data->offidx);
	    free(data->maxshift);
	    cli_errmsg("cli_ac_init(): unable to cli_calloc(%u, %u)\n", tracklen, sizeof(unsigned int));
	    return CL_EMEM;
	}
    }

    return CL_SUCCESS;
}

void cli_ac_freedata(struct cli_ac_data *data)
{
	unsigned int i;


    if(data && data->partsigs) {
	free(data->partcnt);
	free(data->offcnt);
	free(data->offidx);
	free(data->maxshift);

	for(i = 0; i < data->partsigs; i++)
	    free(data->partoff[i]);

	free(data->partoff);
    }
}

int cli_ac_scanbuff(const unsigned char *buffer, unsigned int length, const char **virname, const struct cli_matcher *root, struct cli_ac_data *mdata, unsigned short otfrec, unsigned long int offset, cli_file_t ftype, int fd, struct cli_matched_type **ftoffset)
{
	struct cli_ac_node *current;
	struct cli_ac_patt *pt;
	int type = CL_CLEAN, j;
        unsigned int i, position, curroff;
	uint8_t offnum, found;
	struct cli_matched_type *tnode;
	struct cli_target_info info;


    if(!root->ac_root)
	return CL_CLEAN;

    if(!mdata) {
	cli_errmsg("cli_ac_scanbuff(): mdata == NULL\n");
	return CL_ENULLARG;
    }

    memset(&info, 0, sizeof(info));
    current = root->ac_root;

    for(i = 0; i < length; i++)  {
	current = current->trans[buffer[i] & 0xff];

	if(current->islast) {
	    position = i - ac_depth + 1;

	    pt = current->list;
	    while(pt) {
		if(cli_findpos(buffer, ac_depth, position, length, pt)) {
		    curroff = offset + position - pt->prefix_length;

		    if((pt->offset || pt->target) && (!pt->sigid || pt->partno == 1)) {
			if((fd == -1 && !ftype) || !cli_validatesig(ftype, pt->offset, curroff, &info, fd, pt->virname)) {
			    pt = pt->next;
			    continue;
			}
		    }

		    if(pt->sigid) { /* it's a partial signature */

			if(mdata->partcnt[pt->sigid - 1] + 1 == pt->partno) {
			    offnum = mdata->offcnt[pt->sigid - 1];
			    if(offnum < AC_DEFAULT_TRACKLEN) {
				mdata->partoff[pt->sigid - 1][offnum] = curroff + pt->length;

				if(mdata->maxshift[pt->sigid - 1] == -1 || ((int) (mdata->partoff[pt->sigid - 1][offnum] - mdata->partoff[pt->sigid - 1][0]) <= mdata->maxshift[pt->sigid - 1]))
				    mdata->offcnt[pt->sigid - 1]++;
			    } else {
				if(mdata->maxshift[pt->sigid - 1] == -1 || ((int) (curroff + pt->length - mdata->partoff[pt->sigid - 1][0]) <= mdata->maxshift[pt->sigid - 1])) {
				    if(!(mdata->offidx[pt->sigid - 1] %= AC_DEFAULT_TRACKLEN))
					mdata->offidx[pt->sigid - 1]++;

				    mdata->partoff[pt->sigid - 1][mdata->offidx[pt->sigid - 1]] = curroff + pt->length;
				    mdata->offidx[pt->sigid - 1]++;
				}
			    }

			} else if(mdata->partcnt[pt->sigid - 1] + 2 == pt->partno) {
			    found = 0;
			    for(j = mdata->offcnt[pt->sigid - 1] - 1; j >= 0; j--) {
				found = 1;
				if(pt->maxdist)
				    if(curroff - mdata->partoff[pt->sigid - 1][j] > pt->maxdist)
					found = 0;

				if(found && pt->mindist)
				    if(curroff - mdata->partoff[pt->sigid - 1][j] < pt->mindist)
					found = 0;

				if(found)
				    break;
			    }

			    if(found) {
				mdata->maxshift[pt->sigid - 1] = mdata->partoff[pt->sigid - 1][j] + pt->maxdist - curroff;

				mdata->partoff[pt->sigid - 1][0] = curroff + pt->length;
				mdata->offcnt[pt->sigid - 1] = 1;

				if(++mdata->partcnt[pt->sigid - 1] + 1 == pt->parts) {
				    if(pt->type) {
					if(otfrec) {
					    if(pt->type > type || pt->type >= CL_TYPE_SFX) {
						cli_dbgmsg("Matched signature for file type %s\n", pt->virname);
						type = pt->type;
						if(ftoffset && (!*ftoffset || (*ftoffset)->cnt < SFX_MAX_TESTS) && ftype == CL_TYPE_MSEXE && type >= CL_TYPE_SFX) {
						    if(!(tnode = cli_calloc(1, sizeof(struct cli_matched_type)))) {
							cli_errmsg("cli_ac_scanbuff(): Can't allocate memory for new type node\n");
							if(info.exeinfo.section)
							    free(info.exeinfo.section);
							return CL_EMEM;
						    }

						    tnode->type = type;
						    tnode->offset = -1; /* we don't remember the offset of the first part */

						    if(*ftoffset)
							tnode->cnt = (*ftoffset)->cnt + 1;
						    else
							tnode->cnt = 1;

						    tnode->next = *ftoffset;
						    *ftoffset = tnode;
						}
					    }
					}
				    } else {
					if(virname)
					    *virname = pt->virname;

					if(info.exeinfo.section)
					    free(info.exeinfo.section);
					return CL_VIRUS;
				    }
				}
			    }
			}

		    } else { /* old type signature */
			if(pt->type) {
			    if(otfrec) {
				if(pt->type > type || pt->type >= CL_TYPE_SFX) {
				    cli_dbgmsg("Matched signature for file type %s at %u\n", pt->virname, curroff);
				    type = pt->type;
				    if(ftoffset && (!*ftoffset ||(*ftoffset)->cnt < SFX_MAX_TESTS) && ftype == CL_TYPE_MSEXE && type >= CL_TYPE_SFX) {
					if(!(tnode = cli_calloc(1, sizeof(struct cli_matched_type)))) {
					    cli_errmsg("cli_ac_scanbuff(): Can't allocate memory for new type node\n");
					    if(info.exeinfo.section)
						free(info.exeinfo.section);
					    return CL_EMEM;
					}
					tnode->type = type;
					tnode->offset = curroff;

					if(*ftoffset)
					    tnode->cnt = (*ftoffset)->cnt + 1;
					else
					    tnode->cnt = 1;

					tnode->next = *ftoffset;
					*ftoffset = tnode;
				    }
				}
			    }
			} else {
			    if(virname)
				*virname = pt->virname;

			    if(info.exeinfo.section)
				free(info.exeinfo.section);
			    return CL_VIRUS;
			}
		    }
		}

		pt = pt->next;
	    }

	    current = current->fail;
	}
    }

    if(info.exeinfo.section)
	free(info.exeinfo.section);

    return otfrec ? type : CL_CLEAN;
}

void cli_ac_setdepth(unsigned int depth)
{
    ac_depth = depth;
}

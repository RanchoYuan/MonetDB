/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 *                * Copyright August 2008-2014 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * (c)2014 author Martin Kersten
 * The header block contains the mapping from OIDs to chunks
 */

static void
MOSdumpHeader(Client cntxt, MOStask task)
{
	MosaicHdr hdr = (MosaicHdr) task->hdr;
	int i;

	mnstr_printf(cntxt->fdout,"#header block version %d\n", hdr->version);
	mnstr_printf(cntxt->fdout,"#index top %d\n", hdr->top);
	for(i= 0; i< hdr->top; i++)
		mnstr_printf(cntxt->fdout,"#[%d] "OIDFMT" " LLFMT "\n",i, hdr->index[i], hdr->offset[i]);
}

// add the chunk to the index to facilitate 'fast' OID-based access
static void
MOSupdateHeader(Client cntxt, MOStask task)
{
	MosaicHdr hdr = (MosaicHdr) task->hdr;
	lng minsize = -1;
	int i, j;

	(void) cntxt;
    task->wins[task->blk->tag]++;
	if( hdr->top < MOSAICINDEX-1 ){
		if( hdr->top == 0)
			hdr->index[hdr->top] = task->blk->cnt;
		else hdr->index[hdr->top] = task->blk->cnt + hdr->index[hdr->top-1];
		hdr->offset[hdr->top] =  (lng) task->dst - (lng) task->hdr;
		hdr->top++;
		MOSdumpHeader(cntxt,task);
		return;
	}
	// compress the index by finding the smallest compressed fragment pair
	hdr->index[hdr->top] = task->blk->cnt + hdr->index[hdr->top-1];
	hdr->offset[hdr->top] =  (lng) task->dst - (lng) task->hdr;
	minsize = hdr->offset[1];
	j = 0;
	for( i = 1; i <= hdr->top; i++)
	if ( hdr->offset[i] - hdr->offset[i-1] < minsize ){
		minsize = hdr->offset[i] - hdr->offset[i-1];
		j = i;
	}
	mnstr_printf(cntxt->fdout,"#ditch entry %d\n",j);
	// simply remove on element
	for( i = j; i < hdr->top; i++){
		hdr->index[i]  = hdr->index[i+1];
		hdr->offset[i] = hdr->offset[i+1];
	}
	MOSdumpHeader(cntxt,task);
}

static void
MOSinitHeader(MOStask task)
{
	MosaicHdr hdr = (MosaicHdr) task->hdr;
	hdr->version = MOSAIC_VERSION;
	hdr->top = 0;
}

// determine the index of the chunk that contains 
// the value of a specific oid, update the task
static void
MOSfindChunk(Client cntxt, MOStask task, oid o)
{
	MosaicHdr hdr = (MosaicHdr) task->hdr;
	int i = 0;

	(void) cntxt;
	assert( o <= hdr->index[hdr->top-1]);
	for(;  i <hdr->top-1; i++)
		if ( hdr->index[i+1] > o)
			break;
	task->blk = (MosaicBlk) task->dst + hdr->offset[i];
}

/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/blenlib/intern/BLI_ghash.c
 *  \ingroup bli
 *
 * A general (pointer -> pointer) hash table ADT
 *
 * \note edgehash.c is based on this, make sure they stay in sync.
 */

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "MEM_guardedalloc.h"

#include "BLI_sys_types.h"  /* for intptr_t support */
#include "BLI_utildefines.h"
#include "BLI_mempool.h"
#include "BLI_ghash.h"

/***/

#ifdef __GNUC__
#  pragma GCC diagnostic error "-Wsign-conversion"
#  if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406  /* gcc4.6+ only */
#    pragma GCC diagnostic error "-Wsign-compare"
#    pragma GCC diagnostic error "-Wconversion"
#  endif
#endif

const unsigned int hashsizes[] = {
	5, 11, 17, 37, 67, 131, 257, 521, 1031, 2053, 4099, 8209, 
	16411, 32771, 65537, 131101, 262147, 524309, 1048583, 2097169, 
	4194319, 8388617, 16777259, 33554467, 67108879, 134217757, 
	268435459
};

/***/

typedef struct Entry {
	struct Entry *next;

	void *key, *val;
} Entry;

struct GHash {
	GHashHashFP hashfp;
	GHashCmpFP cmpfp;

	Entry **buckets;
	struct BLI_mempool *entrypool;
	unsigned int nbuckets;
	unsigned int nentries;
	unsigned short cursize, flag;
};


/* -------------------------------------------------------------------- */
/* GHash API */

/* internal utility API */

BLI_INLINE bool ghash_test_expand_buckets(const unsigned int nentries, const unsigned int nbuckets)
{
	return (nentries > nbuckets * 3);
}

BLI_INLINE unsigned int ghash_keyhash(GHash *gh, const void *key)
{
	return gh->hashfp(key) % gh->nbuckets;
}

BLI_INLINE Entry *ghash_lookup_entry_ex(GHash *gh, const void *key,
                                        const unsigned int hash)
{
	Entry *e;

	for (e = gh->buckets[hash]; e; e = e->next) {
		if (gh->cmpfp(key, e->key) == 0) {
			return e;
		}
	}
	return NULL;
}

BLI_INLINE Entry *ghash_lookup_entry(GHash *gh, const void *key)
{
	const unsigned int hash = ghash_keyhash(gh, key);
	return ghash_lookup_entry_ex(gh, key, hash);
}

static void ghash_insert_ex(GHash *gh, void *key, void *val,
                            unsigned int hash)
{
	Entry *e = (Entry *)BLI_mempool_alloc(gh->entrypool);

	BLI_assert((gh->flag & GHASH_FLAG_ALLOW_DUPES) || (BLI_ghash_haskey(gh, key) == 0));

	e->next = gh->buckets[hash];
	e->key = key;
	e->val = val;
	gh->buckets[hash] = e;

	if (UNLIKELY(ghash_test_expand_buckets(++gh->nentries, gh->nbuckets))) {
		Entry **old = gh->buckets;
		const unsigned nold = gh->nbuckets;
		unsigned int i;

		gh->nbuckets = hashsizes[++gh->cursize];
		gh->buckets = (Entry **)MEM_callocN(gh->nbuckets * sizeof(*gh->buckets), "buckets");

		for (i = 0; i < nold; i++) {
			Entry *e_next;
			for (e = old[i]; e; e = e_next) {
				e_next = e->next;
				hash = ghash_keyhash(gh, e->key);
				e->next = gh->buckets[hash];
				gh->buckets[hash] = e;
			}
		}

		MEM_freeN(old);
	}
}


/* Public API */

GHash *BLI_ghash_new_ex(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info,
                        const unsigned int nentries_reserve)
{
	GHash *gh = MEM_mallocN(sizeof(*gh), info);

	gh->hashfp = hashfp;
	gh->cmpfp = cmpfp;

	gh->nbuckets = hashsizes[0];  /* gh->cursize */
	gh->nentries = 0;
	gh->cursize = 0;
	gh->flag = 0;

	/* if we have reserved the number of elements that this hash will contain */
	if (nentries_reserve) {
		while (ghash_test_expand_buckets(nentries_reserve, gh->nbuckets)) {
			gh->nbuckets = hashsizes[++gh->cursize];
		}
	}

	gh->buckets = MEM_callocN(gh->nbuckets * sizeof(*gh->buckets), "buckets");
	gh->entrypool = BLI_mempool_create(sizeof(Entry), 64, 64, 0);

	return gh;
}

GHash *BLI_ghash_new(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info)
{
	return BLI_ghash_new_ex(hashfp, cmpfp, info, 0);
}

int BLI_ghash_size(GHash *gh)
{
	return (int)gh->nentries;
}

void BLI_ghash_insert(GHash *gh, void *key, void *val)
{
	const unsigned int hash = ghash_keyhash(gh, key);
	ghash_insert_ex(gh, key, val, hash);
}

/**
 * Assign a new value to a key that may already be in ghash.
 * Avoids #BLI_ghash_remove, #BLI_ghash_insert calls (double lookups)
 */
void BLI_ghash_reinsert(GHash *gh, void *key, void *val, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp)
{
	const unsigned int hash = ghash_keyhash(gh, key);
	Entry *e = ghash_lookup_entry_ex(gh, key, hash);
	if (e) {
		if (keyfreefp) keyfreefp(e->key);
		if (valfreefp) valfreefp(e->val);

		e->key = key;
		e->val = val;
	}
	else {
		ghash_insert_ex(gh, key, val, hash);
	}
}

void *BLI_ghash_lookup(GHash *gh, const void *key)
{
	Entry *e = ghash_lookup_entry(gh, key);
	return e ? e->val : NULL;
}

void **BLI_ghash_lookup_p(GHash *gh, const void *key)
{
	Entry *e = ghash_lookup_entry(gh, key);
	return e ? &e->val : NULL;
}

bool BLI_ghash_remove(GHash *gh, void *key, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp)
{
	const unsigned int hash = ghash_keyhash(gh, key);
	Entry *e;
	Entry *p = NULL;

	for (e = gh->buckets[hash]; e; e = e->next) {
		if (gh->cmpfp(key, e->key) == 0) {
			Entry *n = e->next;

			if (keyfreefp) keyfreefp(e->key);
			if (valfreefp) valfreefp(e->val);
			BLI_mempool_free(gh->entrypool, e);

			/* correct but 'e' isn't used before return */
			/* e = n; *//*UNUSED*/
			if (p) p->next = n;
			else   gh->buckets[hash] = n;

			gh->nentries--;
			return true;
		}
		p = e;
	}

	return false;
}

/* same as above but return the value,
 * no free value argument since it will be returned */
void *BLI_ghash_pop(GHash *gh, void *key, GHashKeyFreeFP keyfreefp)
{
	const unsigned int hash = ghash_keyhash(gh, key);
	Entry *e;
	Entry *p = NULL;

	for (e = gh->buckets[hash]; e; e = e->next) {
		if (gh->cmpfp(key, e->key) == 0) {
			Entry *n = e->next;
			void *value = e->val;

			if (keyfreefp) keyfreefp(e->key);
			BLI_mempool_free(gh->entrypool, e);

			/* correct but 'e' isn't used before return */
			/* e = n; *//*UNUSED*/
			if (p) p->next = n;
			else   gh->buckets[hash] = n;

			gh->nentries--;
			return value;
		}
		p = e;
	}

	return NULL;
}

bool BLI_ghash_haskey(GHash *gh, const void *key)
{
	return (ghash_lookup_entry(gh, key) != NULL);
}

void BLI_ghash_clear(GHash *gh, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp)
{
	unsigned int i;

	if (keyfreefp || valfreefp) {
		for (i = 0; i < gh->nbuckets; i++) {
			Entry *e;

			for (e = gh->buckets[i]; e; ) {
				Entry *n = e->next;

				if (keyfreefp) keyfreefp(e->key);
				if (valfreefp) valfreefp(e->val);

				e = n;
			}
		}
	}

	gh->cursize = 0;
	gh->nentries = 0;
	gh->nbuckets = hashsizes[gh->cursize];

	MEM_freeN(gh->buckets);
	gh->buckets = MEM_callocN(gh->nbuckets * sizeof(*gh->buckets), "buckets");
}

void BLI_ghash_free(GHash *gh, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp)
{
	unsigned int i;

	if (keyfreefp || valfreefp) {
		for (i = 0; i < gh->nbuckets; i++) {
			Entry *e;

			for (e = gh->buckets[i]; e; ) {
				Entry *n = e->next;

				if (keyfreefp) keyfreefp(e->key);
				if (valfreefp) valfreefp(e->val);

				e = n;
			}
		}
	}

	MEM_freeN(gh->buckets);
	BLI_mempool_destroy(gh->entrypool);
	gh->buckets = NULL;
	gh->nentries = 0;
	gh->nbuckets = 0;
	MEM_freeN(gh);
}

void BLI_ghash_flag_set(GHash *gh, unsigned short flag)
{
	gh->flag |= flag;
}

void BLI_ghash_flag_clear(GHash *gh, unsigned short flag)
{
	gh->flag &= (unsigned short)~flag;
}


/* -------------------------------------------------------------------- */
/* GHash Iterator API */

GHashIterator *BLI_ghashIterator_new(GHash *gh)
{
	GHashIterator *ghi = MEM_mallocN(sizeof(*ghi), "ghash iterator");
	BLI_ghashIterator_init(ghi, gh);
	return ghi;
}
void BLI_ghashIterator_init(GHashIterator *ghi, GHash *gh)
{
	ghi->gh = gh;
	ghi->curEntry = NULL;
	ghi->curBucket = UINT_MAX;  /* wraps to zero */
	while (!ghi->curEntry) {
		ghi->curBucket++;
		if (ghi->curBucket == ghi->gh->nbuckets)
			break;
		ghi->curEntry = ghi->gh->buckets[ghi->curBucket];
	}
}
void BLI_ghashIterator_free(GHashIterator *ghi)
{
	MEM_freeN(ghi);
}

void *BLI_ghashIterator_getKey(GHashIterator *ghi)
{
	return ghi->curEntry ? ghi->curEntry->key : NULL;
}
void *BLI_ghashIterator_getValue(GHashIterator *ghi)
{
	return ghi->curEntry ? ghi->curEntry->val : NULL;
}

void BLI_ghashIterator_step(GHashIterator *ghi)
{
	if (ghi->curEntry) {
		ghi->curEntry = ghi->curEntry->next;
		while (!ghi->curEntry) {
			ghi->curBucket++;
			if (ghi->curBucket == ghi->gh->nbuckets)
				break;
			ghi->curEntry = ghi->gh->buckets[ghi->curBucket];
		}
	}
}
bool BLI_ghashIterator_done(GHashIterator *ghi)
{
	return ghi->curEntry == NULL;
}

/***/

unsigned int BLI_ghashutil_ptrhash(const void *key)
{
	return (unsigned int)(intptr_t)key;
}
int BLI_ghashutil_ptrcmp(const void *a, const void *b)
{
	if (a == b)
		return 0;
	else
		return (a < b) ? -1 : 1;
}

unsigned int BLI_ghashutil_inthash(const void *ptr)
{
	uintptr_t key = (uintptr_t)ptr;

	key += ~(key << 16);
	key ^=  (key >>  5);
	key +=  (key <<  3);
	key ^=  (key >> 13);
	key += ~(key <<  9);
	key ^=  (key >> 17);

	return (unsigned int)(key & 0xffffffff);
}

int BLI_ghashutil_intcmp(const void *a, const void *b)
{
	if (a == b)
		return 0;
	else
		return (a < b) ? -1 : 1;
}

/**
 * This function implements the widely used "djb" hash apparently posted
 * by Daniel Bernstein to comp.lang.c some time ago.  The 32 bit
 * unsigned hash value starts at 5381 and for each byte 'c' in the
 * string, is updated: <literal>hash = hash * 33 + c</literal>.  This
 * function uses the signed value of each byte.
 *
 * note: this is the same hash method that glib 2.34.0 uses.
 */
unsigned int BLI_ghashutil_strhash(const void *ptr)
{
	const signed char *p;
	unsigned int h = 5381;

	for (p = ptr; *p != '\0'; p++) {
		h = (h << 5) + h + (unsigned int)*p;
	}

	return h;
}
int BLI_ghashutil_strcmp(const void *a, const void *b)
{
	return strcmp(a, b);
}

GHash *BLI_ghash_ptr_new_ex(const char *info,
                            const unsigned int nentries_reserve)
{
	return BLI_ghash_new_ex(BLI_ghashutil_ptrhash, BLI_ghashutil_ptrcmp, info,
	                        nentries_reserve);
}
GHash *BLI_ghash_ptr_new(const char *info)
{
	return BLI_ghash_ptr_new_ex(info, 0);
}

GHash *BLI_ghash_str_new_ex(const char *info,
                            const unsigned int nentries_reserve)
{
	return BLI_ghash_new_ex(BLI_ghashutil_strhash, BLI_ghashutil_strcmp, info,
	                        nentries_reserve);
}
GHash *BLI_ghash_str_new(const char *info)
{
	return BLI_ghash_str_new_ex(info, 0);
}

GHash *BLI_ghash_int_new_ex(const char *info,
                            const unsigned int nentries_reserve)
{
	return BLI_ghash_new_ex(BLI_ghashutil_inthash, BLI_ghashutil_intcmp, info,
	                        nentries_reserve);
}
GHash *BLI_ghash_int_new(const char *info)
{
	return BLI_ghash_int_new_ex(info, 0);
}

GHash *BLI_ghash_pair_new_ex(const char *info,
                             const unsigned int nentries_reserve)
{
	return BLI_ghash_new_ex(BLI_ghashutil_pairhash, BLI_ghashutil_paircmp, info,
	                        nentries_reserve);
}
GHash *BLI_ghash_pair_new(const char *info)
{
	return BLI_ghash_pair_new_ex(info, 0);
}

GHashPair *BLI_ghashutil_pairalloc(const void *first, const void *second)
{
	GHashPair *pair = MEM_mallocN(sizeof(GHashPair), "GHashPair");
	pair->first = first;
	pair->second = second;
	return pair;
}

unsigned int BLI_ghashutil_pairhash(const void *ptr)
{
	const GHashPair *pair = ptr;
	unsigned int hash = BLI_ghashutil_ptrhash(pair->first);
	return hash ^ BLI_ghashutil_ptrhash(pair->second);
}

int BLI_ghashutil_paircmp(const void *a, const void *b)
{
	const GHashPair *A = a;
	const GHashPair *B = b;

	int cmp = BLI_ghashutil_ptrcmp(A->first, B->first);
	if (cmp == 0)
		return BLI_ghashutil_ptrcmp(A->second, B->second);
	return cmp;
}

void BLI_ghashutil_pairfree(void *ptr)
{
	MEM_freeN(ptr);
}

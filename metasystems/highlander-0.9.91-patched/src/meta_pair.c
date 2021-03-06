/*
    libcoremeta - A library of useful C functions and ADT's
    Copyright (C) 2000-2006 B. Augestad, boa@metasystems.no 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <meta_common.h>
#include <meta_pair.h>

/**
 * Local helper structure.
 */
struct element {
	char* name;
	char* value;
};

/**
 * Implementation of the meta_pair ADT
 */
struct pair_tag {
	struct element* element;
	size_t nelem;
	size_t used;
};

/* Small helper returning the index to an element or 0 if not found */
static int pair_find(pair p, const char* name, size_t* pi)
{
	assert(p != NULL);

	for(*pi = 0; *pi < p->used; (*pi)++) {
		if(0 == strcmp(p->element[*pi].name, name)) 
			return 1;
	}

	return 0;
}

pair pair_new(size_t nelem)
{
	pair p;

	assert(nelem > 0);

	if( (p = mem_malloc(sizeof *p)) == NULL)
		;
	else if( (p->element = mem_calloc(nelem, sizeof *p->element)) == NULL) {
		mem_free(p);
		p = NULL;
	}
	else {
		p->used = 0;
		p->nelem = nelem; 
	}

	return p;
}

void pair_free(pair p)
{
	size_t i;

	assert(p != NULL);

	for(i = 0; i < p->used; i++) {
		mem_free(p->element[i].name);
		mem_free(p->element[i].value);
	}

	mem_free(p->element);
	mem_free(p);
}

static int pair_extend(pair p, size_t cElementsToAdd)
{
	struct element* pnew;

	assert(p != NULL);

	pnew = realloc(p->element, sizeof *p->element * (p->nelem + cElementsToAdd));
	if(pnew != NULL) {
		p->element = pnew;
		p->nelem += cElementsToAdd;
		return 1;
	} 
	else 
		return 0;
}

size_t pair_size(pair p)
{
	assert(p != NULL);

	return p->used;
}

const char* pair_get_value_by_index(pair p, size_t i)
{
	assert(p != NULL);
	assert(i < p->nelem);

	return p->element[i].value;
}

/* Returns pointer to value if name exists, else NULL */
const char* pair_get(pair p, const char* name)
{
	size_t i;

	if(!pair_find(p, name, &i)) 
		return NULL;
	else
		return p->element[i].value;
}


/* overwrites an existing value . Adds a new entry if not.
 * returns
 * 	1	OK
 * 	0	Memory problems
 */
int pair_set(pair p, const char* name, const char* value)
{
	size_t i, cbAvailable, cbRequired;

	if(!pair_find(p, name, &i))
		return pair_add(p, name, value);

	/* Modify existing value */
	cbAvailable = strlen(p->element[i].value) + 1;
	cbRequired = strlen(value) + 1;
	if(cbAvailable < cbRequired) {
		char* s = realloc(p->element[i].value, cbRequired);
		if(NULL == s) 
			return 0;

		p->element[i].value = s;
	}

	strcpy(p->element[i].value, value);
	return 1;
}

int pair_add(pair p, const char* name, const char* value)
{
	struct element* pnew; /* Just a helper to beautify the code */
	
	/* Resize when needed */
	if(p->used == p->nelem) {
		if(!pair_extend(p, p->nelem * 2))
			return 0;
	}

	/* Assign the helper */
	pnew = &p->element[p->used];
	if( (pnew->name = mem_malloc(strlen(name) + 1)) == NULL)
		return 0;
	else if( (pnew->value = mem_malloc(strlen(value) + 1)) == NULL) {
		mem_free(pnew->name);
		return 0;
	}
	else {
		strcpy(pnew->name, name);
		strcpy(pnew->value, value);
		p->used++;
		return 1;
	}
}

const char* pair_get_name(pair p, size_t idx)
{
	return p->element[idx].name;
}

#ifdef CHECK_PAIR
#include <stdio.h>

int main(void)
{
	pair p;
	size_t i, j, niter = 100, nelem = 10;
	char name[1024], value[1024];
	const char* pval;

	for(i = 0; i < niter; i++) {
		if( (p = pair_new(nelem)) == NULL)
			return 1;

		/* Add a bunch of pairs */
		for(j = 0; j < nelem * 2; j++) {
			sprintf(name, "name %lu", (unsigned long)j);
			sprintf(value, "value %lu", (unsigned long)j);
			if(!pair_add(p, name, value))
				return 1;
		}

		/* Locate the same pairs and compare the returned value */
		for(j = 0; j < nelem * 2; j++) {
			sprintf(name, "name %lu", (unsigned long)j);
			sprintf(value, "value %lu", (unsigned long)j);
			if( (pval = pair_get(p, name)) == NULL)
				return 1;
			else if(strcmp(pval, value) != 0)
				return 1;
		}

		/* Manually extend the pair and then try again */
		if(!pair_extend(p, nelem * 2)) 
			return 1;

		/* Add a bunch of pairs */
		for(j = 0; j < nelem * 3; j++) {
			sprintf(name, "name %lu", (unsigned long)j);
			sprintf(value, "value %lu", (unsigned long)j);
			if(!pair_add(p, name, value))
				return 1;
		}

		/* Locate the same pairs and compare the returned value */
		for(j = 0; j < nelem * 3; j++) {
			sprintf(name, "name %lu", (unsigned long)j);
			sprintf(value, "value %lu", (unsigned long)j);
			if( (pval = pair_get(p, name)) == NULL)
				return 1;
			else if(strcmp(pval, value) != 0)
				return 1;
		}


		pair_free(p);

	}

	return 0;
}
#endif



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
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#include <meta_cache.h>
#include <meta_stringmap.h>
#include <meta_filecache.h>

/* We store a couple of things for each file in a struct like this */
fileinfo fileinfo_new(void)
{
	fileinfo p;

	if( (p = mem_malloc(sizeof *p)) != NULL) {
		p->mimetype = NULL;
		p->name = NULL;
		p->alias = NULL;
		p->contents = NULL;
	}

	return p;
}

#if 0
fileinfo fileinfo_dup(const fileinfo src)
{
	fileinfo p;

	if( (p = fileinfo_new()) != NULL) {
		if( (p->mimetype = mem_malloc(strlen(src->mimetype) + 1)) == NULL
		||  (p->name = mem_malloc(strlen(src->name) + 1)) == NULL
		||  (p->alias = mem_malloc(strlen(src->alias) + 1)) == NULL) {
			fileinfo_free(p);
			p = NULL;
		}

		strcpy(p->mimetype, src->mimetype);
		strcpy(p->name, src->name);
		strcpy(p->alias, src->alias);
		p->st = src->st;
	}

	return p;
}
#endif
	

void fileinfo_free(fileinfo p)
{
	if(p != NULL) {
		mem_free(p->contents);
		mem_free(p->mimetype);
		mem_free(p->alias);
		mem_free(p->name);
		mem_free(p);
	}
}

int fileinfo_set_stat(fileinfo p, const struct stat* pst)
{
	assert(p != NULL);
	assert(pst != NULL);
	p->st = *pst;
	return 1;
}

int fileinfo_set_name(fileinfo p, const char* s)
{
	assert(p != NULL);
	assert(s != NULL);

	if(p->name != NULL)
		mem_free(p->name);

	if( (p->name = mem_malloc(strlen(s) + 1)) == NULL)
		return 0;

	strcpy(p->name, s);
	return 1;
}

int fileinfo_set_alias(fileinfo p, const char* s)
{
	assert(p != NULL);
	assert(s != NULL);

	if(p->alias != NULL)
		mem_free(p->alias);

	if( (p->alias = mem_malloc(strlen(s) + 1)) == NULL)
		return 0;

	strcpy(p->alias, s);
	return 1;
}

int fileinfo_set_mimetype(fileinfo p, const char* s)
{
	assert(p != NULL);
	assert(s != NULL);

	if(p->mimetype != NULL)
		mem_free(p->mimetype);

	if( (p->mimetype = mem_malloc(strlen(s) + 1)) == NULL)
		return 0;

	strcpy(p->mimetype, s);
	return 1;
}


#define HOTLIST_SIZE 10

filecache filecache_new(size_t nelem, size_t bytes)
{
	filecache fc;

	assert(nelem > 0);
	assert(bytes > 0);

	if( (fc = mem_malloc(sizeof *fc)) == NULL
	||  (fc->filenames = stringmap_new(nelem)) == NULL
	||  (fc->metacache = cache_new(nelem, HOTLIST_SIZE, bytes)) == NULL) {
		stringmap_free(fc->filenames);
		mem_free(fc);
		fc = NULL;
	}
	else  {
		pthread_rwlock_init(&fc->lock, NULL);
		fc->nelem = nelem;
		fc->bytes = bytes;
	}

	return fc;
}

void filecache_free(filecache fc)
{
	if(fc != NULL) {
		cache_free(fc->metacache, (dtor)fileinfo_free);
		stringmap_free(fc->filenames);
		pthread_rwlock_destroy(&fc->lock);
		mem_free(fc);
	}
}

int filecache_add(filecache fc, fileinfo finfo, int pin, unsigned long* pid)
{
	int rc, fd = -1;

	char* contents = NULL;

	assert(fc != NULL);
	assert(finfo != NULL);
	assert(finfo->contents == NULL);

	if( (fd = open(fileinfo_name(finfo), O_RDONLY)) == -1)
		goto err;
	else if( (finfo->contents = mem_malloc(finfo->st.st_size)) == NULL)
		goto err;
	else if(read(fd, finfo->contents, (size_t)finfo->st.st_size) != (ssize_t)finfo->st.st_size)
		goto err;
	else if(close(fd) == -1)
		goto err;

	fd = -1;

	pthread_rwlock_wrlock(&fc->lock);
	rc = stringmap_add(fc->filenames, fileinfo_alias(finfo), pid)
		&& cache_add(fc->metacache, *pid, finfo, sizeof *finfo, pin) ;
	pthread_rwlock_unlock(&fc->lock);
	if(rc)
		return 1;

err:
	if(fd != -1)
		close(fd);
	mem_free(contents);
	fileinfo_free(finfo);
	return 0;
}


int filecache_invalidate(filecache fc)
{
	int rc = 1;

	pthread_rwlock_wrlock(&fc->lock);
	stringmap_free(fc->filenames);
	cache_free(fc->metacache, (dtor)fileinfo_free);

	if( (fc->filenames = stringmap_new(fc->nelem)) == NULL
	||  (fc->metacache = cache_new(fc->nelem, HOTLIST_SIZE, fc->bytes)) == NULL) {
		stringmap_free(fc->filenames);
		fc->filenames = NULL;
		fc->metacache = NULL;
	}

	pthread_rwlock_unlock(&fc->lock);
	return rc;
}

double filecache_hitratio(filecache fc)
{
	(void)fc;
	return 1.0;
}

int filecache_exists(filecache fc, const char* filename)
{
	unsigned long id;
	int rc = 0;

	pthread_rwlock_rdlock(&fc->lock);
	if(stringmap_get_id(fc->filenames, filename, &id)) 
		rc = 1;

	pthread_rwlock_unlock(&fc->lock);

	return rc;
}

int filecache_get(filecache fc, const char* filename, void** pdata, size_t* pcb)
{
	unsigned long id;
	int rc = 0;
	void* p;

	pthread_rwlock_rdlock(&fc->lock);

	if(stringmap_get_id(fc->filenames, filename, &id)) {
		rc = cache_get(fc->metacache, id, (void**)&p, pcb);
		if(rc) {
			fileinfo fi = p;
			*pdata = fi->contents;
			*pcb = fi->st.st_size;
		}
	}

	pthread_rwlock_unlock(&fc->lock);
	return rc;
}

int filecache_foreach(filecache fc, int(*fn)(const char*s, void* arg), void* arg)
{
	int rc;

	assert(fc != NULL);
	assert(fn != NULL);

	pthread_rwlock_rdlock(&fc->lock);

	rc = stringmap_foreach(fc->filenames, fn, arg);
	pthread_rwlock_unlock(&fc->lock);
	return rc;
	
}

int filecache_stat(filecache fc, const char* filename, struct stat* p)
{
	unsigned long id;
	void* pst = NULL;
	size_t cb;
	int rc = 0;

	assert(fc != NULL);
	assert(filename != NULL);

	pthread_rwlock_rdlock(&fc->lock);

	if(stringmap_get_id(fc->filenames, filename, &id)) {
		if(cache_get(fc->metacache, id, (void**)&pst, &cb)) {
			*p = *fileinfo_stat(pst);
			rc = 1;
		}
	}

	pthread_rwlock_unlock(&fc->lock);
	return rc;
}

int filecache_get_mime_type(filecache fc, const char* filename, char mime[], size_t cb)
{
	unsigned long id;
	void *p;
	size_t cbptr;
	int rc = 0;

	pthread_rwlock_rdlock(&fc->lock);

	if(stringmap_get_id(fc->filenames, filename, &id)) {
		if(cache_get(fc->metacache, id, (void**)&p, &cbptr)) {
			mime[0] = '\0';
			strncat(mime, fileinfo_mimetype(p), cb - 1);
			rc = 1;
		}
	}

	pthread_rwlock_unlock(&fc->lock);
	return rc;
}

fileinfo filecache_fileinfo(filecache fc, const char* filename)
{
	unsigned long id;
	void* pst = NULL;
	size_t cb;
	int found = 0;

	assert(fc != NULL);
	assert(filename != NULL);


	pthread_rwlock_rdlock(&fc->lock);

	if(stringmap_get_id(fc->filenames, filename, &id)) 
		found = cache_get(fc->metacache, id, (void**)&pst, &cb);

	pthread_rwlock_unlock(&fc->lock);

	if(found)
		return pst;
	else
		return NULL;
}

#ifdef CHECK_FILECACHE
#include <stdio.h>
#include <dirent.h>

int main(void)
{
	DIR* d;
	struct dirent* de;
	struct stat st;
	filecache fc;
	unsigned long id;
	void* pdata;
	size_t cb;
	FILE* f;
	char mime[128];
	fileinfo fi;
	

	fc = filecache_new(100, 100*1024*1024);

	if( (d = opendir(".")) == NULL) {
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	while( (de = readdir(d)) != NULL) {
		if(stat(de->d_name, &st) == -1) {
			perror("fstat");
			exit(EXIT_FAILURE);
		}

		if(S_ISREG(st.st_mode)) {
			char buf[10240];
			size_t offset;
			sprintf(buf, "file -bi %s", de->d_name);
			if( (f = popen(buf, "r")) == NULL) {
				perror("popen");
				exit(EXIT_FAILURE);
			}
			if(fgets(buf, sizeof buf, f) == NULL) {
				perror("fgets");
				exit(EXIT_FAILURE);
			}
			pclose(f);

			/* Nicify the mime type */
			offset = strcspn(buf, ":;, \t\n");
			buf[offset] = '\0';

			fi = fileinfo_new();
			fileinfo_set_name(fi, de->d_name);
			fileinfo_set_alias(fi, de->d_name);
			fileinfo_set_stat(fi, &st);
			fileinfo_set_mimetype(fi, buf);

			if(st.st_size == 0)
				; /* Do not add empty files */
			else if(!filecache_add(fc, fi, 0, &id)) {
				perror(de->d_name);
				exit(EXIT_FAILURE);
			}
			else if(!filecache_get(fc, de->d_name, (void**)&pdata, &cb)) {
				perror(de->d_name);
				exit(EXIT_FAILURE);
			}
			else if(!filecache_get_mime_type(fc, de->d_name, mime, sizeof mime)){
				perror(de->d_name);
				exit(EXIT_FAILURE);
			}
		}
	}

	closedir(d);
	filecache_free(fc);
	return 0;
}
#endif


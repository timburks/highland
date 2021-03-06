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
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <meta_common.h>
#include <cstring.h>
#include <meta_misc.h>

#include <meta_process.h>

/**
 * This small struct stores objects we handle.
 */
struct srv {
	void *object;
	int (*do_func)(void *object);
	int (*undo_func)(void *object);
	int (*run_func)(void *object);
	int (*shutdown_func)(void *object);

	/* We must wait for each started object to finish
	 * so that the main process can e.g. free the object
	 * after process_wait_for_shutdown() returns.
	 * Here's the thread id for the object started.
	 */
	pthread_t tid;

	/* Store the exit code from the run_func() function here. */
	int exitcode;
};

/**
 * Max number of objects a process can start.
 * Each object uses (on x86) 7*4 bytes, so this is
 * very low overhead.
 */
#define MAX_OBJECTS 200


/**
 * Implementation of our process ADT.
 */
struct process_tag {
	cstring appname;
	cstring rootdir;
	cstring username;

	struct srv objects[MAX_OBJECTS];
	size_t objects_used;

	int shutting_down;

	/* The shutdown thread */
	pthread_t sdt;
};

process process_new(const char* appname)
{
	process p;

	if( (p = mem_calloc(1, sizeof *p)) == NULL)
		;
	else if( (p->rootdir = cstring_new()) == NULL
	|| (p->appname = cstring_new()) == NULL
	|| (p->username = cstring_new()) == NULL
	|| !cstring_copy(p->appname, appname)) {
		cstring_free(p->appname);
		cstring_free(p->username);
		cstring_free(p->rootdir);
		mem_free(p);
		p = NULL;
	}
	else {
		p->objects_used = 0;
		p->shutting_down = 0;
	}

	return p;
}

void process_free(process p)
{
	if(p != NULL) {
		cstring_free(p->appname);
		cstring_free(p->username);
		cstring_free(p->rootdir);
		mem_free(p);
	}
}

int process_set_rootdir(process p, const char* path)
{
	assert(p != NULL);
	assert(path != NULL);

	return cstring_copy(p->rootdir, path);
}

int process_set_username(process p, const char* username)
{
	assert(p != NULL);
	assert(username != NULL);

	return cstring_copy(p->username, username);
}

int process_add_object_to_start(
	process p,
	void *object,
	int do_func(void *),
	int undo_func(void *),
	int run_func(void *),
	int shutdown_func(void *))
{
	assert(p != NULL);
	assert(object != NULL);

	/* We need either both the do/undo functions or none */
	assert(do_func != NULL || undo_func == NULL);
	assert(undo_func != NULL || do_func == NULL);

	assert(run_func != NULL);
	assert(shutdown_func != NULL);

	if(p->objects_used == MAX_OBJECTS) 
		return 0;

	p->objects[p->objects_used].object = object;
	p->objects[p->objects_used].do_func = do_func;
	p->objects[p->objects_used].undo_func = undo_func;
	p->objects[p->objects_used].run_func = run_func;
	p->objects[p->objects_used].shutdown_func = shutdown_func;
	p->objects[p->objects_used].exitcode = -1;
	p->objects_used++;
	return 1;
}

/* 
 * We ignore SIGPIPE for the entire process.
 * This way write() will return -1 and errno will be EPIPE when the
 * client has disconnected and we try write to the socket.
 * This function returns 1 on success, else 0.
 */
static int set_signals_to_block(void)
{
	sigset_t block;
	int success = 0;

	errno = 0;
	if(SIG_ERR == signal(SIGPIPE, SIG_IGN))
		;
	else if(-1 == sigemptyset(&block))
		;
	else if(-1 == sigaddset(&block, SIGTERM))
		;
	else if(pthread_sigmask(SIG_BLOCK, &block, NULL))
		;
	else
		success = 1;

	if(!success && errno == 0)
		errno = EINVAL;

	return success;
}

int process_shutting_down(process p)
{
	return p->shutting_down;
}

/**
 * Write the pid to /var/run/appname.pid
 * Return 1 on success, 0 on error.
 */
static int write_pid(process p, pid_t pid)
{
	FILE *f;
	char filename[1024];

	assert(p != NULL);

	/* NOTE: Try to find a portable way of identifying the proper
	 * directory to write this file in. AIX 4.3.3 has no /var/run.
	 */
	snprintf(filename, sizeof filename, "/var/run/%s.pid", c_str(p->appname));

	if( (f = fopen(filename, "w")) == NULL)
		return 0;
	else if(fprintf(f, "%lu", (unsigned long)pid) <= 0) {
		fclose(f);
		return 0;
	}
	else if(fclose(f))
		return 0;
	else
		return 1;
}

static void* shutdown_thread(void* arg)
{
	sigset_t catch;
	int caught;
	process p;
	pid_t my_pid;
	size_t i;

	p = (process)arg;

	/**
	 * Since this is the thread to receive the signal,
	 * we save this threads pid. This is important under Linux,
	 * as each thread has its own pid.
	 */
	my_pid = getpid();
	if(!write_pid(p, my_pid)) 
		warning("Unable to write pid %lu to the pid file.",
			(unsigned long)my_pid);
	sigemptyset(&catch);
	sigaddset(&catch, SIGTERM);

	/* Wait, wait, wait for SIGTERM */
	sigwait(&catch, &caught);
	p->shutting_down = 1;

	/* Shut down all objects we handle */
	for(i = 0; i < p->objects_used; i++) {
		struct srv* s = &p->objects[i];
		s->shutdown_func(s->object);
	}

	return NULL;
}

static int handle_shutdown(process p)
{
	int error;

	/* Block the signals we handle before creating threads */
	if(!set_signals_to_block())
		return 0;

	/* start the shutdown thread */
	if( (error = pthread_create(&p->sdt, NULL, shutdown_thread, p))) {
		errno = error;
		return 0;
	}

	return 1;
}

static void* launcher(void* args)
{
	int exitcode;
	struct srv* s = args;
	exitcode = s->run_func(s->object);
	return (void*)exitcode;
}

/**
 * Handle the thread creation needed to create a thread
 * which starts the actual server.
 */
static int start_one_object(struct srv* s)
{
	assert(s != NULL);

	return pthread_create(&s->tid, NULL, launcher, s);
}

/* Calls the undo() function for all objects for which the do() 
 * function has been called. Used if we encounter
 * an error during start up and want to undo the startup for
 * objects already started.
 * The pointer to the failed object, failed, may be NULL. This 
 * value means that the undo() function should be called for all objects.
 */
static void undo(process p, struct srv* failed)
{
	size_t i;
	struct srv *s;

	for(i = 0; i < p->objects_used; i++) {
		s = &p->objects[i];
		if(s == failed) 
			return;

		if(s->undo_func != NULL)
			s->undo_func(s->object);
	}
}

static void shutdown_started_objects(process p, struct srv* failed)
{
	size_t i;
	struct srv *s;

	for(i = 0; i < p->objects_used; i++) {
		s = &p->objects[i];
		if(s == failed) 
			return;

		/* Shut down the object and wait to get the exitcode */
		s->shutdown_func(s->object);
		pthread_join(s->tid, NULL);
	}
}

/**
 * If we fail after the shutdown thread has started, we have to kill it.
 * That's done here.
 */
static void shutdown_shutdown(process p)
{
	pthread_kill(p->sdt, SIGTERM);
}


int process_start(process p, int fork_and_close)
{
	size_t i;
	struct srv *s;
	int error;

	assert(p != NULL);

	if(fork_and_close) {
		pid_t pid;

		if( (pid = fork()) == -1) 
			return 0;
		else if(pid != 0) 
			exit(EXIT_SUCCESS);

		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
	}

	for(i = 0; i < p->objects_used; i++) {
		s = &p->objects[i];
		if(s->do_func != NULL && !s->do_func(s->object)) {
			undo(p, s);
			return 0;	
		}
	}

	/* Start shutdown thread before we do setuid() or chroot() to
	 * be able to write to /var/run. */
	if(!handle_shutdown(p)) {
		undo(p, NULL);
		return 0;
	}

	/* Set current directory and user id if supplied by the caller */
	if(cstring_length(p->username) > 0) {
		struct passwd* pw;
		errno = 0;
		if( (pw = getpwnam(c_str(p->username))) == NULL) {
			shutdown_shutdown(p);
			undo(p, NULL);

			/* Hmm, either the user didn't exist in /etc/passwd
			 * or we didn't have permission to read it or we ran
			 * out of memory. The Linux man page doesn't mention
			 * any other errno value than ENOMEM, which is why we
			 * set and test errno before return.
			 */
			if(errno == 0) 
				errno = ENOENT;

			return 0;
		}

		/* We must chroot before setuid() to be allowed to chroot. */
		if(cstring_length(p->rootdir) > 0) {
			if(chdir(c_str(p->rootdir)) || chroot(c_str(p->rootdir))) {
				/* Hmm, unable to change directory. */
				shutdown_shutdown(p);
				undo(p, NULL);
				return 0;
			}
		}

		if(setuid(pw->pw_uid)) {
			/* Oops, unable to change user id. This is serious
			 * since the process may continue running as e.g. root.
			 * The safest thing to do is therefore to stop 
			 * the process. 
			 */
			undo(p, NULL);
			shutdown_shutdown(p);
			return 0;
		}
	}
	else {
		/* Don't chroot() twice */
		if(cstring_length(p->rootdir) > 0) {
			if(chdir(c_str(p->rootdir)) || chroot(c_str(p->rootdir))) {
				/* Hmm, unable to change directory. */
				undo(p, NULL);
				shutdown_shutdown(p);
				return 0;
			}
		}
	}

	/* Now start the server objects */
	for(i = 0; i < p->objects_used; i++) {
		s = &p->objects[i];
		if( (error = start_one_object(s))) {
			/* Hmm, we failed to start one object. Do not
			 * start the rest of the objects. Call the shutdown
			 * function for each object to tell it to stop running.
			 */
			shutdown_started_objects(p, s);
			shutdown_shutdown(p);
			return 0;
		}
	}

	/* Success. */
	return 1;
}

int process_wait_for_shutdown(process p)
{
	size_t i;
	int error;

	assert(p != NULL);
	if( (error = pthread_join(p->sdt, NULL))) {
		errno = error;
		return 0;
	}

	/* Now that the shutdown thread has exited, it
	 * is time to wait for the started objects.
	 */
	for(i = 0; i < p->objects_used; i++) {
		struct srv* srv = &p->objects[i];
		/* Note: We need to work around a warning here. 
		 * gcc 4.1.2 type-punned pointers and strict aliasing.
		 * boa 20070105
 		 */
#if 0
		if( (error = pthread_join(srv->tid, (void**)&srv->exitcode))) {
			errno = error;
			return 0;
		}
#else
		{
			void *pfoo = &srv->exitcode;
			if( (error = pthread_join(srv->tid, &pfoo))) {
				errno = error;
				return 0;
			}
		}

#endif
	}

	return 1;
}

int process_get_exitcode(process p, void* object)
{
	size_t i;

	assert(p != NULL);
	assert(object != NULL);

	for(i = 0; i < p->objects_used; i++) {
		struct srv* s = &p->objects[i];
		if(s->object == object)
			return s->exitcode;
	}

	return -1;
}


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
#ifndef CONNECTION_H
#define CONNECTION_H

#include <netinet/in.h> /* for struct sockaddr_in */

#include <meta_membuf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Declaration of our connection ADT */
typedef struct connection_tag* connection;

connection connection_new(int timeout_reads, int timeout_writes, int retries_reads, int retries_writes, void* arg2);
void connection_free(connection conn);
void connection_recycle(connection conn);

void* connection_arg2(connection conn); 

/* connect to a host on a port, return 0 on errors */
int connection_connect(connection c, const char* host, int port);


int connection_putc(connection conn, int ch);
int connection_puts(connection conn, const char *s);
int connection_write(connection conn, const void* buf, size_t cb);
int connection_write_big_buffer(connection conn, const void* buf, size_t cb, int timeout, int retries);
int connection_flush(connection conn);

int connection_read(connection conn, void* buf, size_t cb);
int connection_getc(connection conn, int* pchar);
int connection_gets(connection conn, char *buf, size_t size);
int connection_ungetc(connection conn, int c);

void connection_discard(connection conn); 
int connection_close(connection conn); 

void connection_set_persistent(connection conn, int val);
int  connection_is_persistent(connection conn);

void connection_set_params(connection conn, int fd, struct sockaddr_in* paddr);
struct sockaddr_in* connection_get_addr(connection conn);

int data_on_socket(connection conn);

size_t connection_readbuf_size(connection conn);
size_t connection_writebuf_size(connection conn);

membuf connection_reclaim_read_buffer(connection conn);
membuf connection_reclaim_write_buffer(connection conn);
void   connection_assign_read_buffer(connection conn, membuf buf);
void   connection_assign_write_buffer(connection conn, membuf buf);

void connection_set_fd(connection conn, int fd);
int connection_get_fd(connection conn);

#ifdef __cplusplus
}
#endif

#endif /* guard */


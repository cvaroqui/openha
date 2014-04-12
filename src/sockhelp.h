/*
* Copyright (C) 1999,2000,2001,2002,2003,2004 Samuel Godbillot <sam028@users.sourceforge.net>
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
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef _SOCKHELP_H_
#define _SOCKHELP_H_

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <limits.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <glib.h>

gint atoport( /* char *service, char *proto */ );
struct in_addr *atoaddr( /* char *address */ );
gint get_connection( /* int socket_type, u_short port, int *listener */ );
gint make_connection( /* char *service, int type, char *netaddress */ );
gint sock_read( /* int sockfd, char *buf, size_t count */ );
gint sock_write( /* int sockfd, const char *buf, size_t count */ );
gint sock_gets( /* int sockfd, char *str, size_t count */ );
gint sock_puts( /* int sockfd, const char *str */ );
void ignore_pipe( /*void */ );
void write_wakeup();

#define BUFFER	64
#endif

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
#include <sockhelp.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <config.h>

/* Take a service name, and a service type, and return a port number.  If the
   service name is not found, it tries it as a decimal number.  The number
   returned is byte ordered for the network. */
gint
atoport(gchar * service, gchar * proto)
{
	gint port;
	glong lport;
	struct servent *serv;
	gchar *errpos;

	/* First try to read it from /etc/services */
	serv = getservbyname(service, proto);
	if (serv != NULL)
		port = serv->s_port;
	else {			/* Not in services, maybe a number? */
		lport = strtol(service, &errpos, 0);
		if ((errpos[0] != 0) || (lport < 1) || (lport > 65535))
			return -1;	/* Invalid port address */
		port = htons(lport);
	}
	return port;
}

/* Converts ascii text to in_addr struct.  NULL is returned if the address
   can not be found. */
struct in_addr *
atoaddr(address)
gchar *address;
{
	struct hostent *host;
	static struct in_addr saddr;

	/* First try it as aaa.bbb.ccc.ddd. */
	saddr.s_addr = inet_addr(address);
	if (saddr.s_addr != -1) {
		return &saddr;
	}
	host = gethostbyname(address);
	if (host != NULL) {
		return (struct in_addr *) *host->h_addr_list;
	}
	return NULL;
}

gint
get_connection(socket_type, port, listener)
gint socket_type;
u_short port;
gint *listener;
{
	struct sockaddr_in address;
	gint listening_socket;
	gint connected_socket = -1;
	gint new_process;
	gint reuse_addr = 1;

	memset((char *) &address, 0, sizeof (address));
	address.sin_family = AF_INET;
	address.sin_port = port;
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	listening_socket = socket(AF_INET, socket_type, 0);
	if (listening_socket < 0) {
		perror("socket");
		return -1;
	}

	if (listener != NULL)
		*listener = listening_socket;
	setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR,
		   (const void *) &reuse_addr, sizeof (reuse_addr));

	if (bind(listening_socket, (struct sockaddr *) &address,
		 sizeof (address)) < 0) {
		perror("bind");
		close(listening_socket);
		return -1;
	}

	if (socket_type == SOCK_STREAM) {
		listen(listening_socket, 5);	/* Queue up to five connections before having them automatically rejected. */

		while (connected_socket < 0) {
			connected_socket = accept(listening_socket, NULL, NULL);
			if (connected_socket < 0) {
				if (errno != EINTR) {
					perror("accept");
					close(listening_socket);
					return -1;
				} else {
					continue;	/* don't fork - do the accept again */
				}
			}
			new_process = fork();
			if (new_process < 0) {
				perror("fork");
				close(connected_socket);
				connected_socket = -1;
			} else {	/* We have a new process... */
				if (new_process == 0) {
					/* This is the new process. */
					close(listening_socket);	/* Close our copy of this socket */
					if (listener != NULL)
						*listener = -1;	/* Closed in this process.  We are not responsible for it. */
				} else {
					/* This is the main loop.  Close copy of connected socket, and
					   continue loop. */
					close(connected_socket);
					connected_socket = -1;
				}
			}
		}
		return connected_socket;
	} else
		return listening_socket;
}

int
make_connection(gchar * service, gint type, gchar * netaddress)
{
	/* First convert service from a string, to a number... */
	gint port = -1;
	struct in_addr *addr;
	gint sock, connected;
	struct sockaddr_in address;

	if (type == SOCK_STREAM)
		port = atoport(service, "tcp");
	if (type == SOCK_DGRAM)
		port = atoport(service, "udp");
	if (port == -1) {
		fprintf(stderr, "make_connection:  Invalid socket type.\n");
		return -1;
	}
	addr = atoaddr(netaddress);
	if (addr == NULL) {
		fprintf(stderr, "make_connection:  Invalid network address.\n");
		return -1;
	}

	memset((char *) &address, 0, sizeof (address));
	address.sin_family = AF_INET;
	address.sin_port = (port);
	address.sin_addr.s_addr = addr->s_addr;
	sock = socket(AF_INET, type, 0);

	/*
	 * printf("Connecting to %s on port %d.\n",inet_ntoa(*addr),htons(port));
	 */
	if (type == SOCK_STREAM) {
		connected = connect(sock, (struct sockaddr *) &address,
				    sizeof (address));
		if (connected < 0) {
			perror("connect");
			return -1;
		}
		return sock;
	}
	/* Otherwise, must be for udp, so bind to address. */
	if (bind(sock, (struct sockaddr *) &address, sizeof (address)) < 0) {
		perror("bind");
		return -1;
	}
	return sock;
}

/* This is just like the read() system call, accept that it will make
   sure that all your data goes through the socket. */
gint
sock_read(gint sockfd, gchar * buf, size_t count)
{
	size_t bytes_read = 0;
	gint this_read;

	while (bytes_read < count) {
		do
			this_read = read(sockfd, buf, count - bytes_read);
		while ((this_read < 0) && (errno == EINTR));
		if (this_read < 0)
			return this_read;
		else if (this_read == 0)
			return bytes_read;
		bytes_read += this_read;
		buf += this_read;
	}
	return count;
}

/* This is just like the write() system call, accept that it will
   make sure that all data is transmitted. */
gint
sock_write(gint sockfd, gchar * buf, size_t count)
{
	size_t bytes_sent = 0;
	gint this_write;

	while (bytes_sent < count) {
		do
			this_write = write(sockfd, buf, count - bytes_sent);
		while ((this_write < 0) && (errno == EINTR));
		if (this_write <= 0)
			return this_write;
		bytes_sent += this_write;
		buf += this_write;
	}
	return count;
}

gint
sock_gets(gint sockfd, gchar * str, size_t count)
{
	gint bytes_read;
	gint total_count = 0;
	gchar *current_position;
	gchar last_read = 0;

	current_position = str;
	while (last_read != 10) {
		bytes_read = read(sockfd, &last_read, 1);
		if (bytes_read <= 0) {
			/* The other side may have closed unexpectedly */
			return -1;	/* Is this effective on other platforms than linux? */
		}
		if ((total_count < count) && (last_read != 10)
		    && (last_read != 13)) {
			current_position[0] = last_read;
			current_position++;
			total_count++;
		}
	}
	if (count > 0)
		current_position[0] = 0;
	return total_count;
}

/* This function writes a character string out to a socket.  It will 
   return -1 if the connection is closed while it is trying to write. */
gint
sock_puts(gint sockfd, gchar * str)
{
	return sock_write(sockfd, str, strlen(str));
}

/* This ignores the SIGPIPE signal.  This is usually a good idea, since
   the default behaviour is to terminate the application.  SIGPIPE is
   sent when you try to write to an unconnected socket.  You should
   check your return codes to make sure you catch this error! */
void
ignore_pipe(void)
{
	struct sigaction sig;

	sig.sa_handler = SIG_IGN;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGPIPE, &sig, NULL);
}

void
wakeup(gint TIMEOUT)
{
	/* Set signal to this function. */
	signal(SIGQUIT, wakeup);
	signal(SIGALRM, write_wakeup);
	/* Set alarm clock for "TIMEOUT" seconds. */
	alarm(TIMEOUT);
}

void
write_wakeup()
{
	gint pid;
	pid = getpid();
	kill(pid, SIGHUP);
}

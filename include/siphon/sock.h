#ifndef SIPHON_SOCK_H
#define SIPHON_SOCK_H

#include "common.h"
#include "uri.h"

typedef struct {
	int family; // PF_UNSPEC, PF_INET, PF_INET6
	unsigned send_buffer, receive_buffer;
	uint16_t backlog;
	bool close_exec:1;
	bool reuse_address:1;
	bool reuse_port:1;
	bool non_blocking:1;
	bool no_delay:1;
	bool defer_accept:1;
	bool keep_alive:1;
	bool sig_pipe:1;
} SpSockConfig;

typedef enum {
	SP_ENDPOINT_FD,
	SP_ENDPOINT_IP4,
	SP_ENDPOINT_IP6,
	SP_ENDPOINT_LOCAL,
	SP_ENDPOINT_HOST
} SpEndpointType;

typedef struct {
	union {
		int fd;
		struct sockaddr_in ip4;
		struct sockaddr_in6 ip6;
		struct sockaddr_un local;
		struct sockaddr addr;
		struct {
			char name[256];
			char service[8];
		} host;
	} value;
	SpEndpointType type;
	SpSockConfig config;
} SpEndpoint;

SP_EXPORT const SpSockConfig *SP_SOCK_DEFAULT;

/**
 * @brief   Creates a server socket
 *
 * The endpoint uri should be of the format:
 *     [scheme:]//[host[:port]][path][options]
 *
 * The supported scheme types are:
 * - inet/net/ip: IPv4 or IPv6 (PF_UNSPEC)
 * - inet4/net4/ip4: IPv4 only
 * - inet6/net6/ip6: IPv6 only
 * - local/unix/ipc: local socket
 * - fd: existing file descriptor number
 * - service name (i.e. http, ssh, etc.)
 *
 * Examples:
 * - //0.0.0.0:8080
 * - ip4//localhost:8080
 * - ip6//localhost:8080
 * - local:///tmp/test.sock
 * - fd://8
 * - http://www.imgix.com
 *
 * The URI may be scheme-less for inet endpoints where the host identifies
 * the inet type (IPv4, IPv6). For host names and local sockets, the scheme
 * must be provided.
 *
 * For stream sockets, the socket will be bound and listening. For datagram
 * sockets, the socket will only be bound.
 *
 * Secondarily, the scheme will be assumed to be a service name.
 *
 * @param  uri    endpoint uri
 * @param  stype  socket type (SOCK_STREAM, SOCK_DGRAM)
 * @param  s      bound address if successful
 * @retval valid fd on success, or error code
 */
SP_EXPORT int
sp_listen (const char *uri, int stype, const SpSockConfig *cfg, struct sockaddr_storage *s);

SP_EXPORT int
sp_dial (const char *uri, int stype, const SpSockConfig *cfg, struct sockaddr_storage *s);

SP_EXPORT int
sp_sock_config (int fd, const SpSockConfig *cfg);

SP_EXPORT int
sp_sock_nonblock (int fd, bool on);

SP_EXPORT int
sp_sock_cloexec (int fd, bool on);

SP_EXPORT int
sp_sock_type (int fd);

#endif


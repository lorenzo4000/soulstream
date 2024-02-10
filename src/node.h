#ifndef SS_NODE_H
#define SS_NODE_H

/*
 *	Nodes are low level platform-dependant objects;
 *	Nodes rapresent a general TCP connection;
 */


#include <sys/socket.h>
#include <poll.h>
#include <resolv.h>

#define MAX_NODES 1024
typedef short Node;
extern struct pollfd node_fd[MAX_NODES]; 
extern struct sockaddr_in node_address[MAX_NODES];
extern unsigned int node_number;

void _nodes_init_pools();
Node node_add(struct pollfd, struct sockaddr_in);
Node node_new(struct in_addr, short);
int node_remove(Node);
int node_bind(Node);
int node_connect(Node);
Node node_accept(Node);



#endif // SS_NODE_H

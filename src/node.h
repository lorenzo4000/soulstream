/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#ifndef SS_NODE_H
#define SS_NODE_H

/*
 *	Nodes are low level platform-dependant objects;
 *	Nodes rapresent a general TCP connection;
 */


#include <sys/socket.h>
#include <poll.h>
#include <resolv.h>

#define MAX_NODES 256
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

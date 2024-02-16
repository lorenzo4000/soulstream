/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#include <node.h>

struct pollfd node_fd[MAX_NODES] = {0}; 
struct sockaddr_in node_address[MAX_NODES] = {0};
unsigned int node_number = 0;

void _nodes_init_pools() {
	for(Node i = 0; i < MAX_NODES; i++) {
		node_fd[i].fd = -1;
	}
}

Node node_add(struct pollfd _node_fd, struct sockaddr_in _node_address) {
	if(node_number >= MAX_NODES) {
		return -1;
	}

	for(Node i = 0; i < MAX_NODES; i++) {
		if(node_fd[i].fd < 0) {
			node_address[i] = _node_address;
			node_fd[i] =  _node_fd;

			node_number++;

			return i;
		}
	}
	
	return -1;
}

// address and port are assumed to be in network byte order
Node node_new(struct in_addr ipv4, short port) {
	struct sockaddr_in _node_address = {
		.sin_family = AF_INET,
		.sin_port = port,
		.sin_addr = ipv4
	};

	int _node_fd_fd = socket(AF_INET, SOCK_STREAM, 0); // TCP
	if(_node_fd_fd < 0) {
		perror("node_new: error: could not create new socket: ");
		return -1;
	}	

	struct pollfd _node_fd = {
		_node_fd_fd,
		1,
		0
	};

	return node_add(_node_fd, _node_address);
}

int node_close(Node node) {
	if(node_number <= 0) {
		return -1;
	}

	return close(node_fd[node].fd);
}

int node_remove(Node node) {
	if(node_close(node) < 0) {
		return -1;
	}

	node_fd[node].fd = -1;
	node_number--;

	return 0;
}

int _nodes_close_pools() {
	for(Node n = 0; n < MAX_NODES; n++) {
		node_close(n);
	}
	return 0;
}

int node_bind(Node node) {
	return bind(
		node_fd[node].fd,
		(struct sockaddr*)&node_address[node],
		sizeof(struct sockaddr_in)
	);	
}

int node_connect(Node node) {
	return connect(
		node_fd[node].fd,
		(struct sockaddr*)&node_address[node],
		sizeof(struct sockaddr_in)
	);
}

Node node_accept(Node node) {
	struct sockaddr_in new_address = {0};
	unsigned int new_address_len = 16;
	int new_fd_fd = accept(
		node_fd[node].fd,
		(struct sockaddr*)&new_address,
		&new_address_len
	);
	if(new_fd_fd < 0) {
		perror("node_accept: error: could not accept new node: ");
		return -1;
	}
	
	struct pollfd new_fd = {
		new_fd_fd,
		1,
		0
	};
	
	return node_add(
		new_fd,
		new_address
	);
}

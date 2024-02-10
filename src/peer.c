/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#include <peer.h>

#include <assert.h>
#include <string.h>

Peer peers[MAX_PEERS];
unsigned int peer_number;

void _peers_init_pool() {
	for(size_t i = 0; i < MAX_PEERS; i++) {
		peers[i].node = -1;
		peers[i].connection = CONNECTION_TYPE_N;
	}
}

Peer* peer_add(Peer p) {
	if(peer_number >= MAX_PEERS) {
		return NULL;
	}

	for(size_t i = 0; i < MAX_PEERS; i++) {
		if(peers[i].node < 0) {
			// found spot
			peers[i] = p;
			peer_number++;
			return &peers[i];
		}
	}

	return NULL;
}

int peer_remove(Peer* p) {
	if(node_remove(p->node) < 0) {
		return -1;
	}
	p->node = -1;
	peer_number--;
	return 0;
}

struct pollfd peer_fd(Peer p) {
	return node_fd[p.node];
}

Peer* peer_from_user(char* user) {
	assert(user);
	if(strlen(user) <= 0) {
		return NULL;
	}

	for(size_t i = 0; i < MAX_PEERS; i++) {
		if(!strcmp(peers[i].username, user)) {
			return &peers[i];
		}	
	}
	return NULL;
}

MessageBuffer* peer_send_message(Peer peer, MessageBuffer* buffer) {
	return send_message(buffer, peer_fd(peer).fd);
}

int peer_read_message(Peer peer, MessageBuffer* buffer) {
	return read_message(buffer, peer_fd(peer).fd);
}
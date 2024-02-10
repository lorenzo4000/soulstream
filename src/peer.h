#ifndef SS_PEER_H
#define SS_PEER_H

/*
 *  Peers are protocol-specific connection objects;
 *  A Peer is created by client or server every time a peer connection 
 *  of type `P`, `F` or `D` is enstablished via PeerInit messages;
 */


#include <node.h>
#include <message.h>
#include <connection.h>

#define MAX_PEERS MAX_NODES
#define USERNAME_MAX_LENGTH 30
#define PASSWORD_MAX_LENGTH 50

typedef struct Peer {
	Node node;
	char username[USERNAME_MAX_LENGTH];
	ConnectionType connection;
} Peer;

extern Peer peers[MAX_PEERS];
extern unsigned int peer_number;

void _peers_init_pool();
Peer* peer_add(Peer p);
int peer_remove(Peer* p);
struct pollfd peer_fd(Peer p);
Peer* peer_from_user(char* user);
MessageBuffer* peer_send_message(Peer peer, MessageBuffer* buffer);
int peer_read_message(Peer peer, MessageBuffer* buffer);
#endif // SS_PEER_H

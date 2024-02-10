/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#include <string.h>
#include <assert.h>

#include <node.h>
#include <peer.h>
#include <message.h>
#include <network.h>
#include <password.h>
#include <shell.h>
#include <sys/time.h>
#include <errno.h>

#define RC_FILE "/home/lorenzo/Projects/soulstream/ss.rc"

#define MAINFRAME_NAME "localhost"
#define MAINFRAME_PORT  2240
// #define MAINFRAME_NAME "server.slsknet.org"
// #define MAINFRAME_PORT  2242

// ** global memory
#define INITIAL_MSG_SIZE 2048
MessageBuffer msg;
Node the_mainframe;

// ** configurable stuff
SHELL_VAR(char, [USERNAME_MAX_LENGTH + 1], username, "your soulseek username");
SHELL_VAR(char, [PASSWORD_MAX_LENGTH + 1], password, "yout soulseek password");

int peer_connect(Peer* peer) {
	assert(peer);

	unsigned int token = node_number * 0x69;

	// ** send ConnectToPeer to The Mainframe **
	// printf("sending ConnectToPeer ...\n");
	// 
	// if(strlen(peer->username) > USERNAME_MAX_LENGTH) {
	// 	printf("error: invalid peer username length in peer_connect\n");
	// 	return -1;
	// }
	// 
	// pcode(&msg, SERVER_CONNECT_TO_PEER);
	// p32(&msg, token);
	// pstring(&msg, peer->username, strlen(peer->username));
	// pstring(&msg, (char*)&peer->connection, 1);
	// send_message(&msg, node_fd[the_mainframe].fd);

	// ** send PeerInit to peer **
	printf("sending PeerInit ...\n");
	size_t user_len = strlen(username);
	printf("user_len : %ld\n", user_len);
	printf("username : %s\n", username);

	printf("length of the message before putting stuff into it : %d\n", message_buffer_len(msg));
	pcode(&msg, PEERINIT_PEER_INIT);
	printf("length of the message : %d\n", message_buffer_len(msg));
	pstring(&msg, username, user_len);
	printf("length of the message : %d\n", message_buffer_len(msg));
	pstring(&msg, (char*)&peer->connection, 1);
	printf("length of the message : %d\n", message_buffer_len(msg));
// 	p32(&msg, 0);
	printf("final length of the message : %d\n", message_buffer_len(msg));
	// printf("actual message that we would send : \n");
	// unsigned char b;
	// while(u(&msg, &b)) {
	// 	printf("%02hhx ", b);
	// }
	// putchar('\n');
	peer_send_message(*peer, &msg);

	return 0;
}
 
int get_user_address(char* user, unsigned int* ipv4, unsigned int* port) {
	assert(user);
	assert(node_address);
	
	printf("get user address...\n");
	if(strlen(user) <= 0 || strlen(user) > USERNAME_MAX_LENGTH) {
		printf("error: invalid username length in GetPeerAddress!\n");
		return -1;
	}

	pcode(&msg, SERVER_GET_PEER_ADDRESS);
	pstring(&msg, user, strlen(user));
	send_message(&msg, node_fd[the_mainframe].fd);
	read_message(&msg, node_fd[the_mainframe].fd);

	MessageCode code;
	ucode(&msg, MESSAGE_TYPE_SERVER, &code);
	printf("response code: %u\n", code);

	unsigned int user_len;
	u32(&msg, &user_len);
	if(user_len > USERNAME_MAX_LENGTH) {
		printf("error: invalid username length in GetPeerAddress!\n");
		return -1;
	}
	printf("user_len: %u\n", user_len);

	ustring(&msg, user, user_len);
	for(size_t i = 0; i < user_len; i++) {
		printf("%c", user[i]);
	}
	putchar('\n');
	
	u32(&msg, ipv4);
	printf("user ip: %u\n", *ipv4);

	u32(&msg, port);
	printf("user port: %u\n", *port);

	unsigned char obf;
	u(&msg, &obf);	
	if(obf) {
		printf("info: little guy wants to use obfuscation!\n");
	}

	unsigned int obf_port;
	u32(&msg, &obf_port);	
	printf("obf_port: %u\n", obf_port);

	return 0;
}

#define PEER_CONNECT_TIMEOUT 5 

Peer* user_connect(char* user, ConnectionType connection) {
	assert(user);
	if(strlen(user) > USERNAME_MAX_LENGTH) {
		printf("errror: invalid user length in user_connect\n");
		return NULL;
	}

	printf("user connect...\n");
	
	unsigned int ipv4, port;
	if(get_user_address(user, &ipv4, &port) < 0) {
		printf("error: could not get address of user `%s`\n", user);
		return NULL;
	}

	Node new_node = node_new(
		(struct in_addr) { htonl(ipv4) }, 
		htons(port)
	);

	// ** set the timeout **
	struct timeval timeout = {
		PEER_CONNECT_TIMEOUT,
		0
	};
	
	if (setsockopt(node_fd[new_node].fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        perror("Setting socket timeout failed");
        exit(EXIT_FAILURE);
    }

	if(node_connect(new_node) < 0) {
		if (errno == EINPROGRESS) {
            fprintf(stderr, "Connection timeout\n");
		} else {
			perror("connect");
		}
		return NULL;
	}

	Peer new_peer = {
		.node = new_node,
		.connection = connection
	};
	strcpy(new_peer.username, user);

	if(peer_connect(&new_peer) < 0) {
		printf("error: could not initialize connection with peer\n");
		return NULL;
	}

	return peer_add(new_peer);
}

int interpret_mainframe_message(MessageBuffer* msg) {
	assert(msg);

	MessageCode code = 0;
	if(ucode(msg, MESSAGE_TYPE_SERVER, &code) <= 0) {
		printf("interpret_mainframe_message: error: could not get code from msg\n");
		return -1;
	}

	switch(code) {
		case SERVER_CONNECT_TO_PEER: // TODO
		case SERVER_ROOM_LIST: // TODO
		case SERVER_PING:
		default:
			// log file
			FILE* log_file = fopen("/home/lorenzo/Projects/soulstream/log/log.file", "a");

			fprintf(log_file, " ---- nazi stuff ---- \n");
			unsigned char b;
			while(u(msg, &b)) {
				fprintf(log_file, "%02hhx ", b);
			}
			fprintf(log_file, "\n");

			if(fclose(log_file)) {
				perror("log close");
			}

			// just skip this stuff
			//unsigned char _null;
			//while(u(&msg, _null)) { }
		
	}

	return 0;
}

SHELL_FUNCTION(dump_msg, {}, "stuff") {
	unsigned char b;
	while(u(&msg, &b)) {
		printf("%02hhx ", b);
	}
	putchar('\n');
}


SHELL_FUNCTION(login, {char* user; char* pass;}, "login into the mainframe") {
	if(!args->user || !args->pass) {
		assert(!args->pass);
		assert(!args->user);
		
		// use config stuff
		args->user = username;
		args->pass = password;

		printf("our pass: %s --- our user: %s\n", password, username);
	}

	// ** calculate MD5 of user + pass
	char user_concat[USERNAME_MAX_LENGTH + PASSWORD_MAX_LENGTH + 1] = {0};
	strcpy(user_concat, args->user);
	strcat(user_concat, args->pass);
	
	unsigned char digest[MD5_DIGEST_LENGTH];
	struct MD5Context md5;
	MD5Init(&md5);
	MD5Update(&md5, (const unsigned char*)user_concat, strlen(user_concat));
	MD5Final(digest, &md5);
	char user_concat_hash[MD5_DIGEST_STRING_LENGTH + 1] = {0};
	md5_string_from_bytes(digest, user_concat_hash);
	
	// ** send login message 
	printf("login... \n");
	if(pcode(&msg, SERVER_LOGIN) < 0) {
		printf("error: pcode failed!\n");
	}
	pstring(&msg, args->user, strlen(args->user));
	pstring(&msg, args->pass, strlen(args->pass));
	p32(&msg, 160);
	pstring(&msg, user_concat_hash, strlen(user_concat_hash));
	p32(&msg, 0x11);
	send_message(&msg, node_fd[the_mainframe].fd);
	read_message(&msg, node_fd[the_mainframe].fd);
	unsigned char b;
	while(u(&msg, &b)) {
		printf("%02hhx ", b);
	}
	putchar('\n');
}

SHELL_FUNCTION(set_listen_port, {int port;}, "give port to The Mainframe") {
	printf("setting listen port to %d ...\n", args->port);
	pcode(&msg, SERVER_SET_LISTEN_PORT);
	p32(&msg, args->port);
	p(&msg, 0);
	p32(&msg, 0);
	send_message(&msg, node_fd[the_mainframe].fd);
}

SHELL_FUNCTION(search, {char* str;}, "search file in soulseek network") {
	printf("asking server to search '%s'...\n", args->str);
	pcode(&msg, SERVER_FILE_SEARCH);
	p32(&msg, 0xB00BA);
	pstring(&msg, args->str, strlen(args->str));
	send_message(&msg, node_fd[the_mainframe].fd);
	read_message(&msg, node_fd[the_mainframe].fd);
	unsigned char b;
	while(u(&msg, &b)) {
		printf("%02hhx ", b);
	}
	putchar('\n');
}

SHELL_FUNCTION(download, {char* user; char* file;}, "request file from user") {
	fprintf(stdout, "requesting `%s` of length %ld from `%s` ...\n", args->file, strlen(args->file), args->user);

	Peer* peer = peer_from_user(args->user);
	if(!peer) {
		peer = user_connect(args->user, CONNECTION_TYPE_P);
		assert(peer);
	}

	pcode(&msg, PEER_QUEUE_UPLOAD);
	pstring(&msg, args->file, strlen(args->file));
	peer_send_message(*peer, &msg);
	
	while(1) {}

	pcode(&msg, PEER_PLACE_IN_QUEUE_REQUEST);
	pstring(&msg, args->file, strlen(args->file));
	peer_send_message(*peer, &msg);

	// peer_read_message(*peer, &msg);
	// printf("place in queue response (?) : \n");
	// unsigned char b;
	// while(u(&msg, &b)) {
	// 	printf("%02hhx ", b);
	// }
	// putchar('\n');
}

SHELL_SCOPE(
		// vars
	SHELL_SCOPE_ENTRY(username),
	SHELL_SCOPE_ENTRY(password),
		
		// functions
	SHELL_SCOPE_ENTRY(login),
	SHELL_SCOPE_ENTRY(set_listen_port),
	SHELL_SCOPE_ENTRY(search),
	SHELL_SCOPE_ENTRY(download),
	SHELL_SCOPE_ENTRY(dump_msg)
);

int main() {
	// *** initialize stuff *** 
	msg = new_message_buffer(INITIAL_MSG_SIZE);
	_nodes_init_pools();
	_peers_init_pool();

	// *** connect to the mainframe ***
	// 	** dns query **
	int res_err;
	struct __res_state dns_state;

	res_err = res_ninit(&dns_state);
	if(res_err < 0) {
		printf("error: could not init dns state!\n");
		return -1;
	}

		unsigned char hostaddress[NS_PACKETSZ];
		int res_answer_len = res_nsearch(&dns_state, MAINFRAME_NAME, C_IN, T_A, hostaddress, NS_PACKETSZ);
		if(res_answer_len < 0) {
			printf("error: could not query hostaddress for hostname `%s`!\n", MAINFRAME_NAME);
			return -1;
		}

	res_nclose(&dns_state);

	//  ** connect **
	the_mainframe = node_new(
		ipv4_from_resolve(hostaddress, res_answer_len), 
		htons(MAINFRAME_PORT)
	); 
	if (the_mainframe < 0) {
		printf("error: could not create socket for the Mainframe!\n");
		return -1;
	}

	if(node_connect(the_mainframe) < 0) {
		printf("error: could not connect to the Mainframe!\n");
		return -1;
	}

	// *** main loop ***
	// triggers and stuff
	struct pollfd triggers[] = {
		node_fd[the_mainframe], 
		{
			STDIN_FILENO,
			1,
			0
		}
	};

	shell_terminal_init();
	shell_rc_from_file(RC_FILE);

	while(!shell_terminal_should_close) {
		switch(poll(triggers, sizeof(triggers) / sizeof(struct pollfd), -1)) {
			case -1:
				if(errno != EINTR) {
					perror("poll");
					return -1;
				}	
			case 0:
				break;
			default:
				if(triggers[0].revents) {
					read_message(&msg, node_fd[the_mainframe].fd);
					interpret_mainframe_message(&msg);
				}
		}

		shell_terminal_update();		
	}
	return 0;
}
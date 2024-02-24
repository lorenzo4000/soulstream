/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/
#include <soulstream.h>

#include <node.h>
#include <peer.h>
#include <message.h>
#include <network.h>
#include <password.h>

#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <errno.h>

#define MAINFRAME_NAME "localhost"
#define MAINFRAME_PORT  2240
// #define MAINFRAME_NAME "server.slsknet.org"
// #define MAINFRAME_PORT  2242

// ** global memory
#define INITIAL_MSG_SIZE 2048
static MessageBuffer msg;
static Node the_mainframe;
static Node in_connections_master;

#define MAX_DOWNLOADS_IN_QUEUE 256
static char* download_queue[MAX_DOWNLOADS_IN_QUEUE];

#define SERVER_PORT 2245

static SoulstreamConfig soulstream_config = {0};

int peer_connect(Peer* peer) {
	assert(peer);

	unsigned int token = node_number * 0x69;

	// ** send ConnectToPeer to The Mainframe **
	printf("sending ConnectToPeer ...\n");
	
	if(strlen(peer->username) > USERNAME_MAX_LENGTH) {
		printf("error: invalid peer username length in peer_connect\n");
		return -1;
	}
	
	pcode(&msg, SERVER_CONNECT_TO_PEER);
	p32(&msg, token);
	pstring(&msg, peer->username, strlen(peer->username));
	pstring(&msg, (char*)&peer->connection, 1);
	send_message(&msg, node_fd[the_mainframe].fd);

	// ** send PeerInit to peer **
	printf("sending PeerInit ...\n");
	size_t user_len = strlen(soulstream_config.username);
	printf("user_len : %ld\n", user_len);
	printf("username : %s\n", soulstream_config.username);

	pcode(&msg, PEERINIT_PEER_INIT);
	pstring(&msg, soulstream_config.username, user_len);
	pstring(&msg, (char*)&peer->connection, 1);
 	p32(&msg, 0);
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

int interpret_peer_message(MessageBuffer* msg, Peer* peer) {
	assert(msg);
	assert(peer);

	MessageType message_type;
	message_type = peer->connection == CONNECTION_TYPE_N ? MESSAGE_TYPE_PEERINIT	:
				   peer->connection == CONNECTION_TYPE_P ? MESSAGE_TYPE_PEER	 	: 
				   peer->connection == CONNECTION_TYPE_F ? MESSAGE_TYPE_FILE	 	:
				   peer->connection == CONNECTION_TYPE_D ? MESSAGE_TYPE_DISTRIBUTED : -1;

	printf("message_type: %d\n", message_type);
	
	MessageCode code = 0;
	if(ucode(msg, message_type, &code) <= 0) {
		printf("interpret_peer_message: error: could not get code from msg\n");
		return -1;
	}

	switch(code) {
		case PEERINIT_PEER_INIT:
			// ** username 
			unsigned int node_username_len;
			char node_username[USERNAME_MAX_LENGTH];

			u32(msg, &node_username_len);
			if(node_username_len > USERNAME_MAX_LENGTH) {
				printf("interpret_peer_message: error: invalid username length in PeerInit message!\n");
				return -1;
			}

			ustring(msg, node_username, node_username_len);
			memcpy(peer->username, node_username, node_username_len);
			printf("username: ");
			for(int i = 0; i < node_username_len; i++) {
				putchar(peer->username[i]);
			}
			putchar('\n');

			// ** connection type 
			unsigned char node_connection_type;
			unsigned int node_connection_type_len;

			u32(msg, &node_connection_type_len);
			if(node_connection_type_len != 1) {
				printf("interpret_peer_message: error: invalid connection type in PeerInit message!\n");
				return -1;
			}

			u(msg, &node_connection_type);
			printf("actual connection type: %c\n", node_connection_type);
			peer->connection = (ConnectionType)node_connection_type;

			// ** token (don't know what to do with this tbh)
			unsigned int token = 0;
			u32(msg, &token);
			if(token) {
				printf("interpret_peer_message: info: non-null token in PeerInit message: %u\n", token);
			}

			break;
		case PEER_FILE_SEARCH_RESPONSE:
			// ** decompress 
			if(decompress_message(msg) != Z_OK) {
				printf("interpret_peer_message: error: could not decompress FileSearchResponse message!\n");
				return -1;
			}

			// ** username 
			unsigned int file_username_len;
			char file_username[USERNAME_MAX_LENGTH + 1] = {0};

			u32(msg, &file_username_len);
			if(file_username_len > USERNAME_MAX_LENGTH) {
				printf("interpret_peer_message: error: invalid username length in FileSearchResponse!\n");
				return -1;
			}

			ustring(msg, file_username, file_username_len);
			printf("username: ");
			for(int i = 0; i < file_username_len; i++) {
				putchar(file_username[i]);
			}
			putchar('\n');
			
			// ** token 
			unsigned int file_token = 0;
			u32(msg, &file_token);
			if(token) {
				printf("interpret_peer_message: info: FileSearchResponse token : %u\n", token);
			}
			
			// ** number of results
			unsigned int n = 0;
			u32(msg, &n);
			printf("interpret_peer_message: info: got %d file search results\n", n);

			// ** results
			for(int i = 0; i < n; i++) {
				// * file code
				unsigned char file_code;
				u(msg, &file_code);
				printf("interpret_peer_message: info: file code: %hhu\n", file_code);

				// * file name 
				unsigned int file_name_len;
				u32(msg, &file_name_len);
				char* file_name = malloc(file_name_len);
				assert(file_name);

				ustring(msg, file_name, file_name_len);
				printf("filename: ");
				for(int i = 0; i < file_name_len; i++) {
					putchar(file_name[i]);
				}
				putchar('\n');

				// * file size
				unsigned long long file_size;
				u64(msg, &file_size);
				printf("file size: %llu\n", file_size);

				// * file ext 
				unsigned int file_ext_len;
				u32(msg, &file_ext_len);
				char* file_ext = malloc(file_ext_len);
				assert(file_ext);

				ustring(msg, file_ext, file_ext_len);
				printf("fileext: ");
				for(int i = 0; i < file_ext_len; i++) {
					putchar(file_ext[i]);
				}
				putchar('\n');

				// * number of attributes
				unsigned int na = 0;
				u32(msg, &na);
				printf("interpret_peer_message: info: file %s has %d attributes!\n", file_name, na);

				// * attributes
				for(int j = 0; j < na; j++) {
					unsigned int file_attribute_code;
					u32(msg, &file_attribute_code);
					printf("interpret_peer_message: info: file_attribute code: %hhu\n", file_attribute_code);
					
					unsigned int file_attribute_value;
					u32(msg, &file_attribute_value);
					printf("interpret_peer_message: info: file_attribute value: %hhu\n", file_attribute_value);
					
				}

				free(file_name);
				free(file_ext);
			}
			
			// ** free slot
			unsigned char free_slot;
			u(msg, &free_slot);
			if(free_slot) {
				printf("interpret_peer_message: info: %s has a free download slot!\n", file_username);
			}

			// ** average speed
			unsigned int avg_speed;
			u32(msg, &avg_speed);
			printf("interpret_peer_message: info: %s has an avg speed of %u!\n", file_username, avg_speed);

			// ** queue length
			unsigned int queue_length;
			u32(msg, &queue_length);
			printf("interpret_peer_message: info: %s has a queue length of %u!\n", file_username, queue_length);

			// ** unknown
			unsigned int unk;
			u32(msg, &unk);
			printf("interpret_peer_message: info: %s has an [unknown] of %u!\n", file_username, unk);

			// ** number of private results
			unsigned int np = 0;
			u32(msg, &np);
			printf("interpret_peer_message: info: got %d private file search results\n", np);

			// ** private results
			for(int i = 0; i < np; i++) { 
				// * file code
				unsigned char file_code;
				u(msg, &file_code);
				printf("interpret_peer_message: info: file code: %hhu\n", file_code);

				// * file name 
				unsigned int file_name_len;
				u32(msg, &file_name_len);
				char* file_name = malloc(file_name_len);
				assert(file_name);

				ustring(msg, file_name, file_name_len);
				printf("filename: ");
				for(int i = 0; i < file_name_len; i++) {
					putchar(file_name[i]);
				}
				putchar('\n');

				// * file size
				unsigned long long file_size;
				u64(msg, &file_size);
				printf("file size: %llu\n", file_size);

				// * file ext 
				unsigned int file_ext_len;
				u32(msg, &file_ext_len);
				char* file_ext = malloc(file_ext_len);
				assert(file_ext);

				ustring(msg, file_ext, file_ext_len);
				printf("fileext: ");
				for(int i = 0; i < file_ext_len; i++) {
					putchar(file_ext[i]);
				}
				putchar('\n');

				// * number of attributes
				unsigned int na = 0;
				u32(msg, &na);
				printf("interpret_peer_message: info: file %s has %d attributes!\n", file_name, na);

				// * attributes
				for(int j = 0; j < na; j++) {
					unsigned int file_attribute_code;
					u32(msg, &file_attribute_code);
					printf("interpret_peer_message: info: file_attribute code: %hhu\n", file_attribute_code);
					
					unsigned int file_attribute_value;
					u32(msg, &file_attribute_value);
					printf("interpret_peer_message: info: file_attribute value: %hhu\n", file_attribute_value);
					
				}

				free(file_name);
				free(file_ext);
			}
			
			break;

		case PEER_TRANSFER_REQUEST: 
			unsigned int direction = 0;
			unsigned int download_token = 0;
			unsigned int filename_length = 0;
			char *filename = NULL;
			unsigned long long filesize = 0;
			u32(msg, &direction);
			u32(msg, &download_token);
			u32(msg, &filename_length);
			if(!(filename = malloc(filename_length))) {
				perror("malloc");
				exit(1);
			}
			ustring(msg, filename, filename_length);
			if(direction) {
				u64(msg, &filesize);
			}

			printf("Got a TransferRequest for file `%s` of size `%lld`!\n", filename, filesize);

			for(int i = 0; i < MAX_DOWNLOADS_IN_QUEUE; i++) {
				if(download_queue[i] && !strcmp(filename, download_queue[i])) {
					printf("The TransferRequest is for a file that we requested so send positive TransferResponse!");

					pcode(msg, PEER_UPLOAD_RESPONSE);
					p32(msg, download_token);
					p(msg, 1);
					peer_send_message(*peer, msg);
				}
			}


			free(filename);
			break;
	}
	
	 
	return 0;
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
			// just skip this stuff
			unsigned char _null;
			while(u(msg, &_null)) { }
		
	}

	return 0;
}

int ss_set_listen_port(int port) {
	printf("setting listen port to %d ...\n", port);
	pcode(&msg, SERVER_SET_LISTEN_PORT);
	p32(&msg, port);
	p(&msg, 0);
	p32(&msg, 0);
	send_message(&msg, node_fd[the_mainframe].fd);

	return 0;
}

int ss_login() {
	char* user = soulstream_config.username;
	char* pass = soulstream_config.password;

	printf("login: our pass: %s --- our user: %s\n", pass, user);
	
	// ** calculate MD5 of user + pass
	char user_concat[USERNAME_MAX_LENGTH + PASSWORD_MAX_LENGTH + 1] = {0};
	strcpy(user_concat, user);
	strcat(user_concat, pass);
	
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
	pstring(&msg, user, strlen(user));
	pstring(&msg, pass, strlen(pass));
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
	
	return 0;
}

int ss_search(char* str) {
	printf("asking server to search '%s'...\n", str);
	pcode(&msg, SERVER_FILE_SEARCH);
	p32(&msg, 0xB00BA);
	pstring(&msg, str, strlen(str));
	send_message(&msg, node_fd[the_mainframe].fd);
	read_message(&msg, node_fd[the_mainframe].fd);
	unsigned char b;
	while(u(&msg, &b)) {
		printf("%02hhx ", b);
	}
	putchar('\n');

	return 0;
}

int ss_download(char* user, char* file) {
	printf("requesting `%s` of length %ld from `%s` ...\n", file, strlen(file), user);

	Peer* peer = peer_from_user(user);
	if(!peer) {
		peer = user_connect(user, CONNECTION_TYPE_P);
		assert(peer);
	}

	pcode(&msg, PEER_QUEUE_UPLOAD);
	pstring(&msg, file, strlen(file));
	peer_send_message(*peer, &msg);
	
	// add file to download queue
	for(int i = 0; i < MAX_DOWNLOADS_IN_QUEUE; i++) {
		if(!download_queue[i]) {
			// found empty spot
			download_queue[i] = file;

			return 0;
		}
	}

	return -1;
}

int ss_init(SoulstreamConfig conf) {
	soulstream_config = conf;

	// *** initialize stuff *** 
	msg = new_message_buffer(INITIAL_MSG_SIZE);
	_nodes_init_pools();
	_peers_init_pool();

	//
	//     ** OUT **
	
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


	//
	//     ** IN **
	
	// *** create master node ***
	in_connections_master = node_new(
		(struct in_addr) {INADDR_ANY},
		htons(SERVER_PORT)
	);
    if(in_connections_master < 0) {  
        printf("error: could not create master node!\n");
       	return -1;
    }  

	if(node_bind(in_connections_master) < 0) {
        printf("error: could not bind master connection socket to master_address! maybe another process is using port %d ?\n", SERVER_PORT);
       	return -1;
    }  

	// *** start listening ***
	if(listen(node_fd[in_connections_master].fd, MAX_NODES) < 0) {
        printf("error: could not start listening for connections on master socket!\n");
       	return -1;
    }  

	if(ss_login() < 0) {
		printf("error: could not login!\n");
		return -1;
	}

	if(ss_set_listen_port(SERVER_PORT) < 0) {
		printf("error: could not set listening port!\n");
		return -1;
	}

	return 0;
}

// this a blocking function
int ss_update() {
	switch(poll(node_fd, sizeof(node_fd) / sizeof(struct pollfd), -1)) {
		case -1:
			if(errno != EINTR) {
				perror("poll");
				return -1;
			}	
		case 0:
			break;
		default:
			if(node_fd[the_mainframe].revents) {
				read_message(&msg, node_fd[the_mainframe].fd);
				interpret_mainframe_message(&msg);
			}

			if(node_fd[in_connections_master].revents) { 
				// ** master activity means new IN connection
				if(node_number >= MAX_NODES || peer_number >= MAX_PEERS) {
					printf("error: connection limit reached!\n");
					return -1;
				}

				Node new_client = node_accept(in_connections_master);
				if(new_client < 0) {
					printf("error: could not accept new client connection\n");
					return -1;
				}

				if(!peer_add((Peer){.node = new_client, .connection = CONNECTION_TYPE_N})) {
					printf("error: could not create new peer!\n");
				}

				printf("info: new connection accepted! socket fd: %d , ip: %x , port: %d\n", 
				   node_fd[new_client].fd, node_address[new_client].sin_addr.s_addr, ntohs(node_address[new_client].sin_port));  
			}

			for(int i = 0; i < MAX_PEERS; i++) {
				// ** peer activity means IO operations
				if(peer_fd(peers[i]).revents) {
					switch(read_message(&msg, peer_fd(peers[i]).fd)) {
						case -1:
							printf("error: could not read peer message\n");
							break;
						case  0:
							// means connection closed
							if(close(peer_fd(peers[i]).fd) < 0) {
								perror("error: could not close client socket : ");
								return -1;
							}

							printf("successfully closed client socket %d!\n", peer_fd(peers[i]).fd);

							peer_remove(&peers[i]);
							break;
						default:
							// print the cute message ^.^
							printf("info: received something from people!! POG \n");
							
							if(interpret_peer_message(&msg, &peers[i]) < 0) {
								printf("error: invalid message\n");
							};

							break;
					}
				}
			}
	}

	return 0;
}

int ss_close() {
	_nodes_close_pools();

	// ...
	
	return 0;
}

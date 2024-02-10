#include <sys/socket.h>
#include <resolv.h>
#include <poll.h>
#include <unistd.h>
#include <assert.h>

#include <node.h>
#include <peer.h>
#include <message.h>
#include <connection.h>
#include <shell.h>


// ** global memory
#define INITIAL_MSG_SIZE 2048
MessageBuffer msg;

#define SERVER_PORT 2245

SHELL_SCOPE();

int interpret_message(MessageBuffer* msg, Peer* peer) {
	assert(msg);
	assert(peer);

	MessageType message_type;
	message_type = peer->connection == CONNECTION_TYPE_N ? MESSAGE_TYPE_PEERINIT	 :
				   peer->connection == CONNECTION_TYPE_P ? MESSAGE_TYPE_PEER	 	 : 
				   peer->connection == CONNECTION_TYPE_F ?	MESSAGE_TYPE_FILE	 	 :
				   peer->connection == CONNECTION_TYPE_D ? MESSAGE_TYPE_DISTRIBUTED : -1;

	printf("message_type: %d\n", message_type);
	
	MessageCode code = 0;
	if(ucode(msg, message_type, &code) <= 0) {
		printf("interpret_message: error: could not get code from msg\n");
		return -1;
	}

	switch(code) {
		case PEERINIT_PEER_INIT:
			// ** username 
			unsigned int node_username_len;
			char node_username[USERNAME_MAX_LENGTH];

			u32(msg, &node_username_len);
			if(node_username_len > USERNAME_MAX_LENGTH) {
				printf("interpret_message: error: invalid username length in PeerInit message!\n");
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
				printf("interpret_message: error: invalid connection type in PeerInit message!\n");
				return -1;
			}

			u(msg, &node_connection_type);
			printf("actual connection type: %c\n", node_connection_type);
			peer->connection = (ConnectionType)node_connection_type;

			// ** token (don't know what to do with this tbh)
			unsigned int token = 0;
			u32(msg, &token);
			if(token) {
				printf("interpret_message: info: non-null token in PeerInit message: %u\n", token);
			}

			break;
		case PEER_FILE_SEARCH_RESPONSE:
			// ** decompress 
			if(decompress_message(msg) != Z_OK) {
				printf("interpret_message: error: could not decompress FileSearchResponse message!\n");
				return -1;
			}

			// ** username 
			unsigned int file_username_len;
			char file_username[USERNAME_MAX_LENGTH + 1] = {0};

			u32(msg, &file_username_len);
			if(file_username_len > USERNAME_MAX_LENGTH) {
				printf("interpret_message: error: invalid username length in FileSearchResponse!\n");
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
				printf("interpret_message: info: FileSearchResponse token : %u\n", token);
			}
			
			// ** number of results
			unsigned int n = 0;
			u32(msg, &n);
			printf("interpret_message: info: got %d file search results\n", n);

			// ** results
			for(int i = 0; i < n; i++) {
				// * file code
				unsigned char file_code;
				u(msg, &file_code);
				printf("interpret_message: info: file code: %hhu\n", file_code);

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
				printf("interpret_message: info: file %s has %d attributes!\n", file_name, na);

				// * attributes
				for(int j = 0; j < na; j++) {
					unsigned int file_attribute_code;
					u32(msg, &file_attribute_code);
					printf("interpret_message: info: file_attribute code: %hhu\n", file_attribute_code);
					
					unsigned int file_attribute_value;
					u32(msg, &file_attribute_value);
					printf("interpret_message: info: file_attribute value: %hhu\n", file_attribute_value);
					
				}

				free(file_name);
				free(file_ext);
			}
			
			// ** free slot
			unsigned char free_slot;
			u(msg, &free_slot);
			if(free_slot) {
				printf("interpret_message: info: %s has a free download slot!\n", file_username);
			}

			// ** average speed
			unsigned int avg_speed;
			u32(msg, &avg_speed);
			printf("interpret_message: info: %s has an avg speed of %u!\n", file_username, avg_speed);

			// ** queue length
			unsigned int queue_length;
			u32(msg, &queue_length);
			printf("interpret_message: info: %s has a queue length of %u!\n", file_username, queue_length);

			// ** unknown
			unsigned int unk;
			u32(msg, &unk);
			printf("interpret_message: info: %s has an [unknown] of %u!\n", file_username, unk);

			// ** number of private results
			unsigned int np = 0;
			u32(msg, &np);
			printf("interpret_message: info: got %d private file search results\n", np);

			// ** private results
			for(int i = 0; i < np; i++) { 
				// * file code
				unsigned char file_code;
				u(msg, &file_code);
				printf("interpret_message: info: file code: %hhu\n", file_code);

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
				printf("interpret_message: info: file %s has %d attributes!\n", file_name, na);

				// * attributes
				for(int j = 0; j < na; j++) {
					unsigned int file_attribute_code;
					u32(msg, &file_attribute_code);
					printf("interpret_message: info: file_attribute code: %hhu\n", file_attribute_code);
					
					unsigned int file_attribute_value;
					u32(msg, &file_attribute_value);
					printf("interpret_message: info: file_attribute value: %hhu\n", file_attribute_value);
					
				}

				free(file_name);
				free(file_ext);
			}
			
			break;
	}
	
	 
	return 0;
}

int main() {
	// *** initialize stuff ***
	msg = new_message_buffer(INITIAL_MSG_SIZE);
	_nodes_init_pools();
	_peers_init_pool();

	// *** create master node ***
	Node master = node_new(
		(struct in_addr) {INADDR_ANY},
		htons(SERVER_PORT)
	);
    if(master < 0) {  
        printf("error: could not create master node!\n");
       	return -1;
    }  

	if(node_bind(master) < 0) {
        printf("error: could not bind master connection socket to master_address! maybe another process is using port %d ?\n", SERVER_PORT);
       	return -1;
    }  

	// *** start listening ***
	if(listen(node_fd[master].fd, MAX_NODES) < 0) {
        printf("error: could not start listening for connections on master socket!\n");
       	return -1;
    }  
	
	// *** main loop ***
	while(1) {
		// ** wait indefinetly for incoming connection
		printf("server: waiting for incoming connection...\n");
		int n = poll(node_fd, MAX_NODES, -1);
		if(n <= 0) {
			printf("error: connection poll error!\n");
		}	

		// ** master socket means new connection
		if(node_fd[master].revents) {
			if(node_number >= MAX_NODES || peer_number >= MAX_PEERS) {
				printf("error: connection limit reached!\n");
				return -1;
			}

			Node new_client = node_accept(master);
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
			

		// ** client sockets mean IO operations
		for(int i = 0; i < MAX_PEERS; i++) {
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
						
						if(interpret_message(&msg, &peers[i]) < 0) {
							printf("error: invalid message\n");
							break;
						};
				}
			}
		}
	}

	return -1;
}

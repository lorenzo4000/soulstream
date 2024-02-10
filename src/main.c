// 		#include <stdio.h>
// 		#include <stdlib.h>
// 		#include <sys/socket.h>
// 		#include <resolv.h>
// 		#include <string.h>
// 		#include <sys/types.h>
// 		#include <md5.h>
// 		
// 		#include <message.h>
// 		#include <server.h>
// 		#include <client.h>

//	int main() {
//		// dns query
//		int res_err;
//		struct __res_state dns_state;
//	
//		res_err = res_ninit(&dns_state);
//		if(res_err < 0) {
//			printf("error: could not init dns state!\n");
//			return -1;
//		}
//	
//			unsigned char hostaddress[NS_PACKETSZ];
//			int res_answer_len = res_nsearch(&dns_state, HOST_NAME, C_IN, T_A, hostaddress, NS_PACKETSZ);
//			if(res_answer_len < 0) {
//				printf("error: could not query hostaddress for hostname `%s`!\n", HOST_NAME);
//				return -1;
//			}
//	
//		res_nclose(&dns_state);
//	
//		Client the_mainframe;
//		if(client_init(
//			&the_mainframe, 
//			ipv4_from_resolve(hostaddress, res_answer_len), 
//			htons(HOST_PORT)) < 0
//		) {
//			printf("error: could not create socket for the Mainframe!\n");
//		}
//		if(client_connect(&the_mainframe) < 0) {
//			printf("error: could not connect to the Mainframe!\n");
//			return -1;
//		}
//	
//		MessageBuffer message_buffer = {0};
//		char* user = "soulstream";
//		char* pass = "password";
//	
//		// hash digest of user + pass
//		char* user_concat = malloc(strlen(user) + strlen(pass) + 1); // ( + NULL-terminator )
//		strcpy(user_concat, user);
//		strcat(user_concat, pass);
//		
//		unsigned char digest[MD5_DIGEST_LENGTH];
//		struct MD5Context md5;
//		MD5Init(&md5);
//		MD5Update(&md5, (const unsigned char*)user_concat, strlen(user_concat));
//		MD5Final(digest, &md5);
//		char user_concat_hash[MD5_DIGEST_STRING_LENGTH + 1];// ( + NULL-terminator )
//		md5_string_from_bytes(digest, user_concat_hash);
//	
//		printf("hash: %s\n", user_concat_hash);
//	
//		char* _uch = user_concat_hash;
//		
//		printf("login... \n");
//		pcode(&message_buffer, SERVER_LOGIN);
//		pstring(&message_buffer, user, strlen(user));
//		pstring(&message_buffer, pass, strlen(pass));
//		p32(&message_buffer, 160);
//		pstring(&message_buffer, _uch, strlen(_uch));
//		p32(&message_buffer, 0x11);
//		send_message(&message_buffer, the_mainframe.fd);
//	 
//		int _p = fork();
//		if(_p == 0) {
//			printf("info: starting server!\n");
//			Server my_server = {0};
//			server_init(&my_server);
//			server_start(&my_server);
//		} else 
//		if(_p > 0) {
//			printf("info: server started!\n");
//		} else {
//			perror("fork error: ");
//		}
//	
//		// i. uint32 port
//		// ii. bool use obfuscation
//		// iii. uint32 obfuscated port
//		printf("set listen port... \n");
//		message_buffer.len = 0;
//		pcode(&message_buffer, SERVER_SET_LISTEN_PORT);
//		p32(&message_buffer, SERVER_PORT);
//		p(&message_buffer, 0);
//		p32(&message_buffer, SERVER_PORT);
//		send_message(&message_buffer, the_mainframe.fd);
//		
//		#define TEST_SEARCH "caparezza exuvia"
//		printf("file search... \n");
//		message_buffer.len = 0;
//		pcode(&message_buffer, SERVER_FILE_SEARCH);
//		p32(&message_buffer, 69);
//		pstring(&message_buffer, TEST_SEARCH, strlen(TEST_SEARCH));
//		send_message(&message_buffer, the_mainframe.fd);
//	
//		while(1) {
//	
//		}
//	}

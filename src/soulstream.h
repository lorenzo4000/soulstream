#ifndef SS_H
#define SS_H

#include <peer.h>

typedef struct SoulstreamConfig {
	char username[USERNAME_MAX_LENGTH];
	char password[PASSWORD_MAX_LENGTH];
} SoulstreamConfig;

int ss_init(SoulstreamConfig);
int ss_update();
int ss_close();

int ss_login();
int ss_set_listen_port(int);
int ss_search(char*);
int ss_download(char*, char*);


#endif // SS_H

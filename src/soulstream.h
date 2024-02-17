#ifndef SS_H
#define SS_H

#include <peer.h>

typedef struct SoulstreamConfig {
	char username[USERNAME_MAX_LENGTH];
	char password[PASSWORD_MAX_LENGTH];
} SoulstreamConfig;

int soulstream_init(SoulstreamConfig);
int soulstream_update();
int soulstream_close();

int login();
int set_listen_port(int);
int search(char*);
int download(char*, char*);


#endif // SS_H

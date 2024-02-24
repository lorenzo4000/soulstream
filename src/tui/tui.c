/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#include <shell.h>
#include <soulstream.h>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


static const SoulstreamConfig SS_CONFIG = {
	.username = "soulstream",
	.password = "password"
};

SHELL_FUNCTION(search, {char* str;}, "search file in soulseek network") {
	ss_search(args->str);
}

SHELL_FUNCTION(download, {char* user; char* file;}, "request file from user") {
	ss_download(args->user, args->file);
}


SHELL_SCOPE(
	// functions
	SHELL_SCOPE_ENTRY(search),
	SHELL_SCOPE_ENTRY(download),
);

void* main_shell(void* args) {
	int shell_log = open("./shell.log", O_RDWR | O_CREAT | O_APPEND, 00700);
	if(shell_log < 0) {
		perror("open");
		return NULL;
	}

	if(shell_terminal_init(shell_log) < 0) {
		printf("error: could not initialize shell!\n");
		return NULL;
	}

	while(!shell_terminal_should_close) {
		// ...
		shell_terminal_update();		
	}

	if(close(shell_log) < 0) {
		perror("close");
	}

	return NULL;
}

void* main_engine(void* args) {
	while(1) {
		ss_update();
	}

	return NULL;
}

int main() {
	ss_init(SS_CONFIG);

	pthread_t engine_thread;
	pthread_t shell_thread;

	// create threads
	if(pthread_create(&engine_thread, NULL, main_engine, NULL) ||
	   pthread_create(&shell_thread, NULL, main_shell, NULL) ){
		perror("pthread_create");
		return -1;
	}

	// wait for shell to finish
	if(pthread_join(shell_thread, NULL) ){
		perror("pthread_create");
		return -1;
	}

	// brutally kill engine thread
	pthread_kill(engine_thread, SIGKILL); 

	ss_close();

	return 0;
}

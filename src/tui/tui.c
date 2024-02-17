/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/


#include <shell.h>

#define RC_FILE "/home/lorenzo/.ss.rc"

// ** configurable stuff
SHELL_VAR(char, [USERNAME_MAX_LENGTH + 1], username, "your soulseek username");
SHELL_VAR(char, [PASSWORD_MAX_LENGTH + 1], password, "yout soulseek password");

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

SHELL_FUNCTION(dump_msg, {}, "stuff") {
	unsigned char b;
	while(u(&msg, &b)) {
		printf("%02hhx ", b);
	}
	putchar('\n');
}


SHELL_FUNCTION(login, {char* user; char* pass;}, "login into the mainframe") {
}

SHELL_FUNCTION(set_listen_port, {int port;}, "give port to The Mainframe") {
}

SHELL_FUNCTION(search, {char* str;}, "search file in soulseek network") {
}

SHELL_FUNCTION(download, {char* user; char* file;}, "request file from user") {
}


int main() {
	shell_terminal_init();
	shell_rc_from_file(RC_FILE);

	while(!shell_terminal_should_close) {
		// ...
		shell_terminal_update();		
	}
}

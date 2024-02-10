/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#ifndef SS_SHELL_H
#define SS_SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>

#define MAX_TOKEN_LENGTH 50
#define EXPRESSION_MAX_TOKENS 20
#define MAX_EXPRESSION_LENGTH  EXPRESSION_MAX_TOKENS * MAX_TOKEN_LENGTH + 64

typedef struct ShellObject {
    char op[MAX_TOKEN_LENGTH];

	// function
    void (*action)();

	// variable
    void* value;
	size_t size;

	char* _help;
} ShellObject;




//
// *** shell function ***
//
#define SHELL_FUNCTION_DECL(x, p) 					   	   \
	struct __attribute__((packed)) __shell_##x##_params	p; \
	extern ShellObject __shell_##x##_obj;		   \
	void x (struct __shell_##x##_params* args); 			   \

#define SHELL_FUNCTION_DEF(x, p, h)   \
	ShellObject __shell_##x##_obj = {			   \
		.op = #x,					   		 			   \
		.action = x,					   	 			   \
		._help = #x #p " : " #h			   				   \
	}; 													   \
	void x (struct __shell_##x##_params* args) 

#define SHELL_FUNCTION(x, p, h)   \
	SHELL_FUNCTION_DECL(x, p); \
	SHELL_FUNCTION_DEF(x, p, h)		  
	
//
// *** shell variable  ***
//
  // `a` is the array thing. like [20] 
#define SHELL_VAR_DECL(t, a, x) 					   	   \
	extern ShellObject __shell_##x##_obj;		   	   \
	extern t x a;										   \

#define SHELL_VAR_DEF(t, a, x, h)							   \
	ShellObject __shell_##x##_obj = {					   \
		.op = #x,					   		 			   \
		.value = &x,					   	 			   \
		.size  = sizeof(x),					   	 		   \
		._help = #x " : " #h			   				   \
	}; 													   \
	t x a;												   \

#define SHELL_VAR(t, a, x, h)   \
	SHELL_VAR_DECL(t, a, x); \
	SHELL_VAR_DEF(t, a, x, h);





typedef ShellObject* ShellScopeEntry;
#define SHELL_SCOPE_ENTRY(x) 	  \
	&__shell_##x##_obj

typedef enum ShellTokenType {
	TOKEN_OP,
	TOKEN_STRING_LITERAL,
	TOKEN_INT_LITERAL,

	/* Character Tokens */
	TOKEN_COMMA = ',',
	TOKEN_SEMICOLON = ';',
	TOKEN_EXCLAMATION_POINT = '!'
} ShellTokenType;
#define CHAR_IS_SHELL_TOKEN(c) ( \
	c == TOKEN_COMMA     ||	  	  \
	c == TOKEN_SEMICOLON ||   	  \
	c == TOKEN_EXCLAMATION_POINT  \
)

typedef struct ShellToken {
	ShellTokenType type;
	union {
		char string_value[MAX_TOKEN_LENGTH];
		long long int int_value;
	};
} ShellToken;

typedef enum ShellCommandType {
	COMMAND_CALL,
	COMMAND_SET
} ShellCommandType;	
#define SHELL_COMMAND_MAX_ARGSIZE 1024
typedef struct ShellCommand {
	ShellCommandType type;
	ShellScopeEntry decl;
	unsigned char args[SHELL_COMMAND_MAX_ARGSIZE];
} ShellCommand;

SHELL_FUNCTION_DECL(help, {});
SHELL_FUNCTION_DECL(quit, {});

#define SHELL_SCOPE(...)			    		      \
	ShellScopeEntry _shell_scope__[] = {        	  	  \
		/*   built-in commands	*/	    		  	  \
		SHELL_SCOPE_ENTRY(help),	  \
		SHELL_SCOPE_ENTRY(quit),	  \
									    		      \
		/*   user defined commands   */ 		      \
		__VA_ARGS__					    		      \
	};								    		      \
	size_t _shell_scope_size__ = sizeof(_shell_scope__) / sizeof(ShellScopeEntry);


ShellToken shell_lex_token(char*, size_t);
int shell_lex_expression(char*, size_t, ShellToken[EXPRESSION_MAX_TOKENS]);
int shell_parse_expression(ShellToken[EXPRESSION_MAX_TOKENS], size_t, ShellCommand*);
pid_t shell_execute_command(ShellCommand, int[2]);
pid_t shell_interpret_expression(char*, size_t, int[2]);

// *** terminal ***
#define SHELL_TERMINAL_UI " ~> "
#define SHELL_TERMINAL_FORMAT SHELL_TERMINAL_UI "%s"
#define SHELL_TERMINAL_OFFSET strlen(SHELL_TERMINAL_UI)

extern int shell_terminal_should_close;
void shell_terminal_clear_expression();
void shell_terminal_print_expression();
void shell_terminal_close();
void shell_terminal_init();
void shell_terminal_update();
int shell_rc(char*, size_t);
int shell_rc_from_file(const char*);



#endif // SS_SHELL_H
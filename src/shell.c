#include <shell.h>

#include <assert.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

extern ShellScopeEntry _shell_scope__[];
extern size_t _shell_scope_size__;

SHELL_FUNCTION_DEF(help, {}, "print this help") {
	size_t n = _shell_scope_size__;
								
    for(int i = 0; i < n; i++) {
		printf("%s\n", _shell_scope__[i]->_help);
	}
}

#define shell_log(fmt, ...) {	\
	if(stdscr) {				\
		printw( fmt ,##__VA_ARGS__ );      \
		refresh();				\
	} else {                    \
		printf( fmt ,##__VA_ARGS__ );      \
	}                           \
}

ShellToken shell_lex_token(char* token_str, size_t token_len) {
	assert(token_str);
	assert(token_len);
	assert(strlen(token_str) == token_len); // TODO: get rid of this assumption

	ShellToken ret = {0};
	if(strlen(token_str) == 1 && CHAR_IS_SHELL_TOKEN(token_str[0])) {
		ret.type = token_str[0];
		return ret;
	}
	switch(token_str[0]) {
		case '\"':
		case '\'':
			// Skip first character and discard last character
			ret.type = TOKEN_STRING_LITERAL;
			for(int i = 1; i < (int)token_len - 1; i++) {
				ret.string_value[i - 1] = token_str[i];
			}

			break;
		default:
			// TODO: if token_str[0] == '0':
			// 			if token_str[1] == 'x':	"%llx" ; // HEX
			//          if token_str[1] >= 48 && token_str[1] <= 57) : "%llo"; // OCTAL
			
			if(token_str[0] >= 48 && token_str[0] <= 57) {
				ret.type = TOKEN_INT_LITERAL;
				sscanf(token_str, "%lld", &ret.int_value);
				break;
			} 
			ret.type = TOKEN_OP;
			for(int i = 0; i < (int)token_len; i++) {
				ret.string_value[i] = token_str[i];
			}
	}

	return ret;
}

// on success returns number of tokens lexxed
int shell_lex_expression(char* expression, size_t expression_len, ShellToken tokens[EXPRESSION_MAX_TOKENS]) {
	int index = 0;
	int token_index = 0;
	while(index < expression_len) {
		if(token_index >= EXPRESSION_MAX_TOKENS) {
			shell_log("lexical warning: expression exceedes maximum number of tokens (%d), skipping...\n", EXPRESSION_MAX_TOKENS);
			break;
		}

		char next_token_str[MAX_TOKEN_LENGTH] = {0};
		while(index < expression_len && 
			  expression[index] != ' ' && 
			  expression[index] != '\t' && 
			  expression[index] != '\n' &&
			  expression[index] != '\"' &&
			  expression[index] != '\''
		) {
			strncat(next_token_str, &expression[index], 1);
			if(CHAR_IS_SHELL_TOKEN(expression[index])) {
				break;
			}
			if(index + 1 < expression_len) {
				if(CHAR_IS_SHELL_TOKEN(expression[index + 1])) {
					shell_log("next is character (%c)\n", expression[index + 1]);
					break;
				}
			}
			index++;
		}

		if(index < expression_len && expression[index] == '\"') {
			strncat(next_token_str, &expression[index], 1); // open literal
			index++;

			while(index < expression_len && expression[index] != '\"') {
				strncat(next_token_str, &expression[index], 1);
				index++;
			}

			if(index >= expression_len) {
				shell_log("lexical error: expected closing `\"` before end of expression\n");
			} else {
				strncat(next_token_str, &expression[index], 1); // close literal
			}
		}

		if(strlen(next_token_str)) {
			tokens[token_index] = shell_lex_token(next_token_str, strlen(next_token_str));
			token_index++;
		}

		index++;
	}

	return token_index;
}

int shell_parse_call(ShellToken tokens[EXPRESSION_MAX_TOKENS], size_t n, ShellCommand* cmd) {
	assert(n);
	assert(cmd);

	assert(tokens[0].type == TOKEN_OP);
	cmd->type = COMMAND_CALL;

	char* op = tokens[0].string_value;
	// ** search for OP in shell scope **
    for(int i = 0; i < _shell_scope_size__; i++) {
		if(!strcmp(op, _shell_scope__[i]->op)) {
			cmd->decl = _shell_scope__[i];

			assert(cmd->decl->action);
			assert(!cmd->decl->value);
			
			// **  iterate through args  **	
			size_t j = 1;
			size_t args_index = 0;
			while(j < n) {
				if(args_index >= SHELL_COMMAND_MAX_ARGSIZE) {
					shell_log("parse warning: arguments exceed max size of %d bytes. skipping...", SHELL_COMMAND_MAX_ARGSIZE);
					break;
				}
				switch(tokens[j].type) {
					case TOKEN_STRING_LITERAL:
						*((char**)(cmd->args + args_index)) = &(tokens[j].string_value[0]);
						args_index += sizeof(char*);
						break;
					case TOKEN_INT_LITERAL:
						*((long long int*)(cmd->args + args_index)) = tokens[j].int_value;
						args_index += sizeof(long long int);
						break;

					default:
						shell_log("parse error: expected a value\n");
				}
				j++;

				if(j < n) {
					// *   expect `,`   *
					if(tokens[j].type != TOKEN_COMMA) {
						shell_log("parse error: expected `,` after agument\n");
						return -1;
					}
				}
				j++;
			}

			return 0;
		}
	}

	shell_log("parse error: `%s` is not a shell command\n", op);
	return -1;
}

int shell_parse_set(ShellToken tokens[EXPRESSION_MAX_TOKENS], size_t n, ShellCommand* cmd) {
	assert(n);
	assert(cmd);

	if(n < 3) {
		shell_log("parse error: expected a certain amount of stuff but received fewer stuff\n");
		return -1;
	}

	assert(tokens[0].type == TOKEN_EXCLAMATION_POINT);
	cmd->type = COMMAND_SET;
	shell_log("doing stuf!\n");

	char* name = tokens[1].string_value;
	shell_log("name : %s\n", name);
	// ** search for name in shell scope **
    for(int i = 0; i < _shell_scope_size__; i++) {
		if(!strcmp(name, _shell_scope__[i]->op)) {
			cmd->decl = _shell_scope__[i];

			assert(!cmd->decl->action);
			assert(cmd->decl->value);
			
			// put the thing into the thing, but be careful about the size of the thing
			ShellToken* value = &tokens[2];
			switch(value->type) {
				case TOKEN_STRING_LITERAL:
					assert(memcpy(cmd->args, value->string_value, strlen(value->string_value)));
					break;
				case TOKEN_INT_LITERAL:
					assert(memcpy(cmd->args, &value->int_value, cmd->decl->size));
					break;
				default:
					shell_log("parse error: expected a value\n");
			}

			return 0;
		}
	}

	return -1;
}
// on success writes command in *cmd and returns 0
// on error returns -1
int shell_parse_expression(ShellToken tokens[EXPRESSION_MAX_TOKENS], size_t n, ShellCommand* cmd) {
	assert(n);
	assert(cmd);

	switch(tokens[0].type) {
		case TOKEN_OP: 
			return shell_parse_call(tokens, n, cmd);
		case TOKEN_EXCLAMATION_POINT:
			return shell_parse_set(tokens, n, cmd);
		default:
			shell_log("parse error: first token is not an operation\n");
			return -1;
	}

	return -1;
}

pid_t shell_execute_call(ShellCommand cmd, int pipes[2]) {
	assert(cmd.type == COMMAND_CALL);

    pid_t command_pid;

	if (pipe(pipes) < 0) {
		perror("pipe");
		return -1;
	}

    if ((command_pid = fork()) < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (command_pid == 0) { 

        dup2(pipes[1], 1);   // redirect stdout to out[1]
        dup2(pipes[1], 2);   // redirect stderr to err[1] 

		// child
		close(pipes[1]);
        close(pipes[0]);

		setbuf(stdout, NULL);
		setbuf(stderr, NULL);

		cmd.decl->action(cmd.args);
		exit(0);
    } 

	close(pipes[1]);
	return command_pid;
}

int shell_execute_set(ShellCommand cmd) {
	assert(cmd.type == COMMAND_SET);

	assert(memcpy(cmd.decl->value, cmd.args, cmd.decl->size));
	return -1;
}

pid_t shell_execute_command(ShellCommand cmd, int pipes[2]) {
	return cmd.type == COMMAND_CALL ? shell_execute_call(cmd, pipes) :
		   cmd.type == COMMAND_SET  ?  shell_execute_set(cmd)        : -1;
}

pid_t shell_interpret_expression(char* expression, size_t expression_len, int pipes[2]) {
	ShellToken tokens[EXPRESSION_MAX_TOKENS];
	ShellCommand cmd = {0};

	int ntokens = shell_lex_expression(expression, expression_len, tokens);
	if(ntokens <= 0) {
		shell_log("lex error: aborting...\n");
		return -2;
	}

	for(int i = 0; i < ntokens; i++) {
		shell_log("token #%d : { %d,  %s, %lld } \n", i, tokens[i].type, tokens[i].string_value, tokens[i].int_value);
	}

	if(shell_parse_expression(tokens, ntokens, &cmd) < 0) {
		shell_log("parse error: aborting...\n");
		return -2;
	}

	return shell_execute_command(cmd, pipes);
}

static char shell_terminal_current_expression[MAX_EXPRESSION_LENGTH];
void shell_terminal_clear_expression() {
	memset(shell_terminal_current_expression, 0, MAX_EXPRESSION_LENGTH);
}

void shell_terminal_print_line() {
	clrtoeol(); 
	printw(SHELL_TERMINAL_FORMAT, shell_terminal_current_expression);
}

int shell_terminal_should_close = 0;
void shell_terminal_close() {
	assert(stdscr);
	assert(endwin() == OK);
	shell_terminal_should_close = 1;
}

SHELL_FUNCTION_DEF(quit, {}, "quit") {
	if(stdscr) {
		// we assume we are a terminal child process, so send SIGTERM up
        kill(getppid(), SIGTERM);
	}
}

void shell_terminal_init() {
    initscr();
    raw();
	noecho();
	scrollok(stdscr, TRUE);
    keypad(stdscr, TRUE);
	assert(def_prog_mode() == OK);
	
	addch('\r');
	shell_terminal_print_line();
	refresh();

    signal(SIGTERM, shell_terminal_close);
}

void shell_terminal_interpret_expression_and_print_output() {
	assert(reset_shell_mode() == OK); {
		addch('\n');

		int pipes[2];
		pid_t command_pid = shell_interpret_expression(
			shell_terminal_current_expression, 
			strlen(shell_terminal_current_expression), 
			pipes
		);

		if(command_pid >= 0) {
			unsigned char b;
			while (read(pipes[0], &b, 1) > 0) {
				addch(b);
			}

			close(pipes[0]);
			waitpid(command_pid, NULL, 0);
		}

		refresh();
	}; assert(reset_prog_mode() == OK);
}

void shell_terminal_update() {
    int y, x; getyx(stdscr, y, x);
	int c =   getch();

	switch(c) {
		case KEY_LEFT:
			move(y, x > SHELL_TERMINAL_OFFSET ? x - 1 : x);
			break;

		case KEY_RIGHT:
			move(y, x < strlen(shell_terminal_current_expression) + SHELL_TERMINAL_OFFSET ? x + 1 : x);
			break;

		case 263: // Backspace key
			if (x > SHELL_TERMINAL_OFFSET) {
				int expression_index = x - SHELL_TERMINAL_OFFSET;
				assert(expression_index >= 0);

				memmove(&shell_terminal_current_expression[expression_index - 1],
					   	&shell_terminal_current_expression[expression_index],
					   	strlen(shell_terminal_current_expression) - expression_index + 1);
				move(y, x - 1);
			}
			break;

		case '\n':
			shell_terminal_interpret_expression_and_print_output();

			if(shell_terminal_should_close) {
				assert(reset_shell_mode() == OK);
				return;
			}

			shell_terminal_clear_expression();
			shell_terminal_print_line();
			refresh();
			return;

		default:
			if(x < MAX_EXPRESSION_LENGTH + SHELL_TERMINAL_OFFSET && isprint(c)) {
				int expression_index = x - SHELL_TERMINAL_OFFSET;
				assert(expression_index >= 0);

				memmove(&shell_terminal_current_expression[expression_index + 1],
					   	&shell_terminal_current_expression[expression_index],
					   	strlen(shell_terminal_current_expression) - expression_index);
				shell_terminal_current_expression[expression_index] = (char)c;

				move(y, x + 1);
			}
			break;
	}

	getyx(stdscr, y, x);
		addch('\r');
		shell_terminal_print_line();
	move(y, x);
	refresh();
}

int shell_rc(char* buffer, size_t length) {
	assert(buffer);
	assert(strlen(buffer) == length);

	shell_terminal_clear_expression();

	size_t index = 0;
	while(index < length) {
		if(strlen(shell_terminal_current_expression) >= MAX_EXPRESSION_LENGTH) {
			shell_log("shell_rc : warning: expression excedes maximum length (%d), skipping...\n", MAX_EXPRESSION_LENGTH);
		} else {
			shell_terminal_current_expression[strlen(shell_terminal_current_expression)] = buffer[index];
		}

		if(buffer[index] == '\n' || buffer[index] == '\0') {
			shell_terminal_interpret_expression_and_print_output();
			shell_terminal_clear_expression();
		}

		index++;
	}

	shell_terminal_clear_expression();
	shell_terminal_print_line();
	refresh();

	return 0;
}

int shell_rc_from_file(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if(!file) {
		perror("shell_rc : fopen");
		return -1;
	}	

	size_t length;
	fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

	char* buffer = malloc(length + 1);
	if(!buffer) {
		perror("shell_rc : could not allocate memory");
		fclose(file);
		return -1;
	}

	if (fread(buffer, 1, length, file) != length) {
        perror("shell_rc : error reading file");
        fclose(file);
		free(buffer);
        return -1;
    }

    buffer[length] = 0;
    fclose(file);

	int ret = shell_rc(buffer, length);
	free(buffer);
	return ret;
}

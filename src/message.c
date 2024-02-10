/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#include <message.h>

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

MessageBuffer new_message_buffer(size_t size) {
	MessageBuffer buffer = {0};

	unsigned char* data = malloc(size);	
	if(!data) {
		return buffer;
	}

	buffer.size = size;
	buffer.buffer = data;
	return buffer;
}

MessageLength message_buffer_len(MessageBuffer buffer) {
	int delta = (int)buffer.w - (int)buffer.r;

	if(delta < 0) {
		return buffer.size + delta;
	}
	return delta;
}

int message_buffer_resize(MessageBuffer* buffer, size_t size) {
	unsigned char* data = malloc(size);
	if(!data) {
		perror("message_buffer_resize: malloc error: ");
		return -1;
	}
	
	// q: is it better to keep the old read index in the newly allocated buffer 
	// 	  or to reset it to 0 sliding the entire message?
	MessageLength buffer_len = message_buffer_len(*buffer);
	for(size_t i = 0; i < buffer_len; i++) {
		size_t _i = (buffer->r + i) % buffer->size;
		size_t _j = (buffer->r + i) %         size;
		data[_j] = buffer->buffer[_i];
	}

	buffer->w      = (buffer->r + buffer_len) % size;
	buffer->size   = size;
	buffer->buffer = data;
	return size;
}

int message_buffer_empty(MessageBuffer* buffer) {
	buffer->w = buffer->r;
	return 0;
}

int p(MessageBuffer* buffer, unsigned char x) {
	// TODO: test `if` instead of `while`
	while((buffer->w + 1) % buffer->size == buffer->r) {
		// buffer is full, need resize
		
		size_t new_size = buffer->size * 2;
		if(message_buffer_resize(buffer, new_size) < 0) {
			return -1;
		}
	}
	assert(message_buffer_len(*buffer) < buffer->size);

	buffer->buffer[buffer->w] = x;
	buffer->w = (buffer->w + 1) % buffer->size;
	return 0;
}

// ** assuming CPU uses Little Endian encoding in all these functions **
int p32(MessageBuffer* buffer, unsigned int x) {
	for(int i = 0; i < 4; i++) {
		int err = p(buffer, (x >> (i * 8) & 0xFF));
		if(err < 0) {
			return err;
		}
	}	
	
	return 0;
}

int p64(MessageBuffer* buffer, unsigned long long x) {
	for(int i = 0; i < 8; i++) {
		int err = p(buffer, (x >> (i * 8) & 0xFF));
		if(err < 0) {
			return err;
		}
	}	
	
	return 0;
}

/*
	String Length : uint32
	String 		  : bytes
*/
int pstring(MessageBuffer* buffer, char* x, unsigned int x_len) {
	int err = p32(buffer, x_len);
	if(err < 0) {
		return err;
	}
	for(int i = 0; i < x_len; i++) {
		int err = p(buffer, x[i]);
		if(err < 0) {
			return err;
		}
	}	

	return 0;
}

int pcode(MessageBuffer* buffer, MessageCode code) {
	switch(MESSAGE_CODE_TYPE(code)) {
		case MESSAGE_TYPE_SERVER:
		case MESSAGE_TYPE_PEER:
			return p32(buffer, MESSAGE_CODE_CODE(code));
		case MESSAGE_TYPE_PEERINIT:
		case MESSAGE_TYPE_DISTRIBUTED:
			return p(buffer, MESSAGE_CODE_CODE(code));
		case MESSAGE_TYPE_FILE:
			assert(0 && "pcode: MESSAGE_TYPE_FILE is unhandled (TODO)");
	}
	
	return -1;
}

int u(MessageBuffer* buffer, unsigned char *x) {
	if(buffer->r == buffer->w) {
		return 0;
	}

	*x = buffer->buffer[buffer->r];
	buffer->r = (buffer->r + 1) % buffer->size;
	return 1;
}

// ** assuming CPU uses Little Endian encoding in all these functions **
int u32(MessageBuffer* buffer, unsigned int *x) {
	int r = 0;
	for(int i = 0; i < 4; i++) {
		r += u(buffer, (unsigned char*)x + i);
	}	
	
	return r;
}

int u64(MessageBuffer* buffer, unsigned long long *x) {
	int r = 0;
	for(int i = 0; i < 8; i++) {
		r += u(buffer, (unsigned char*)x + i);
	}	
	
	return r;
}

//   String Length (x_len is input)   : uint32
//   String 		  				  : bytes
int ustring(MessageBuffer* buffer, char* x, unsigned int x_len) {
	int r = 0;

	// r += u32(buffer, (unsigned int*)x_len);
	// if(r < 4) {
	// 	return r;
	// }

	for(int i = 0; i < x_len; i++) {
		r += u(buffer, (unsigned char*)x + i);
	}	
	
	return r;
}

int ucode(MessageBuffer* buffer, MessageType type, MessageCode *code) {
	int ret = -1;

	*code = 0;
		switch(type) {
			case MESSAGE_TYPE_SERVER:
			case MESSAGE_TYPE_PEER:
				ret = u32(buffer, code);
				break;
			case MESSAGE_TYPE_PEERINIT:
			case MESSAGE_TYPE_DISTRIBUTED:
				ret = u(buffer, (unsigned char*)code);
				break;
			case MESSAGE_TYPE_FILE: assert(0 && "TODO");
		}
	*code = MESSAGE_CODE(type, *code);

	return ret;
}

MessageBuffer* send_message(MessageBuffer* buffer, int client_fd) {
	// *** send message
	ssize_t sent = 0;
	printf("sending %lu bytes (MessageLength)...\n", sizeof(MessageLength));
	MessageLength buffer_len = message_buffer_len(*buffer);
	sent = send(client_fd, &buffer_len, sizeof(MessageLength), 0);
	if(sent < 0) {
		printf("error: could not send message length!\n");
		return NULL;
	}
	printf("sent: %lu\n", sent);


	// FIXME: this whole stuff is ugly, but also sending byte by byte is slow and apperently 
	// 		  the server does not like it... reading byte by byte works though! [ see read_message() ]
	// 		  still slow i guess but cleaner than this.
	printf("sending %u bytes:\n", buffer_len);
	if((int)buffer->w - (int)buffer->r < 0) {
		printf("sending wrap around message of length %u... \n", buffer_len);
	
		size_t _first_send = send(client_fd, &buffer->buffer[buffer->r], buffer->size - buffer->r, 0);
		printf("sent first part of length %lu\n", _first_send);
		if(_first_send < 0) {
			perror("error: could not send packed message! : ");
			return NULL;
		}
		size_t _second_send = send(client_fd, buffer->buffer, buffer->w, 0);
		printf("sent second part of length %lu\n", _second_send);
		if(_second_send < 0) {
			perror("error: could not send packed message! : ");
			return NULL;
		}
	
		sent = _first_send + _second_send;
	} else {
		sent = send(client_fd, &buffer->buffer[buffer->r], buffer_len, 0);
		printf("sent contigous (no wrap around) message of length %lu\n", sent);
		if(sent < 0) {
			perror("error: could not send packed message! : ");
			return NULL;
		}
	}
	assert(sent == buffer_len);

	message_buffer_empty(buffer);
	return buffer;
}

// return {
// 	-1 : ERROR
//   0 : EOF
// > 0 : Message Length
int read_message(MessageBuffer* buffer, int client_fd) {
	MessageLength buffer_len = 0;
	message_buffer_empty(buffer);

	if(read(client_fd, &buffer_len, sizeof(MessageLength)) < 0) {
		return -1;
	}
	// printf("response_len : %u\n", buffer_len);

	for(size_t i = 0; i < buffer_len; i++) {
		unsigned char b = 0;
		int r;

		if((r = read(client_fd, &b, 1)) <= 0) {
			return r;
		}

		if(p(buffer, b) < 0) {
			return -1;
		}
	}
	assert(buffer_len == message_buffer_len(*buffer));

	return buffer_len;
}

#define COMPRESS_CHUNK 64 * 1024
#define UPPER_CLAMP(x, y) x >= y ? y : x
int compress_message(MessageBuffer* buffer) {
	z_stream stream;
	int ret, flush;
	unsigned char in[COMPRESS_CHUNK];
	unsigned char out[COMPRESS_CHUNK];

	// Initialize zlib stream
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	ret = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK) {
		return ret;
	}

	int remaining_input = message_buffer_len(*buffer);

	// Compress the data
	do {
		stream.avail_in = ustring(buffer, (char*)in, UPPER_CLAMP(remaining_input, COMPRESS_CHUNK));
		remaining_input -= stream.avail_in;
		assert(remaining_input >= 0);

		flush = remaining_input ? Z_NO_FLUSH : Z_FINISH;

		stream.next_in  = in;
		do {
			stream.avail_out = COMPRESS_CHUNK;
			stream.next_out = out;
			ret = deflate(&stream, flush);
			assert(ret != Z_STREAM_ERROR);

			for(size_t i = 0; i < COMPRESS_CHUNK - stream.avail_out; i++) {
				p(buffer, out[i]);
			}
		} while (stream.avail_out == 0);
		assert(stream.avail_in == 0);

	} while (flush != Z_FINISH);
	assert(ret == Z_STREAM_END);
	assert(!remaining_input);

	// Clean up
	deflateEnd(&stream);
	return Z_OK;
}

int decompress_message(MessageBuffer* buffer) {
	z_stream stream;
	int ret;
	unsigned char in[COMPRESS_CHUNK];
	unsigned char out[COMPRESS_CHUNK];

	// Initialize zlib stream
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	ret = inflateInit(&stream);
	if (ret != Z_OK) {
		return ret;
	}

	int remaining_input = message_buffer_len(*buffer);

	// Decompress the data
	do {
		stream.avail_in = ustring(buffer, (char*)in, UPPER_CLAMP(remaining_input, COMPRESS_CHUNK));
		if(!stream.avail_in) {
			break;	
		}

		remaining_input -= stream.avail_in;
		assert(remaining_input >= 0);

		stream.next_in  = in;
		do {
			stream.avail_out = COMPRESS_CHUNK;
			stream.next_out = out;
			ret = inflate(&stream, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					inflateEnd(&stream);
					return ret;
			}

			for(size_t i = 0; i < COMPRESS_CHUNK - stream.avail_out; i++) {
				p(buffer, out[i]);
			}
		} while (stream.avail_out == 0);

	} while(ret == Z_STREAM_END);
	assert(!remaining_input);

	// Clean up
	inflateEnd(&stream);
	return Z_OK;
}
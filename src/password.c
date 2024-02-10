/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#include <password.h>

#include <stdio.h>

void md5_string_from_bytes(unsigned char digest[MD5_DIGEST_LENGTH], char data[MD5_DIGEST_STRING_LENGTH + 1] /* ( + NULL-terminator ) */) { 
	char b[3];
	for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
		sprintf(b, "%02hhx", digest[i]);
		data[i * 2]     = b[0];
		data[i * 2 + 1] = b[1];
	}	

	// zero terminate lol
	data[32] = 0;
}
/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#ifndef SS_PASSWORD_H
#define SS_PASSWORD_H

#include <md5.h>

void md5_string_from_bytes(unsigned char[MD5_DIGEST_LENGTH], char[MD5_DIGEST_STRING_LENGTH + 1] /* ( + NULL-terminator ) */);

#endif // SS_PASSWORD_H
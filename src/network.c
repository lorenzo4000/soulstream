/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#include <network.h>
#include <netinet/in.h>

// network byte order (big-endian) ipv4 address
struct in_addr ipv4_from_resolve(unsigned char res_answer[], int res_answer_len) {
	int ip_offset = res_answer_len - 4; // last 4 bytes are IPV4

	unsigned int ip = 0;
	for(int i = ip_offset; i < res_answer_len; i++) {
		ip |= (unsigned int)(((int)res_answer[i]) << (((i - ip_offset))*8));
	}	

	struct in_addr ret = {
		ip
	};

	return ret;
}
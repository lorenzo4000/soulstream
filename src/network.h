/*																		
   Copyright (C) 2024  Contina Lorenzo Giuseppe									
   																				
   This program is free software: you can redistribute it and/or modify			
   it under the terms of the GNU General Public License as published by			
   the Free Software Foundation, either version 3 of the License, or			
   (at your option) any later version. See http://www.gnu.org/copyleft/gpl.html 
   the full text of the license.											    
*/

#ifndef SS_NETWORK_H
#define SS_NETWORK_H

#include <netinet/in.h>

// network byte order (big-endian) ipv4 address
struct in_addr ipv4_from_resolve(unsigned char[], int);


#endif // SS_NETWORK_H
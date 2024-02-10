#ifndef SS_NETWORK_H
#define SS_NETWORK_H

#include <netinet/in.h>

// network byte order (big-endian) ipv4 address
struct in_addr ipv4_from_resolve(unsigned char[], int);


#endif // SS_NETWORK_H

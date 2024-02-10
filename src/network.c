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

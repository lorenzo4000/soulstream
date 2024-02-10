#ifndef SS_PASSWORD_H
#define SS_PASSWORD_H

#include <md5.h>

void md5_string_from_bytes(unsigned char[MD5_DIGEST_LENGTH], char[MD5_DIGEST_STRING_LENGTH + 1] /* ( + NULL-terminator ) */);

#endif // SS_PASSWORD_H

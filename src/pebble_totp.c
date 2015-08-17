#include "pebble.h"
#include "pebble_totp.h"

#include "otp.h"

#define MIN(A, B) ((A) > (B) ? (B) : (A))

void
pebble_totp_init(pebble_totp *token,
                 const unsigned char *key,
                 size_t keylen,
                 unsigned short interval)
{
    memset(token->key, 0x00, sizeof(token->key));
    memcpy(token->key, key, MIN(sizeof(token->key), keylen));

    token->interval = interval;
}

char *
pebble_totp_get_code(pebble_totp *token,
		     time_t t)
{
    unsigned int totpvalue;

    totpvalue = otp_value(token->key, sizeof(token->key), t / token->interval);
    snprintf(token->buffer, sizeof(token->buffer), "%.6u", totpvalue);

    return token->buffer;
}

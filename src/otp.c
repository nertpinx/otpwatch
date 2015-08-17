#include "sha1.h"
#include <stdint.h>
#include <string.h>

static int
otp_truncate(const unsigned char *hmac_result)
{
    unsigned short offset = hmac_result[19] & 0xf;
    return  ((hmac_result[offset]  & 0x7f) << 24 |
             (hmac_result[offset+1] & 0xff) << 16 |
             (hmac_result[offset+2] & 0xff) <<  8 |
             (hmac_result[offset+3] & 0xff));
}

#define GET_HIGH_ORDER_BYTE(VAR, BYTE) \
                (VAR >> ((8 - (BYTE + 1)) * 8))


static void
otp_convert_counter(uint64_t counter,
                    unsigned char *counterbuf)
{
    counterbuf[0] = GET_HIGH_ORDER_BYTE(counter, 0);
    counterbuf[1] = GET_HIGH_ORDER_BYTE(counter, 1);
    counterbuf[2] = GET_HIGH_ORDER_BYTE(counter, 2);
    counterbuf[3] = GET_HIGH_ORDER_BYTE(counter, 3);
    counterbuf[4] = GET_HIGH_ORDER_BYTE(counter, 4);
    counterbuf[5] = GET_HIGH_ORDER_BYTE(counter, 5);
    counterbuf[6] = GET_HIGH_ORDER_BYTE(counter, 6);
    counterbuf[7] = GET_HIGH_ORDER_BYTE(counter, 7);
}


int
otp_value(const unsigned char *key,
          size_t keylen,
          uint64_t counter)
{
    unsigned char hmac_result[20] = "abcd";
    unsigned char counterbuf[8];

    otp_convert_counter(counter, counterbuf);
    sha1_hmac(key, keylen, counterbuf, 8, hmac_result);

    return otp_truncate(hmac_result) % 1000000;
}

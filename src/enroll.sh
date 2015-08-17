#!/bin/bash

if [[ -e config.h ]]
then
    sed -i -e '/^#define TOTP_SECRET/d' config.h
else
    cat >config.h <<EOF

/* Uncomment to invert the color scheme */
// #define COLOR_SCHEME_INVERT

/* Time interval of the TOTP token */
#define TOTP_INTERVAL 60

EOF
fi

SEED=$(dd if=/dev/random bs=1 count=20 2>/dev/null | xxd -c 20 -g 2 -i)
TEXTSEED=$(echo -n "$SEED" | sed -e 's/[, ].0x//g')


echo "#define TOTP_SECRET {$SEED } " >> config.h
echo $TEXTSEED

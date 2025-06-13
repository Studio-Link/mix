#!/usr/bin/env bash
set -e

if [ "$1" = 'slmix' ]; then
    if [[ -z $TOKENHOST ]]; then
        echo "No TOKENHOST env!"
        exit 1
    fi
    if [[ -z $TOKENDOWNLOAD ]]; then
        echo "No TOKENDOWNLOAD env!"
        exit 1
    fi
    if [[ -z $TOKENGUEST ]]; then
        echo "No TOKENGUEST env!"
        exit 1
    fi
    if [[ -z $TOKENAPI ]]; then
        echo "No TOKENAPI env!"
        exit 1
    fi
    cat > /opt/slmix/config <<EOF
mix_room                MixRoom
mix_url                 /
mix_token_host          $TOKENHOST # can start record
mix_token_download      $TOKENDOWNLOAD # protected download folder
mix_token_guests        $TOKENGUEST # invite url
mix_token_api           $TOKENAPI # api token
#mix_path               /opt/slmix/
EOF
    sudo -E /usr/bin/nginx &
    cd /opt/slmix && /opt/slmix/build/slmix -c /opt/slmix/config &
    wait -n
    exit $?
else
    exec "$@"
fi

#!/bin/bash

sip_path=/usr/local/sip_rtp/bin

function main() {
    cd $sip_path
    while true
    do
        if [ `ps -ef | grep sip_rtp | grep -v grep | wc -l` -eq 0 ];then
                ./sip_rtp
        fi

        sleep 120
        #echo "again"
    done

}

main
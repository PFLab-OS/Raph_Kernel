#!/bin/sh

LOCAL=`date +%s`
REMOTE=`ssh -F .ssh_config default "date +%s"`
if [ -z "REMOTE" ]; then
    vagrant reload
    REMOTE=`ssh -F .ssh_config default "date +%s"`
fi
if [ -z "REMOTE" ]; then
    echo "unknown error"
    exit 1
fi
DIFF=`expr $LOCAL - $REMOTE`
if [ $DIFF -lt 0 ] ; then
    DIFF=`expr -1 \* $DIFF`
fi
if [ $DIFF -gt 2 ] ; then
    ssh -F .ssh_config default "sudo service ntp stop; sudo ntpdate ntp.nict.jp; sudo service ntp start"
fi

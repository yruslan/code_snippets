#!/bin/bash

# This is a tiny script that prepares the home folder of the current Linux user
# to be logged in without a password.
# - It checks permissions of the home folder, .ssh folder and files inside it.
# - It also generates a SSH key for the current user on the current host, althought
#   it is not required for incoming passwordless connections.
#
# Usage:
# ./ssh_nopass.sh

WRITE_GROUP=`ls -ld ~ | cut -c6-6`
WRITE_OTHER=`ls -ld ~ | cut -c9-9`
if [ $WRITE_GROUP = "w" ]; then
    echo "Warning! Home directory is writeable by group. Fixed!"
    chmod g-w ~
fi
if [ $WRITE_OTHER = "w" ]; then
    echo "Warning! Home directory is writeable by all. Fixed!"
    chmod o-w ~
fi

if [ ! -d ~/.ssh ]; then
    mkdir ~/.ssh
fi

chmod 700 ~/.ssh

if [ ! -f ~/.ssh/authorized_keys ]; then
    echo -n "Setting up passwordless authentication for current host..."
    if [ ! -f ~/.ssh/id_dsa ]; then
       echo -n "Generating new private key..."
       ssh-keygen -t dsa -q -f ~/.ssh/id_dsa -N "`whoami`@`hostname`"
    fi
    chmod 600 ~/.ssh/id_dsa
    cat ~/.ssh/id_dsa.pub >> ~/.ssh/authorized_keys
    chmod 644 ~/.ssh/authorized_keys
    echo "OK"
else
    AUTHORIZED_KEYS_PERM=`ls -ld ~/.ssh/authorized_keys | cut -c1-10`
    if [ $AUTHORIZED_KEYS_PERM != "-rw-r--r--" ]; then
        echo "Warning! ~/.ssh/authorized_keys permissions are wrong. Fixed!"
        chmod 644 ~/.ssh/authorized_keys
    fi
fi

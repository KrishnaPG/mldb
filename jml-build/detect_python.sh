#!/bin/bash --norc

if [ -d /usr/include/python3.6 ] ; then
    echo "3.6m";
    exit 0;
fi

if [ -d /usr/include/python3.4 ] ; then
    echo "3.4";
    exit 0;
fi

if [ -d /usr/include/python3.2 ] ; then
    echo "3.2";
    exit 0;
fi

echo "unknown";
exit 0

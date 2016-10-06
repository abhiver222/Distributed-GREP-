#!/bin/bash

gcc -pthread server.c -o server
gcc -pthread client.c -o client
gcc unit_test.c -o unit_test

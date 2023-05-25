#!/bin/sh
gcc -fPIC -fno-stack-protector -c pam_module_example.c
sudo ld -x --shared -o /lib/x86_64-linux-gnu/security/pam_module_example.so pam_module_example.o
gcc -o run_pam.o pam_example.c -lpam -lpam_misc
sudo ./run_pam.o


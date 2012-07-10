NOTE
Dont Configure! Except you want to modify the makefile to adjust the -L and -I options to CC (gcc)

COMPILE
make

GENERAL BUILD

<em>Don't! Except you want to do a little modification to the source</em>

cd src

gcc FPentrypoint.c actions.c database.c -I /opt/DigitalPersona/OneTouchSDK/include/ -L /opt/DigitalPersona/lib -m32 -ldpfpapi -ldpFtrEx -ldl -lc -lm -lpthread -ldpMatch -lsqlite3 `pkg-config --libs --cflags gtk+-2.0` -o output

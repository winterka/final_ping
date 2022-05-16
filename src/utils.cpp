#include "../include/utils.h"
#include <iostream>
#include <signal.h>
#include "../include/ping.h"
#include "../include/loggers.h"

extern int errorCode;
void Diag();

int checkParams(int argc, char *argv[]){
    if (argc == 2)
        return 1;
    errorCode = 1;
    Diag();
    exit(0);
}



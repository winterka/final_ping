#include <signal.h>
#include "src/ping.h"
#include "src/loggers.h"
Ping * p;
extern int AddMessageToLog(const char * message);
extern int main_func();
extern int errorCode;
extern int Diag();
void SingnalHandler(int signo) {

    p->statistic();

    exit(0);
}

int main(int argc, char * argv[]) {
    main_func();
    if (argc != 2){
        errorCode = 1;
        Diag();
    }
    struct sigaction action;
    action.sa_handler = SingnalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT,&action,NULL);

    Ping ping(argv[1], 1);
    p = &ping;
    ping.CreateSocket();
    while(1)
    {
        ping.SendPacket();
        ping.RecvPacket();
        sleep(1);
    }
}

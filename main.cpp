#include <signal.h>        // Заголовочный файл из стандартной библиотеки
#include "include/ping.h"      // Заголовочный файл описывающий функции для пинга
#include "include/utils.h"
#include "include/loggers.h"  // Заголовочный файл описывающий функции для логирования

Ping * p;

int logFunc();
struct sigaction action;
// Обработчик сигналов, после остановки программы <CTRL + C> будет выведена статистика
// и остановлена работа программы.
void SingnalHandler(int signo) {

    p->statistic();

    exit(0);
}


int main(int argc, char * argv[]) {
    logFunc();
    checkParams(argc,argv);
    ///////////////////////////////////
    // Создание сигнала для обработки
    action.sa_handler = SingnalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT,&action,NULL);
    ///////////////////////////////////
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

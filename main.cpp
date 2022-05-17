#include <signal.h>        // Заголовочный файл из стандартной библиотеки
#include "include/ping.h"      // Заголовочный файл описывающий функции для пинга
#include "include/utils.h"
#include "include/loggers.h"  // Заголовочный файл описывающий функции для логирования


Ping * p; //указатель на класс Ping
int logFunc(); // начало работы записи в файл лога
struct sigaction action; // структура для обработки сигнала
// Обработчик сигналов, после остановки программы <CTRL + C> будет выведена статистика
// и остановлена работа программы.
void SingnalHandler(int signo)
{

    p->statistic();

    exit(0);
}


int main(int argc, char * argv[]) 
{
    logFunc();
    checkParams(argc,argv);
    ///////////////////////////////////
    // Создание сигнала для обработки
    action.sa_handler = SingnalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT,&action,NULL);
  /////////////////////////////////////
    Ping ping(argv[1], 1);
    p = &ping; 
    ping.CreateSocket(); // Открываем соединение
    while(1) // Запускаем отправку/принятие пакетов с задержкой в 1 секунду
    {
        ping.SendPacket(); // Отправляем пакет
        ping.RecvPacket(); // Получаем пакет
        sleep(1); // Ожидаем следующей итерации 
    }
}

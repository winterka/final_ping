#include <fcntl.h>
#include "../include/ping.h"
#include "../include/loggers.h"

extern void DiagLog(); //функция обработки возможной ошибки при записи в лог
extern int errorCode; // глобальная переменная хранящая код ошибки
extern int AddMessageToLog(const char * message); //функция для записи в лог
void Diag(); // функция для обработки возможных ошибок и записи их в лог
//переменные для временного хранения сообщений/строк ?
char logText[4048]; 
char buff[4048];  

int icmp_len, ip_header_len; // переменные для хранения размеров структур icmp и ip
int errnop = 0; // переменная для хранения кодов системных ошибок  
double rtt; // переменная для хранения задержки
unsigned long in_addr; // переменная для хранения адреса сокета
struct protoent * protocol; // используемый протокол
struct hostent host_info, * host_pointer; //информация о хосте на который будут отправляться запросы и указатель на него            
struct icmp * icmp_pointer; //указатель на icmp структуру
struct timeval * time_pointer; //указатель времени
struct timeval timeout; //переменная для хранения времени ожидания пакета
struct timeval * time_send; //указатель на время отправки пакета

// Описание функции Ping
Ping::Ping(const char * ip, int max_wait_time)
{
    //const char * ip - адрес по которому отправляются пакеты, int max_wait_time - максимальное время ожидания пакета. 
    this->input_domain = ip;
    //printf(ip);

    this->max_wait_time = max_wait_time < 3 ? max_wait_time : 3;

    // переменные для вывода статистики пинга.
    this->send_pack_num = 0;
    this->recv_pack_num = 0;
    this->lost_pack_num = 0;

    this->min_time = 0;
    this->max_time = 0;
    this->sum_time = 0;
}

Ping::~Ping() 
{
    // обработка ошибки закрытия сокета
    if(close(sock_fd) == -1) {
        fprintf(stderr, "Close socket error:%s \n\a", strerror(errno));
        // присваиваем код ошибки
        errorCode = 3;
        if (AddMessageToLog(logText)==-1) // проверяем нет ли ошибки при записи в логи
            DiagLog(); // вывод ошибки в лог
        Diag(); // вывод ошибки на экран
        exit(1);
    }
}

// описание функции создания сокета
void Ping::CreateSocket()
{                      
    // проверяем является ли используемый протокол протоколом ICMP
    // В случае ошибки присваиваем код ошибки и обрабатываем его    
    if((protocol = getprotobyname("icmp")) == NULL)
    {
        fprintf(stderr, "Get protocol error:%s \n\a", strerror(errno)); // вывод ошибки на экран 
        errorCode = 0;
        if (AddMessageToLog(logText)==-1)
            DiagLog(); // вывод ошибки в лог
        Diag(); // вывод ошибки на экран
        exit(1);
    }
    // пробуем создать сокет с помощью функции socket() из стандартной библиотеки.
    if((sock_fd = socket(AF_INET, SOCK_RAW, protocol->p_proto)) == -1)
    {
        //AF_INET - обозначает то что для коммуникации мы используем ipv4
        //SOCK_RAW - тип сокета, тип RAW позволяет использовать низкоуровневый функционал
        //protocol->p_proto - задает протокол.
        //В случае ошибки создания сокета присваиваем код ошибки и обрабатываем.
        fprintf(stderr, "Create RAW socket error:%s \n\a", strerror(errno)); // вывод ошибки на экран 
        errorCode = 3;
        if (AddMessageToLog(logText)==-1)
            DiagLog(); // вывод ошибки в лог
        Diag(); // вывод ошибки на экран
        exit(1);
    }
    //Присваиваем идентификатор процессу
    setuid(getuid());
    send_addr.sin_family = AF_INET;
    // Функция inet_addr() преобразует обычный вид IP-адреса cp (из номеров и точек) в двоичный код в сетевом порядке расположения байтов
    //Если входящий адрес неверен, то возвращается INADDR_NONE
    if((in_addr = inet_addr(input_domain.c_str())) == INADDR_NONE)
    {
        //В случае неудачи пробуем получить хост по имени домена, в случае неудачи обрабатываем ошибку.
        if(gethostbyname_r(input_domain.c_str(), &host_info, buff, sizeof(buff), &host_pointer, &errnop))
        {
            fprintf(stderr, "Get host by name error:%s \n\a", strerror(errno)); // вывод ошибки на экран 
            if (AddMessageToLog(logText)==-1)
                DiagLog(); // вывод ошибки в лог
            errorCode = 2;
            Diag(); // вывод ошибки на экран 
            exit(1);
        } else
        {
            // передаем полученный адрес в структуру send_addr
            this->send_addr.sin_addr = *((struct in_addr *)host_pointer->h_addr);
        }
    } else
    {
        //присваиваем в структуру адрес от которого мы согласны получать пакеты
        this->send_addr.sin_addr.s_addr = in_addr;
    }
    //Функция inet_ntoa() преобразует IP-адрес
    //заданный в сетевом порядке расположения байтов, в стандартный строчный вид, из номеров и точек.
    this->backup_ip = inet_ntoa(send_addr.sin_addr);
    //Добавляем в лог сообщение о первом отправленом пакете.
    AddMessageToLog("Creating Socket and sending first bytes");
    printf("PING %s (%s) %d(%d) bytes of data.\n", input_domain.c_str(),
            backup_ip.c_str(), PACK_SIZE - 8, PACK_SIZE + 20);
    //Получаем информацию о времени отправки первого пакета.
    gettimeofday(&first_send_time, NULL);
}

//Описание функции расчета контрольной суммы
unsigned short Ping::CalculateCksum(unsigned short * send_pack, int pack_size)
{   // переменные для контрольной суммы
    int check_sum = 0;              
    int nleft = pack_size;          
    unsigned short * p = send_pack; 
    unsigned short temp;            
    // цикл вписывает контрольную сумму пока не проверит весь пакет
    while(nleft > 1) 
    {
        check_sum += *p++;          
        nleft -= 2;
    }
    if(nleft == 1) // механизм обработки последнего элемента
    {
        
        *(unsigned char *)&temp = *(unsigned char *)p; // добавляем пакет во временное хранилище
        check_sum += temp;  
    }

    check_sum = (check_sum >> 16) + (check_sum & 0xffff); // побитовый сдвиг контрольной суммы 
    check_sum += (check_sum >> 16);                         
    temp = ~check_sum;              

    return temp;
}

// описание функции создания пакета
int Ping::GeneratePacket()
{
    //инциализация переменной для хранения размерности пакета
    int pack_size;
    //инициализация переменной для хранения структуры пакета
    icmp_pointer = (struct icmp *)send_pack;
    //присваиваем вид запроса
    icmp_pointer->icmp_type = ICMP_ECHO;
    //присваиваем код сообщения 
    icmp_pointer->icmp_code = 0;
    //обнуляем поле для хранения контрольной суммы для последующего расчета
    icmp_pointer->icmp_cksum = 0;       
    //присваиваем номер пакета   
    icmp_pointer->icmp_seq = send_pack_num + 1;
    //присваиваем в поле идентификатора пакета идентификатор процесса
    icmp_pointer->icmp_id = getpid();       
    //присваиваем размерность пакета
    pack_size = PACK_SIZE;
    //присваиваем в переменную время отправки пакета взятую из соответсвующего поля
    time_pointer = (struct timeval *)icmp_pointer->icmp_data;
    gettimeofday(time_pointer, NULL);
    //расчитываем контрольную сумму
    icmp_pointer->icmp_cksum = CalculateCksum((unsigned short *)send_pack, pack_size);

    return pack_size;
}
//описание функции создания пакета
void Ping::SendPacket()
{
    if (AddMessageToLog("Package generation...")==-1)
    {
        DiagLog(); 
    }
    //получаем размер пакета из функции создания пакета
    int pack_size = GeneratePacket();
    if (AddMessageToLog("Package generated.")==-1)
    {
        DiagLog();
    }
    if (AddMessageToLog("Sending a package...")==-1)
    {
        DiagLog();
    }
    // пробуем отправить пакет с помощью функции sendto()
    if((sendto(sock_fd, send_pack, pack_size,MSG_DONTWAIT, (const struct sockaddr *)&send_addr, sizeof(send_addr))) < 0)
    {
        //sock_fd - дескриптор нашего сокета
        //send_pack - данные для отправки
        //pack_size - размер данных для отправки в битах
        //MSG_DONTWAIT - флаг определяющий поведение запроса
        //(const struct sockaddr *)&send_addr - адресс отправки запроса
        // sizeof(send_addr))) < 0 - размер адреса отправки запроса в битах
        // в случае неудачи обрабатываем ошибку
        printf("Error");
        fprintf(stderr, "Sendto error:%s \n\a", strerror(errno)); // вывод ошибки на экран 
        if (AddMessageToLog(logText)==-1)
                DiagLog();
        errorCode = 4;
        Diag();
        exit(1);
    }
    if (AddMessageToLog("Package sent")==-1)
    {
        DiagLog();
    }
    //увеличиваем счетчик отправленных пакетов
    this->send_pack_num++;
}
//описание функции обработки пакета
int Ping::ResolvePakcet(int pack_size)
{
    //инциализация переменной для хранения пакета
    struct ip * ip_pointer = (struct ip *)recv_pack;
    //присваиваем длину полученного заголовка
    ip_header_len = ip_pointer->ip_hl << 2;    
    //инициализация переменной для хранения структуры пакета                 
    icmp_pointer = (struct icmp *)(recv_pack + ip_header_len); 
    //расчет размера полученного пакета
    icmp_len = pack_size - ip_header_len;                       
    //обрабатываем потерю пакета
    if(icmp_len < 8) 
    {
        printf("received ICMP pack lenth:%d(%d) is error!\n", pack_size, icmp_len); // вывод ошибки на экран 
        lost_pack_num++;
        return -1;
    }
    //в случае если полученные данные равны отправленным присваиваем в переменную время отправки пакета
    if((icmp_pointer->icmp_type == ICMP_ECHOREPLY) &&
        (backup_ip == inet_ntoa(recv_addr.sin_addr)) &&
        (icmp_pointer->icmp_id == getpid()))
    {

        time_send = (struct timeval *)icmp_pointer->icmp_data;
        //расчитываем время отправки полученного пакета
        if((recv_time.tv_usec -= time_send->tv_usec) < 0) 
        {
            --recv_time.tv_sec;
            recv_time.tv_usec += 10000000;
        }
        //расчитываем задержку принимаемого пакета
        rtt = (recv_time.tv_sec - time_send->tv_sec) * 1000 + (double)recv_time.tv_usec / 1000.0;
        if(rtt > (double)max_wait_time * 1000)
            rtt = max_time;
        //вычисляем максимальную и минимальную задержку
        if(min_time == 0 | rtt < min_time)
            min_time = rtt;
        if(rtt > max_time)
            max_time = rtt;
        //вычисляем суммарное время работы
        sum_time += rtt;
        //записываем в лог информацию о полученом пакете и обрабатываем возможную ошибку
        sprintf(logText,"%d byte from %s : icmp_seq=%u ttl=%d time=%.1fms\n",
               icmp_len,
               inet_ntoa(recv_addr.sin_addr),
               icmp_pointer->icmp_seq,
               ip_pointer->ip_ttl,
               rtt);
        printf(logText);
        AddMessageToLog(logText);
        if (AddMessageToLog(logText)==-1)
                DiagLog();
        //увеличиваем счетчик полученных пакетов
        recv_pack_num++;
    } else
    {
        //если данные отправленного пакета не совпадают с данными полученного то 
        //"выкидываем" полученный пакет
        printf("throw away the old package %d\tbyte from %s\ticmp_seq=%u\ticmp_id=%u\tpid=%d\n",
               icmp_len, inet_ntoa(recv_addr.sin_addr), icmp_pointer->icmp_seq,
               icmp_pointer->icmp_id, getpid());

        return -1;
    }

}
//описание функции получения пакета
void Ping::RecvPacket()
{
    //инциализация перменных для хранения размера полученного пакета и размера адреса
    int recv_size, fromlen;
    fromlen = sizeof(struct sockaddr);
    //проверяем что бы кол-во полученных пакетов не превышало кол-во отправленных
    while(recv_pack_num + lost_pack_num < send_pack_num) 
    {
        //инциализация структуры для хранения дескрипторов сокета
        fd_set fds;
        //очищает созданную структуру
        FD_ZERO(&fds);      
        //добавляет сокет в структуру        
        FD_SET(sock_fd, &fds);      
        //максимально кол-во сокетов в структуре
        int maxfd = sock_fd + 1;
        //максимальное время ожидание пакета
        timeout.tv_sec = this->max_wait_time;
        timeout.tv_usec = 0;

        //select - проверяет состояние готовности сокетов к I/O операциям
        int n = select(maxfd, &fds, NULL, NULL, &timeout);

        switch(n) 
        {
            //обрабатываем ошибку функции select()
            case -1:
                fprintf(stderr, "Select error:%s \n\a", strerror(errno));
                if (AddMessageToLog(logText)==-1)
                    DiagLog();
                errorCode = 0;
                Diag();
                exit(1);
            //обрабатываем потерю пакета
            case 0:
                printf("select time out, lost packet!\n");
                lost_pack_num++;
                break;
            default:
                
                if(FD_ISSET(sock_fd, &fds)) 
                {
                    //если сокет находится в множестве/структуре но размер полученного пакета меньше 0 - обрабатываем потерю пакета
                    if((recv_size = recvfrom(sock_fd, recv_pack, sizeof(recv_pack),
                            0, (struct sockaddr *)&recv_addr, (socklen_t *)&fromlen)) < 0) {
                        fprintf(stderr, "packet error(size:%d):%s \n\a", recv_size, strerror(errno));
                        lost_pack_num++;
                    } else
                    {
                        //обрабатываем получение и обработку пакета
                        gettimeofday(&recv_time, NULL);

                        ResolvePakcet(recv_size);
                    }
                }
                break;
        }
    }
}
//описание функции вывода статистики
void Ping::statistic() 
{
    //инициализация переменной для хранения общего времени работы
    double total_time;
    //переменная для хранения времени окончания работы
    struct timeval final_time;
    gettimeofday(&final_time, NULL);
    //расчет общего времени работы программы
    if((final_time.tv_usec -= first_send_time.tv_usec) < 0)
    {
        --final_time.tv_sec;
        final_time.tv_usec += 10000000;
    }
    total_time = (final_time.tv_sec - first_send_time.tv_sec) * 1000 + (double)final_time.tv_usec / 1000.0;
    //вывод статистики и запись ее в лог с обработкой возможных ошибок
    printf("\n--- %s ping statistics ---\n",input_domain.c_str());
    printf("%d packets transmitted, %d received, %.0f%% packet loss, time %.0f ms\n",
            send_pack_num, recv_pack_num, (double)(send_pack_num - recv_pack_num) / (double)send_pack_num,
            total_time);
    printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n", min_time, (double)sum_time / recv_pack_num, max_time);
    sprintf(logText, "\n--- %s ping statistics ---\n",input_domain.c_str());
    if (AddMessageToLog(logText)==-1)
    {
        DiagLog();
    }
    sprintf(logText, "%d packets transmitted, %d received, %.0f%% packet loss, time %.0f ms\n",
            send_pack_num, recv_pack_num, (double)(send_pack_num - recv_pack_num) / (double)send_pack_num,
            total_time);
    if (AddMessageToLog(logText)==-1) 
    {  
        DiagLog();  
    }    
    sprintf(logText, "rtt min/avg/max = %.3f/%.3f/%.3f ms\n", min_time, (double)sum_time / recv_pack_num, max_time);
    if (AddMessageToLog(logText)==-1) 
    {  
        DiagLog();  
    }    
    if (AddMessageToLog("Pinging is done correctly")==-1) 
    { 
        DiagLog();
    }    
}



#ifndef MYPING_PING_H
#define MYPING_PING_H

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <string.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define PACK_SIZE 32                

class Ping {
private:
    //переменные для хранения адреса
    std::string input_domain;       
    std::string backup_ip;          
    //дескриптор сокета
    int sock_fd;
    //максимальное время ожидания пакета
    int max_wait_time;              
    //переменные для статистики
    int send_pack_num;              
    int recv_pack_num;              
    int lost_pack_num;              
    //адреса сокетов
    struct sockaddr_in send_addr;   
    struct sockaddr_in recv_addr;   
    //размеры пакетов
    char send_pack[PACK_SIZE];      
    char recv_pack[PACK_SIZE + 20]; 
    //время отправки и получения пакетов
    struct timeval first_send_time; 
    struct timeval recv_time;       
    //переменные для статистики
    double min_time;
    double max_time;
    double sum_time;

    //создание пакета
    int GeneratePacket();
    //обработка пакета
    int ResolvePakcet(int pack_szie);
    //расчет контрольной суммы
    unsigned short CalculateCksum(unsigned short * send_pack, int pack_size);

public:
    //функция инициализации основных параметров
    Ping(const char * ip, int max_wait_time);
    //обработка закрытия сокета
    ~Ping();
    //создание сокета
    void CreateSocket();
    //отправка,поулучение пакетов и вывод статистики
    void SendPacket();
    void RecvPacket();
    void statistic();
};


#endif 

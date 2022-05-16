

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
    std::string input_domain;       
    std::string backup_ip;          

    int sock_fd;

    int max_wait_time;              

    int send_pack_num;              
    int recv_pack_num;              
    int lost_pack_num;              

    struct sockaddr_in send_addr;   
    struct sockaddr_in recv_addr;   

    char send_pack[PACK_SIZE];      
    char recv_pack[PACK_SIZE + 20]; 

    struct timeval first_send_time; 
    struct timeval recv_time;       

    double min_time;
    double max_time;
    double sum_time;


    int GeneratePacket();
    int ResolvePakcet(int pack_szie);

    unsigned short CalculateCksum(unsigned short * send_pack, int pack_size);

public:
    Ping(const char * ip, int max_wait_time);
    ~Ping();

    void CreateSocket();

    void SendPacket();
    void RecvPacket();
    void statistic();
};


#endif 

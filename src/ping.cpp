#include "ping.h"

Ping::Ping(const char * ip, int max_wait_time){
    this->input_domain = ip;

    this->max_wait_time = max_wait_time < 3 ? max_wait_time : 3;

    this->send_pack_num = 0;
    this->recv_pack_num = 0;
    this->lost_pack_num = 0;

    this->min_time = 0;
    this->max_time = 0;
    this->sum_time = 0;
}

Ping::~Ping() {
    if(close(sock_fd) == -1) {
        fprintf(stderr, "Close socket error:%s \n\a", strerror(errno));
        exit(1);
    }
}

void Ping::CreateSocket(){
    struct protoent * protocol;            
    unsigned long in_addr;                  
    struct hostent host_info, * host_pointer; 
    char buff[2048];                         
    int errnop = 0;                         

    if((protocol = getprotobyname("icmp")) == NULL){
        fprintf(stderr, "Get protocol error:%s \n\a", strerror(errno));
        exit(1);
    }
    if((sock_fd = socket(AF_INET, SOCK_RAW, protocol->p_proto)) == -1){
        fprintf(stderr, "Greate RAW socket error:%s \n\a", strerror(errno));
        exit(1);
    }

    setuid(getuid());

    send_addr.sin_family = AF_INET;

    if((in_addr = inet_addr(input_domain.c_str())) == INADDR_NONE){
        if(gethostbyname_r(input_domain.c_str(), &host_info, buff, sizeof(buff), &host_pointer, &errnop)){
            //非法域名
            fprintf(stderr, "Get host by name error:%s \n\a", strerror(errno));
            exit(1);
        } else{
            this->send_addr.sin_addr = *((struct in_addr *)host_pointer->h_addr);
        }
    } else{
        this->send_addr.sin_addr.s_addr = in_addr;
    }

    this->backup_ip = inet_ntoa(send_addr.sin_addr);

    printf("PING %s (%s) %d(%d) bytes of data.\n", input_domain.c_str(),
            backup_ip.c_str(), PACK_SIZE - 8, PACK_SIZE + 20);

    gettimeofday(&first_send_time, NULL);
}

unsigned short Ping::CalculateCksum(unsigned short * send_pack, int pack_size){
    int check_sum = 0;              
    int nleft = pack_size;          
    unsigned short * p = send_pack; 
    unsigned short temp;            

    while(nleft > 1){
        check_sum += *p++;          
        nleft -= 2;
    }
    if(nleft == 1){
        
        *(unsigned char *)&temp = *(unsigned char *)p;
        check_sum += temp;
    }

    check_sum = (check_sum >> 16) + (check_sum & 0xffff);  
    check_sum += (check_sum >> 16);                         
    temp = ~check_sum;              

    return temp;
}

int Ping::GeneratePacket()
{
    int pack_size;
    struct icmp * icmp_pointer;
    struct timeval * time_pointer;

    icmp_pointer = (struct icmp *)send_pack;

    icmp_pointer->icmp_type = ICMP_ECHO;
    icmp_pointer->icmp_code = 0;
    icmp_pointer->icmp_cksum = 0;          
    icmp_pointer->icmp_seq = send_pack_num + 1;
    icmp_pointer->icmp_id = getpid();       

    pack_size = PACK_SIZE;

    time_pointer = (struct timeval *)icmp_pointer->icmp_data;

    gettimeofday(time_pointer, NULL);

    icmp_pointer->icmp_cksum = CalculateCksum((unsigned short *)send_pack, pack_size);

    return pack_size;
}

void Ping::SendPacket() {
    int pack_size = GeneratePacket();

    if((sendto(sock_fd, send_pack, pack_size, 0, (const struct sockaddr *)&send_addr, sizeof(send_addr))) < 0){
        fprintf(stderr, "Sendto error:%s \n\a", strerror(errno));
        exit(1);
    }

    this->send_pack_num++;
}

int Ping::ResolvePakcet(int pack_size) {
    int icmp_len, ip_header_len;
    struct icmp * icmp_pointer;
    struct ip * ip_pointer = (struct ip *)recv_pack;
    double rtt;
    struct timeval * time_send;

    ip_header_len = ip_pointer->ip_hl << 2;                     
    icmp_pointer = (struct icmp *)(recv_pack + ip_header_len); 
    icmp_len = pack_size - ip_header_len;                       

    if(icmp_len < 8) {
        printf("received ICMP pack lenth:%d(%d) is error!\n", pack_size, icmp_len);
        lost_pack_num++;
        return -1;
    }
    if((icmp_pointer->icmp_type == ICMP_ECHOREPLY) &&
        (backup_ip == inet_ntoa(recv_addr.sin_addr)) &&
        (icmp_pointer->icmp_id == getpid())){

        time_send = (struct timeval *)icmp_pointer->icmp_data;

        if((recv_time.tv_usec -= time_send->tv_usec) < 0) {
            --recv_time.tv_sec;
            recv_time.tv_usec += 10000000;
        }

        rtt = (recv_time.tv_sec - time_send->tv_sec) * 1000 + (double)recv_time.tv_usec / 1000.0;

        if(rtt > (double)max_wait_time * 1000)
            rtt = max_time;

        if(min_time == 0 | rtt < min_time)
            min_time = rtt;
        if(rtt > max_time)
            max_time = rtt;

        sum_time += rtt;

        printf("%d byte from %s : icmp_seq=%u ttl=%d time=%.1fms\n",
               icmp_len,
               inet_ntoa(recv_addr.sin_addr),
               icmp_pointer->icmp_seq,
               ip_pointer->ip_ttl,
               rtt);

        recv_pack_num++;
    } else{
        printf("throw away the old package %d\tbyte from %s\ticmp_seq=%u\ticmp_id=%u\tpid=%d\n",
               icmp_len, inet_ntoa(recv_addr.sin_addr), icmp_pointer->icmp_seq,
               icmp_pointer->icmp_id, getpid());

        return -1;
    }

}

void Ping::RecvPacket() {
    int recv_size, fromlen;
    fromlen = sizeof(struct sockaddr);

    while(recv_pack_num + lost_pack_num < send_pack_num) {
        fd_set fds;
        FD_ZERO(&fds);              
        FD_SET(sock_fd, &fds);      

        int maxfd = sock_fd + 1;
        struct timeval timeout;
        timeout.tv_sec = this->max_wait_time;
        timeout.tv_usec = 0;

        
        int n = select(maxfd, NULL, &fds, NULL, &timeout);

        switch(n) {
            case -1:
                fprintf(stderr, "Select error:%s \n\a", strerror(errno));
                exit(1);
            case 0:
                printf("select time out, lost packet!\n");
                lost_pack_num++;
                break;
            default:
                
                if(FD_ISSET(sock_fd, &fds)) {
                    
                    if((recv_size = recvfrom(sock_fd, recv_pack, sizeof(recv_pack),
                            0, (struct sockaddr *)&recv_addr, (socklen_t *)&fromlen)) < 0) {
                        fprintf(stderr, "packet error(size:%d):%s \n\a", recv_size, strerror(errno));
                        lost_pack_num++;
                    } else{
                        
                        gettimeofday(&recv_time, NULL);

                        ResolvePakcet(recv_size);
                    }
                }
                break;
        }
    }
}

void Ping::statistic() {
    double total_time;
    struct timeval final_time;
    gettimeofday(&final_time, NULL);

    if((final_time.tv_usec -= first_send_time.tv_usec) < 0) {
        --final_time.tv_sec;
        final_time.tv_usec += 10000000;
    }
    total_time = (final_time.tv_sec - first_send_time.tv_sec) * 1000 + (double)final_time.tv_usec / 1000.0;

    printf("\n--- %s ping statistics ---\n",input_domain.c_str());
    printf("%d packets transmitted, %d received, %.0f%% packet loss, time %.0f ms\n",
            send_pack_num, recv_pack_num, (double)(send_pack_num - recv_pack_num) / (double)send_pack_num,
            total_time);
    printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n", min_time, (double)sum_time / recv_pack_num, max_time);


}

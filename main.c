#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<assert.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h> /* do obliczenia czasow */

#include<arpa/inet.h>
#include<netinet/ip_icmp.h>
#include<netinet/ip.h>

int pid;

/* obliczanie czasu */
long delta_time_table[3];
long receive_time;
struct timeval time_send, time_receive;

u_int16_t compute_icmp_checksum (const void *buff, int length);

struct icmp* get_icmp_header(u_int8_t* buffer){
    struct ip* ip_header = (struct ip*)buffer;
    u_int8_t* icmp_packet = buffer + 4 * ip_header->ip_hl;
    struct icmp* icmp_header = (struct icmp*) icmp_packet;
    return icmp_header;
}

int custom_receive(int sockfd, int pid, int ttl, char* addr_ip){
    printf("custom_receive\n");
    /* ustawienie descryptorów na sockfd */
    struct  sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);

    fd_set desc;
    FD_ZERO (&desc);
    FD_SET (sockfd, &desc);

    /* przygotowanie bufforu na odbieranie pakietów */
    u_int8_t buffer[IP_MAXPACKET];

    /* timer 1 sek dla select */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int packege_count = 0;
    bool isGood = false;

    int ready = select(sockfd + 1, &desc, NULL, NULL, &tv);
    if(ready < 0){
        fprintf(stderr,"err: %s", strerror(errno));
        return -1;
    }else if(ready == 0) {
        printf("* * *\n");
        return -1;
    }else{
        char ip_str[20];
        while(ready > 0){
            for(int i = 0; i < ready; i++){
                printf("for - ready: %d \n", ready);
                /* odebranie pakietow */
                ssize_t pack_size = recvfrom(sockfd, buffer, IP_MAXPACKET, 0, (struct sockaddr*)&addr, &addr_size);
                /* odczytywanie informacji z odebranych pakietow */
                if(pack_size < 0){
                    printf("error: negative size of packege!\n");
                    return -1;
                }

                struct icmp* icmp_header = get_icmp_header(buffer);
                if( icmp_header->icmp_type == 11)
                    icmp_header = (struct icmp*)get_icmp_header((uint8_t*)&(icmp_header->icmp_ip));
                
                int temp_recv_seq = icmp_header->icmp_seq;
                int rcv_ttl = temp_recv_seq / 10;

                printf("pid get: %d, pid: %d, ttl get: %d, ttl: %d\n", icmp_header->icmp_id, pid, rcv_ttl, ttl);
                if(icmp_header->icmp_id != pid || rcv_ttl != ttl )
                    continue;

                /* odczyt adresu ip */
                inet_ntop(AF_INET, &(addr.sin_addr), ip_str, sizeof(ip_str));
                if(strcmp(ip_str, addr_ip) == 0){
                    printf("%d. %s ", ttl, ip_str);
                    packege_count++;
                    printf ("ip_recv: %s, addr_ip: %s , count: %d", ip_str, addr_ip, packege_count);
                    if(packege_count == 3) isGood = true;
                }
            }
            ready = select (sockfd+1, &desc, NULL, NULL, &tv);
        }
    }
    printf("package_count: %d\n", packege_count);
    if(packege_count == 3 && isGood) return 12;
    else if(packege_count == 2 || packege_count == 1) printf("???\n");
    else if(packege_count == 0) printf("*\n");
    // else msek
    return 0;
}

// DONE
int custom_send(struct  sockaddr_in recipient,int sockfd, int ttl, int s_number){
    printf("custom_send\n");

    //przygotowanie danych do wyslania
    struct icmp header;
    header.icmp_type =  ICMP_ECHO;
    header.icmp_code = 0;
    header.icmp_hun.ih_idseq.icd_id = getpid();
    header.icmp_hun.ih_idseq.icd_seq = ttl * 10 + s_number;
    /*
        header.icmp_id = getpid();
        header.icmp_seq = runda;
    */
    printf("ttl: %d\n", ttl);
    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum(
        (u_int16_t*)&header, sizeof(header));

    //gniazdo
    setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    struct timeval stop, start;
    gettimeofday(&start, NULL);

    ssize_t bytes_sent = sendto(
        sockfd,
        &header,
        sizeof(header),
        0,
        (struct sockaddr*)&recipient,
        sizeof(recipient)
    );

    if(bytes_sent < 0){
        fprintf(stderr, "err: bytes_sent: %s\n", strerror(errno));
        return -1;
    }
    printf("wyslalem, bytes_sent: %ld\n", bytes_sent);
    return 0;

}

int main(int argc, char *argv[]){
    // definicje
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    char* receiver_ip = "";
    if(sockfd < 0){
        fprintf(stderr ,"err: socket error %s\n", strerror(errno));
        return -1;
    }
    if(argc == 2){
        char* input_ip = argv[1];

        struct sockaddr_in recipient;
        bzero(&recipient, sizeof(recipient));
        recipient.sin_port = htons(7);
        recipient.sin_family = AF_INET;
        int check = inet_pton(AF_INET, input_ip, &recipient.sin_addr);

        if(check == 0){
            fprintf(stderr, "err: invalid ip address!\n");
            return -1;
        }
        if(check < 0){
            fprintf(stderr, "err: %s\n", strerror(errno));
            return -1;
        }

        pid = getpid();
        for(int i = 1; i < 30; i++){
            int result;
            result = custom_send(recipient, sockfd, i, 1);
            if(result < 0) return result;

            result = custom_send(recipient, sockfd, i, 2);
            if(result < 0) return result;
            
            result = custom_send(recipient, sockfd, i, 3);
            if(result < 0) return result;

            result = custom_receive(sockfd, pid, i, input_ip);
            if(result < 0) return result;
            if(result == 12) return 0;
            //if(strcmp(receiver_ip, input_ip) == 0) return 0;
            //else if(result == 0) return 0;
        }
    }else printf("wrn: podales niepoprawna liczbe argumentow!\n");
    return 0;
}

u_int16_t compute_icmp_checksum (const void *buff, int length)
{
	u_int32_t sum;
	const u_int16_t* ptr = buff;
	assert (length % 2 == 0);
	for (sum = 0; length > 0; length -= 2)
		sum += *ptr++;
	sum = (sum >> 16) + (sum & 0xffff);
	return (u_int16_t)(~(sum + (sum >> 16)));
}

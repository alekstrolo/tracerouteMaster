#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<assert.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>

// Gniazdo
#include<arpa/inet.h>
#include<netinet/ip_icmp.h> //IP_MAXPACKET
#include<netinet/ip.h> //IP_MAXPACKET

int runda = 1;

bool validate_ip(char* ip_str);

u_int16_t compute_icmp_checksum (const void *buff, int length);

struct icmp* get_icmp_header(u_int8_t* buffer){
    struct ip* ip_header = (struct ip*)buffer;
    u_int8_t* icmp_packet = buffer + 4 * ip_header->ip_hl;
    struct icmp* icmp_header = (struct icmp*) icmp_packet;
    return icmp_header;
}

void custom_receive(struct  sockaddr_in addr, int sockfd){
    printf("custom_receive\n");
    fd_set desc;
    FD_ZERO (&desc);
    FD_SET (sockfd, &desc);

    u_int8_t buffer[IP_MAXPACKET];

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int ready = select(sockfd + 1, &desc, NULL, NULL, &tv);
    if(ready < 0) printf("err: ready below zero");
    else if(ready == 0) {
        printf("* * *\n");
        return;
    }else if(ready == -1){
        fprintf(stderr,"err: %s", strerror(errno));
        return;
    }

    printf("\n READY: %d \n", ready);

    //odczyt addr ip
    char sender_ip_str[20];

    inet_ntop(
        AF_INET,
        &(addr.sin_addr),
        sender_ip_str,
        sizeof(sender_ip_str)
    );
    printf("ip: %s\n", sender_ip_str);

    //buffer
    struct icmp* icmp_header = get_icmp_header(buffer);

    if(icmp_header->icmp_type == 11){
        icmp_header = get_icmp_header((uint8_t*)&icmp_header->icmp_ip);
    }

    printf("icmp_id: %d\n",icmp_header->icmp_id);

    int rcv_id = icmp_header->icmp_seq;
    int rcv_ttl = rcv_id / 3 + 1;

    printf("rcv id: %d, ttl: %d\n", rcv_id, rcv_ttl);
}

void custom_send(struct  sockaddr_in addr,int sockfd, int ttl){
    printf("custom_send\n");

    //przygotowanie danych do wyslania
    struct icmp header;
    header.icmp_type =  ICMP_ECHO;
    header.icmp_code = 0;
    header.icmp_hun.ih_idseq.icd_id = getpid();
    header.icmp_hun.ih_idseq.icd_seq = runda;
    /*
        header.icmp_id = getpid();
        header.icmp_seq = runda;
    */
    runda++;
    printf("runda: %d\n", runda);
    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum(
        (u_int16_t*)&header, sizeof(header));

    //odbiorca

/*
    struct sockaddr_in recipient;
    bzero(&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_ntop(AF_INET, addr.sin_addr, &recipient.sin_addr, );
*/
    //gniazdo
    setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    ssize_t bytes_sent = sendto(
        sockfd,
        &header,
        sizeof(header),
        0,
        (struct sockaddr*)&addr,
        sizeof(addr)
    );

    if(bytes_sent < 0){
        fprintf(stderr, "err: bytes_sent: %s\n", strerror(errno));
        return;
    }
    printf("wyslalem, bytes_sent: %ld\n", bytes_sent);

}

int main(int argc, char *argv[]){
    // definicje
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if(sockfd < 0){
        fprintf(stderr ,"err: socket error %s\n", strerror(errno));
        return -1;
    }
    if(argc == 2){
    char* input_ip = argv[1];
    
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(7);
    addr.sin_family = AF_INET;

    if(inet_pton(AF_INET, input_ip, &(addr.sin_addr)) != 1){
        printf("err: invalid ip addr!\n");
        return -1;
    }

    //if(validate_ip(input_ip)){
        for(int i = 1; i < 30; i++){
            custom_send(addr, sockfd, i);
            custom_receive(addr, sockfd);
        }
    //}else printf("wrn: podales niepoprawny addr ip");
    }else printf("wrn: podales niepoprawna liczbe argumentow!\n");

}

bool validate_ip(char* ip_str)
{
    if (ip_str == NULL)
        return false;
    int len = strlen(ip_str);
    int count = 0;

    int pom  = 0;
    int po = 0;
    for (int i = 0; i < len; i++){
        if (ip_str[i] == '.' || i + 1 == len){
            char subbuff[5];
            if(i + 1 == len) pom += 1;
            memcpy( subbuff, &ip_str[po], pom);
            subbuff[pom] = '\0';
            int part = atoi(subbuff);
            if(part < 0 && part > 255) return false;
            po += pom + 1;
            pom = 0;
            count++;
        }else
            pom++;
    }
    if(count == 4) return true;
    return false;
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

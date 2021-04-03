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

int custom_receive(int sockfd{
    printf("custom_receive\n");
    /* ustawienie descryptorów na sockfd */
    struct  sockaddr_in addr;
    ssize_t addr_size = sizeof(addr);

    fd_set desc;
    FD_ZERO (&desc);
    FD_SET (sockfd, &desc);

    /* przygotowanie bufforu na odbieranie pakietów */
    u_int8_t buffer[IP_MAXPACKET];

    /* timer 1 sek dla select */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    /* śmieci */
    int ttl = 1;

    /* zmienne pomocnicze */
    int packege_count = 0;


    int ready = select(sockfd + 1, &desc, NULL, NULL, &tv);
    if(ready < 0){
        fprintf(stderr,"err: %s", strerror(errno));
        return;
    }else if(ready == 0) {
        printf("* * *\n");
        return;
    }else{
        for(int i = 0; i < ready; i++){
            printf("ready: %d \n", ready);
            /* odebranie pakietow */
            long pack_size = recvfrom(sockfd, buffer, IP_MAXPACKET, 0, (struct sockaddr*)&addr, &addr_size);
            /* odczytywanie informacji z odebranych pakietow */
            if(pack_size < 0){
                printf("error: negative size of packet!\n");
                return;
            }

            struct ip* ip_header = (struct ip*) buffer;
            ssize_t	ip_header_len = 4 * ip_header->ip_hl;

            u_int8_t* icmp_packet = buffer + ip_header_len;
            struct icmp* icmp_header = (struct icmp*) icmp_packet;
            if( icmp_header->icmp_type == 11){

                ip_header = &(icmp_header->icmp_ip);
                ip_header_len = 4 * ip_header->ip_hl;

                icmp_packet = (u_int8_t*)ip_header + ip_header_len;
                icmp_header = (struct icmp*)icmp_packet;
            }

            int rcv_ttl = (icmp_header->icmp_seq - 1)/3 + 1;

            if(icmp_header->icmp_id != PID || rcv_ttl != ttl )
                continue;
            /* odczyt adresu ip */
            char ip_str[20]; 
            inet_ntop(AF_INET,
                &(addr.sin_addr),
                sender_ip_str,
                sizeof(ip_str));
            printf ("%d. %s ", ttl, ip_str);
            
            packege_count++;

            /*gettimeofday(&tvorig, (struct timezone *)NULL);
            tsorig = (tvorig.tv_sec % (24*60*60)) * 1000 + tvorig.tv_usec / 1000;
            tsrecv = ntohl(icmp_header->icmp_otime);
            avgTime += tsorig - tsrecv;*/
        }
        ready = select (sockfd+1, &descriptors, NULL, NULL, &tv);
    }
    if(packege_count == 3) return 0;
    printf("to less packeges!\n");
    /*
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

    printf("icmp_id: %d\n", icmp_header->icmp_id);

    int rcv_id = icmp_header->icmp_seq;
    int rcv_ttl = rcv_id / 3 + 1;

    printf("rcv id: %d, ttl: %d\n", rcv_id, rcv_ttl);*/
}

int custom_send(struct  sockaddr_in recipient,int sockfd, int ttl){
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

    //gniazdo
    setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

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

    //if(validate_ip(input_ip)){
        for(int i = 1; i < 30; i++){
            custom_send(recipient, sockfd, i);
            custom_receive(sockfd);
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

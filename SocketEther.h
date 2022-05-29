#include <vector>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pcap.h>
#include <iostream>
#include <ifaddrs.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <linux/if_arp.h>
#include "sendprogress.h"
#include "recieveprogress.h"
#include <QObject>

#define ETH_FRAME_SIZE 1518
#define ETH_P_NULL 0x0

#define SUCCESS (0)
#define UNKNOWN_ERROR (-1)
#define REQUEST_NOT_ACCEPT (-2)
#define RESPONSE_WAITING_TIME_OUT (-3)
#define ABORT (-4)
#define ERROR_CREATE_FILE (-5)

class SocketEther : public QObject{

    Q_OBJECT

private:
    int socket_descriptor;
    struct sockaddr_ll socket_address;

public:
    SocketEther( int index_interface){
        socket_descriptor = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        socket_address.sll_family   = PF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_IP);
        socket_address.sll_ifindex  = index_interface;
        socket_address.sll_hatype   = ARPHRD_ETHER;
        socket_address.sll_pkttype  = PACKET_OTHERHOST;
        socket_address.sll_halen    = ETH_ALEN;
    }

    SocketEther(){

    }

    int send_file(unsigned char* src_mac, unsigned char* dst_mac, char* path_file){
        FILE *file;
        file = fopen(path_file, "rb");
        if(!file)
        {
            perror("ERROR_File_Open");
            return -1;
        }
        char* name_file = get_file_name(path_file);
        fseek(file, 0, SEEK_END);
        signed long size = ftell(file);
        fseek(file, 0, SEEK_SET);    

        unsigned char* buffer = nullptr;
        buffer = (unsigned char*)malloc(ETH_FRAME_SIZE);                        //Frame
        unsigned char* etherhead = buffer;	                                    //Pointer to ethernet header
        unsigned char* data = buffer + 14;                                      //Pointer to data
        struct ethhdr *eh = (struct ethhdr *)etherhead;                         //Pointer to ethernet header
        //set destination mac address
        socket_address.sll_addr[0]  = dst_mac[0];
        socket_address.sll_addr[1]  = dst_mac[1];
        socket_address.sll_addr[2]  = dst_mac[2];
        socket_address.sll_addr[3]  = dst_mac[3];
        socket_address.sll_addr[4]  = dst_mac[4];
        socket_address.sll_addr[5]  = dst_mac[5];
        socket_address.sll_addr[6]  = 0x00;
        socket_address.sll_addr[7]  = 0x00;
        bool send_file_name = false;
        bool end_send_file = false;

        do {
            //prepare buffer
            memcpy((void *) buffer, (void *) dst_mac, ETH_ALEN);
            memcpy((void *) (buffer + ETH_ALEN), (void *) src_mac, ETH_ALEN);
            eh->h_proto = ETH_P_NULL;
            //fill data into frame
            long size_to_send = 1500;
            if(size<0)
            {
                memcpy(data, "END_DATA", 9);
                size_to_send =9;
                end_send_file = true;
            }
            else {
                if (send_file_name) {
                    if (size_to_send > size)
                        size_to_send = size;
                    size -= 1500;
                    fread(data, sizeof(char), size_to_send, file);
                } else {
                    memcpy(data, name_file, strlen(name_file) + 1);
                    size_to_send = strlen(name_file) + 1;
                }
            }

            //send data
            int sent = sendto(socket_descriptor, buffer, size_to_send + ETH_HLEN, 0, (struct sockaddr *) &socket_address,
                              sizeof(socket_address));
            if (sent == -1) {
                perror("ERROR_Send_Data");
                return UNKNOWN_ERROR;
            }
            //Wait for response packet 30 sec
            time_t now = time(0), tlimit = now + 30;
            while(1) {
                int length = recvfrom(socket_descriptor, buffer, ETH_FRAME_SIZE, 0, NULL, NULL);
                if (length == -1) {
                    perror("ERROR_Answer_Destination");
                    return UNKNOWN_ERROR;
                }

                if (eh->h_proto == ETH_P_NULL && memcmp( (const void*)eh->h_dest, (const void*)src_mac, ETH_ALEN) == 0 && memcmp( (const void*)eh->h_source, (const void*)dst_mac, ETH_ALEN) == 0)
                {
                    if(!send_file_name)
                    {
                        if(memcmp(data,"YES",3)==0) {
                            send_file_name = true;
                            emit set_range_signal(size);
                            break;
                        }
                        else
                        {
                            puts("REQUEST_NOT_ACCEPT");
                            return REQUEST_NOT_ACCEPT;
                        }
                    }
                    else
                    {
                        if(memcmp(data, "OK", 2)==0) {
                            emit set_progress_send_signal(size_to_send);
                            break;
                        } else {
                            return ABORT;
                        }
                    }
                }

                if ((now = time(0)) > tlimit)
                {
                    puts("RESPONSE_WAITING_TIME_OUT");
                    return RESPONSE_WAITING_TIME_OUT;
                }
            }
        }while(!end_send_file);
        return SUCCESS;
    }

    int receive_file(unsigned char *src_mac, unsigned char* dst_mac, char*file_name)
    {
        unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_SIZE);     //Frame
        unsigned char* etherhead = buffer;	                                    //Pointer to ethernet header
        unsigned char* data = buffer + 14;                                      //Pointer to data
        struct ethhdr *eh = (struct ethhdr *)etherhead;                         //Pointer to ethernet header

        FILE *f;
        f = fopen(file_name, "wb");
        if(!f)
        {
            perror("ERROR_Create_File");
            return ERROR_CREATE_FILE;
        }
        fclose(f);

        time_t now = time(0), tlimit = now + 5;
        while (1) {
            int length = recvfrom(socket_descriptor, buffer, ETH_FRAME_SIZE, 0, NULL, NULL);
            if (length == -1) {
                perror("ERROR_RECEIVE_PACKET");
                return UNKNOWN_ERROR;
            }

            if (eh->h_proto == ETH_P_NULL && memcmp( (const void*)eh->h_dest, (const void*)src_mac, ETH_ALEN) == 0 && memcmp( (const void*)eh->h_source, (const void*)dst_mac, ETH_ALEN) == 0) {
                now = time(0), tlimit = now + 5;
                emit set_progress_recieve_signal(length);
                if(memcmp(data, "END_DATA", 8)==0) {
                    if(send_message(src_mac, dst_mac, (char *)"OK")!=0)
                        return UNKNOWN_ERROR;
                    return SUCCESS;
                }
                else {
                    if(memcmp(data, "ABORT", 5)==0)
                        return ABORT;
                    f = fopen(file_name, "ab");
                    fwrite(data, sizeof(char), length - 14, f);
                    fclose(f);
                }

                if(send_message(src_mac, dst_mac, (char *)"OK")!=0)
                    return UNKNOWN_ERROR;
            }

            if ((now = time(0)) > tlimit)
            {
                puts("RESPONSE_WAITING_TIME_OUT");
                return RESPONSE_WAITING_TIME_OUT;
            }
        }
    }

    unsigned char* listen(unsigned char* src_mac)
    {
        unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_SIZE);     //Frame
        unsigned char* etherhead = buffer;	                                    //Pointer to ethernet header
        unsigned char* data = buffer + 14;                                      //Pointer to data
        struct ethhdr *eh = (struct ethhdr *)etherhead;                         //Pointer to ethernet header

        while (1) {
            int length = recvfrom(socket_descriptor, buffer, ETH_FRAME_SIZE, 0, NULL, NULL);
            if (length == -1) {
                perror("ERROR_RECEIVE_PACKET");
                return NULL;
            }

            if (eh->h_proto == ETH_P_NULL && memcmp( (const void*)eh->h_dest, (const void*)src_mac, ETH_ALEN) == 0 ) {
                return buffer;
            }
        }
    }

    int send_message(unsigned char* src_mac, unsigned char* dst_mac, char* message){
        unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_SIZE);          //Frame
        unsigned char* etherhead = buffer;	                                    //Pointer to ethernet header
        unsigned char* data = buffer + 14;                                      //Pointer to data
        struct ethhdr *eh = (struct ethhdr *)etherhead;                         //Pointer to ethernet header
        //set destination mac address
        socket_address.sll_addr[0]  = dst_mac[0];
        socket_address.sll_addr[1]  = dst_mac[1];
        socket_address.sll_addr[2]  = dst_mac[2];
        socket_address.sll_addr[3]  = dst_mac[3];
        socket_address.sll_addr[4]  = dst_mac[4];
        socket_address.sll_addr[5]  = dst_mac[5];
        socket_address.sll_addr[6]  = 0x00;
        socket_address.sll_addr[7]  = 0x00;

        eh->h_proto = ETH_P_NULL;

        memcpy(buffer, dst_mac, ETH_ALEN);
        memcpy(buffer + ETH_ALEN, src_mac, ETH_ALEN);
        memcpy(data, message, strlen(message)+1);

        int sent = sendto(socket_descriptor, buffer, (sizeof(message))+ETH_HLEN, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));
        if (sent == -1) {
            perror("ERROR_Send_Answer");
            return UNKNOWN_ERROR;
        }
        return SUCCESS;
    }

    void clear_socket(){
        unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_SIZE);
        for(int i=0; i<10;i++)
            recvfrom(socket_descriptor, buffer, ETH_FRAME_SIZE, MSG_DONTWAIT, NULL, NULL);
    }

signals:
    void set_progress_send_signal(int);
    void set_range_signal(int);
    void set_progress_recieve_signal(int);

private:

    char* get_file_name(char* path_file)
    {
        char* name_file;
        for(int i=strlen(path_file); i>=0; i--)
        {
            if(path_file[i] == '/' || i==0) {
                name_file = (char*) calloc(strlen(path_file)-i+10, sizeof(char));
                strcpy(name_file, "name_file/");
                int bg=i+1;
                if(i==0)
                    bg--;
                strcat(name_file, path_file+bg);
                name_file[strlen(path_file)-i+10-1] = '\0';
                break;
            }
        }
        return name_file;
    }
};


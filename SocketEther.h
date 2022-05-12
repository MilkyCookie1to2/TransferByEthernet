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

#define ETH_FRAME_SIZE 1518
#define ETH_P_NULL 0x0

class SocketEther{

private:
    int socket_descriptor;
    struct sockaddr_ll socket_address;

public:
    SocketEther(int index_interface){
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

    int send_file(unsigned char* src_mac, unsigned char* dst_mac, char* path_file, SendProgress *win){
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
        puts(name_file);
        unsigned char* buffer = nullptr;
        buffer = (unsigned char*)malloc(ETH_FRAME_SIZE);     //Frame
        puts("dfsd");
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
                return -1;
            }
            //Wait for response packet 30 sec
            time_t now = time(0), tlimit = now + 30;
            while(1) {
                int length = recvfrom(socket_descriptor, buffer, ETH_FRAME_SIZE, 0, NULL, NULL);
                if (length == -1) {
                    perror("ERROR_Answer_Destination");
                    return -1;
                }

                if (eh->h_proto == ETH_P_NULL && memcmp( (const void*)eh->h_dest, (const void*)src_mac, ETH_ALEN) == 0 )
                {
                    if(!send_file_name)
                    {
                        if(memcmp(data,"YES",3)==0) {
                            send_file_name = true;
                            win->set_range(size);
                            break;
                        }
                        else
                        {
                            puts("REQUEST_NOT_ACCEPT");
                            return -1;
                        }
                    }
                    else
                    {
                        if(memcmp(data, "OK", 2)==0) {
                            win->set_progress(size_to_send);
                            break;
                        }
                    }
                }

                if ((now = time(0)) > tlimit)
                {
                    puts("RESPONSE_WAITING_TIME_OUT");
                    return -1;
                }
            }
        }while(!end_send_file);
        return 0;
    }

    int receive_file(unsigned char *buffer_request, unsigned char* src_mac,unsigned char *response,char*file_name, RecieveProgress *win)
    {
        send_answer(buffer_request, src_mac, response);
        if(strcmp((char*)response, "NO")==0)
            return 0;
        unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_SIZE);     //Frame
        unsigned char* etherhead = buffer;	                                    //Pointer to ethernet header
        unsigned char* data = buffer + 14;                                      //Pointer to data
        struct ethhdr *eh = (struct ethhdr *)etherhead;                         //Pointer to ethernet header

        FILE *f;
        f = fopen(file_name, "wb");
        if(!f)
        {
            perror("ERROR_Create_File");
            return -1;
        }
        fclose(f);

        time_t now = time(0), tlimit = now + 5;
        while (1) {
            /*Wait for incoming packet...*/
            int length = recvfrom(socket_descriptor, buffer, ETH_FRAME_SIZE, 0, NULL, NULL);
            if (length == -1) {
                perror("ERROR_RECEIVE_PACKET");
                return NULL;
            }

            /*See if we should answer (Ethertype == 0x0 && destination address == our MAC)*/
            if (eh->h_proto == ETH_P_NULL && memcmp( (const void*)eh->h_dest, (const void*)src_mac, ETH_ALEN) == 0 ) {
                now = time(0), tlimit = now + 5;
                win->set_recieve_byte(length);
                if(memcmp(data, "END_DATA", 8)==0) {
                    if(send_answer(buffer, src_mac, (unsigned char*)"OK")!=0)
                        return -1;
                    return 0;
                }
                else {
                    f = fopen(file_name, "ab");
                    fwrite(data, sizeof(char), length - 14, f);
                    fclose(f);
                }

                if(send_answer(buffer, src_mac, (unsigned char*)"OK")!=0)
                    return -1;
            }

            if ((now = time(0)) > tlimit)
            {
                puts("RESPONSE_WAITING_TIME_OUT");
                return -1;
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
            /*Wait for incoming packet...*/
            int length = recvfrom(socket_descriptor, buffer, ETH_FRAME_SIZE, 0, NULL, NULL);
            if (length == -1) {
                perror("ERROR_RECEIVE_PACKET");
                return NULL;
            }

            /*See if we should answer (Ethertype == 0x0 && destination address == our MAC)*/
            if (eh->h_proto == ETH_P_NULL && memcmp( (const void*)eh->h_dest, (const void*)src_mac, ETH_ALEN) == 0 ) {
                return buffer;
            }
        }
    }

private:

    int send_answer(unsigned char* buffer, unsigned char* src_mac, unsigned char* answer)
    {
        unsigned char* etherhead = buffer;	                                    //Pointer to ethernet header
        unsigned char* data = buffer + 14;                                      //Pointer to data
        struct ethhdr *eh = (struct ethhdr *)etherhead;                         //Pointer to ethernet header

        /*exchange addresses in buffer*/
        memcpy( (void*)etherhead, (const void*)(etherhead+ETH_ALEN), ETH_ALEN);
        memcpy( (void*)(etherhead+ETH_ALEN), (const void*)src_mac, ETH_ALEN);
        memcpy(data, answer, sizeof answer);

        /*prepare sockaddr_ll*/
        socket_address.sll_addr[0]  = eh->h_dest[0];
        socket_address.sll_addr[1]  = eh->h_dest[1];
        socket_address.sll_addr[2]  = eh->h_dest[2];
        socket_address.sll_addr[3]  = eh->h_dest[3];
        socket_address.sll_addr[4]  = eh->h_dest[4];
        socket_address.sll_addr[5]  = eh->h_dest[5];
        socket_address.sll_addr[6]  = 0x00;
        socket_address.sll_addr[7]  = 0x00;

        eh->h_proto = ETH_P_NULL;
        /*send answer*/
        int sent = sendto(socket_descriptor, buffer, sizeof(answer)+ETH_HLEN, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));
        if (sent == -1) {
            perror("ERROR_Send_Answer");
            return -1;
        }
        return 0;
    }

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
                //strcat(name_file, "/");
                break;
            }
        }
        return name_file;
    }
};


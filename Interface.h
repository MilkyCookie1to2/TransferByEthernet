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

class Interface {
private:
    struct ifreq req;
    int s;

public:
    Interface(const char *name_interface){
        strcpy(req.ifr_name, name_interface);

        s = socket(AF_PACKET, SOCK_RAW, 0);
    }

    unsigned char* get_mac(){
        if(s<0)
            return NULL;
        unsigned char* mac = (unsigned char*) malloc(ETH_ALEN);
        if( ioctl( s, SIOCGIFHWADDR, &req ) != -1 )
        {
            memcpy(mac,(unsigned char*)req.ifr_ifru.ifru_hwaddr.sa_data, ETH_ALEN);
            return mac;
        }
        else
            return NULL;
    }

    int get_index(){
        if(s<0)
            return -1;
        if (ioctl(s, SIOCGIFINDEX, &req) == -1) {
            return -1;
        }
        return req.ifr_ifindex;
    }
};


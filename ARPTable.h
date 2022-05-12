#ifndef COURSEWORK_V4_ARPTABLE_H
#define COURSEWORK_V4_ARPTABLE_H

#include <vector>
#include <sys/socket.h>
#include <linux/if_arp.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pcap.h>
#include <iostream>

class ARPTable {
private:
    int s;
    struct arpreq arp_req;
    struct sockaddr_in *sin;

public:
    ARPTable(const char *name_interface){
        memset(&arp_req, 0, sizeof(arpreq));
        strcpy(arp_req.arp_dev, name_interface);

        s = socket(AF_INET, SOCK_DGRAM, 0);

        sin = (struct sockaddr_in *) &arp_req.arp_pa;
        sin->sin_family = AF_INET;
    }

    ~ARPTable(){
        close(s);
    }

    int get_ARP_table(std::vector<unsigned char *> &list_mac_addresses)
    {
        if (s < 0)
            return -1;
        std::string part_ip = "192.168.";
        for(int j=0; j<256;j++){
            for(int i=0;i<256; i++)
            {
                unsigned char *mac = get_mac_from_ip((part_ip + std::to_string(j) + "." + std::to_string(i)).c_str());
                if(mac!=NULL)
                    list_mac_addresses.push_back(mac);
            }
        }
        return 0;
    }

private:

    unsigned char * get_mac_from_ip(const char* ip)
    {
        sin->sin_addr.s_addr = inet_addr(ip);
        if (ioctl(s, SIOCGARP, &arp_req) < 0) {
            return NULL;
        }
        if (arp_req.arp_flags & ATF_COM) {
            unsigned char *mac = (unsigned char*) calloc(6, sizeof(unsigned char));
            memcpy(mac, arp_req.arp_ha.sa_data, 6);
            return mac;
        }
        else
            return NULL;
    }
};

#endif

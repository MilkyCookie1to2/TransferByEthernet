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

class ListInterfaces {
private:
    struct ifaddrs *addrs;

public:
    std::vector<const char*> getListInterfaces(){
        std::vector<const char*> list;
        getifaddrs(&addrs);
        while(addrs){
            if(addrs->ifa_addr && addrs->ifa_addr->sa_family == AF_PACKET)
                list.push_back(addrs->ifa_name);
            addrs = addrs->ifa_next;
        }
        freeifaddrs(addrs);
        return list;
    }
};


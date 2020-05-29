#include "std_include.hpp"

#include <sys/epoll.h>

int main(){
    int epfd = epoll_create(1);
    int i = epoll_wait(epfd, NULL, 1, -1);
    return 0;
}
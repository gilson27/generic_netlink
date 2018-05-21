#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#include <linux/genetlink.h>

/* Generic macros for dealing with netlink sockets. Might be duplicated
 * elsewhere. It is recommended that commercial grade applications use
 * libnl or libnetlink and use the interfaces provided by the library
 */
#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))
#define THREAD_COUNT 3
// #define MESSAGE_TO_KERNEL "Test Message"
#define MESSAGE_TO_KERNEL "Test"

typedef struct Send_Data{
	int id;
	int nl_fd;
	char family_name[32];
	char message[128];
}send_data_struct;

typedef struct Test_Data{
	int id;
} test_data_struct;

/**
	3 threads for send/receive 
*/
pthread_t rx_thread[3];
//Variables used for netlink
struct sockaddr_nl nl_address; //netlink socket address

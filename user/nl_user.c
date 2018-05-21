/**
	\File nl_user.c
	\Brief User space test application where three threads -
			connect to kernel modulke using netlink socket.
	\Date 27 Feb 2017
	\Author GV
*/

#include "nl_user.h"

void *handle_rx (void *send_data) {
	send_data_struct *t_send_data = (send_data_struct *) send_data;
	int id = t_send_data->id;
	int nl_family_id; //The family ID resolved by the netlink controller for this userspace program

	int nl_rxtx_length; //Number of bytes sent or received via send() or recv()
	struct nlattr *nl_na; //pointer to netlink attributes structure within the payload 
	struct { //memory for netlink request and response messages - headers are included
		struct nlmsghdr n;
		struct genlmsghdr g;
		char buf[256];
	} nl_request_msg, nl_response_msg;

	//Step 3. Resolve the family ID corresponding to the string "FAMILY_NAME"
	//Populate the netlink header
	nl_request_msg.n.nlmsg_type = GENL_ID_CTRL;
	nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
	nl_request_msg.n.nlmsg_seq = 0;
	nl_request_msg.n.nlmsg_pid = getpid();
	nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	//Populate the payload's "family header" : which in our case is genlmsghdr
	nl_request_msg.g.cmd = CTRL_CMD_GETFAMILY;
	nl_request_msg.g.version = 0x1;
	//Populate the payload's "netlink attributes"
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);
	nl_na->nla_type = CTRL_ATTR_FAMILY_NAME;
	nl_na->nla_len = strlen(t_send_data->family_name) + 1 + NLA_HDRLEN;
	strcpy(NLA_DATA(nl_na), t_send_data->family_name); //Family name length can be upto 16 chars including \0
	
	nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;

	/**
		Send the request
	*/

	nl_rxtx_length = sendto(t_send_data->nl_fd, (char *) &nl_request_msg, nl_request_msg.n.nlmsg_len,
							0, (struct sockaddr *) &nl_address, sizeof(nl_address));
	if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
		perror("sendto()");
		return NULL;
	}

	//Wait for the response message
	nl_rxtx_length = recv(t_send_data->nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
	if (nl_rxtx_length < 0) {
		perror("recv()");
		return NULL;
	}

	//Validate response message
	if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
		fprintf(stderr, "family ID request : invalid message\n");
		return NULL;
	}
	if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //error
		fprintf(stderr, "family ID request : receive error\n");
		return NULL;
	}

	//Extract family ID
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
	nl_na = (struct nlattr *) ((char *) nl_na + NLA_ALIGN(nl_na->nla_len));
	if (nl_na->nla_type == CTRL_ATTR_FAMILY_ID) {
		nl_family_id = *(__u16 *) NLA_DATA(nl_na);
	}

	//Step 4. Send own custom message
	memset(&nl_request_msg, 0, sizeof(nl_request_msg));
	memset(&nl_response_msg, 0, sizeof(nl_response_msg));

	nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	nl_request_msg.n.nlmsg_type = nl_family_id;
	nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
	nl_request_msg.n.nlmsg_seq = 60;
	nl_request_msg.n.nlmsg_pid = getpid();
	nl_request_msg.g.cmd = 1; //corresponds to FAMILY_C_RECV;
		
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_request_msg);
	nl_na->nla_type = 1; // corresponds to FAMILYx_A_MSG
	
	int test_int = 132;
	test_data_struct test;
	test.id = 5;

	nl_na->nla_len = sizeof(t_send_data->message)+NLA_HDRLEN; //Message length
	memcpy(NLA_DATA(nl_na), (void *)&t_send_data->message, sizeof(t_send_data->message));

	// nl_na->nla_len = sizeof(test_int)+NLA_HDRLEN; //Message length
	// memcpy(NLA_DATA(nl_na), (void *)&test_int, sizeof(test_int));

	// memcpy(NLA_DATA(nl_na), (void *)&test, sizeof(test));

	nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;

	/**
		Send custom message
	*/
	//Send the custom messagel
	nl_rxtx_length = sendto(t_send_data->nl_fd, (char *) &nl_request_msg, nl_request_msg.n.nlmsg_len,
							0, (struct sockaddr *) &nl_address, sizeof(nl_address));
	if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
		perror("sendto()");
		return NULL;
	}
	printf("Sent to kernel: %s\n",t_send_data->message);

	//Receive reply from kernel
	nl_rxtx_length = recv(t_send_data->nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
	if (nl_rxtx_length < 0) {
		perror("recv()");
		return NULL;
	}

	//Validate response message
	if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //Error
		printf("Error while receiving reply from kernel: NACK Received\n");
		return NULL;
	}
	if (nl_rxtx_length < 0) {
		printf("Error while receiving reply from kernel\n");
		return NULL;
	}
	if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
		printf("Error while receiving reply from kernel: Invalid Message\n");
		return NULL;
	}

	/**
		Parse the reply message
	*/
	nl_rxtx_length = GENLMSG_PAYLOAD(&nl_response_msg.n);
	nl_na = (struct nlattr *) GENLMSG_DATA(&nl_response_msg);
	printf("Kernel replied: %s\n",(char *)NLA_DATA(nl_na));
	return NULL;
}
/**
	\Brief Entry to program
			Creates 3 threads
			Runs 3 threads
	\return {int} Program exit status
*/
int main(void) {
	int i;
	send_data_struct send_data[3];
	memset(&nl_address, 0, sizeof(nl_address));
	nl_address.nl_family = AF_NETLINK;
	nl_address.nl_groups = 0;

	/**
		Create three sockets and rx threads
	*/
	for(i = 0; i < THREAD_COUNT; ++i) {
		send_data[i].nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
		send_data[i].id = i;
		sprintf(send_data[i].family_name, "FAMILY%d", i+1); 
		sprintf(send_data[i].message, "%s%d", MESSAGE_TO_KERNEL, i+1);
		if(send_data[i].nl_fd < 0) {
			fprintf(stderr, "Unable to create socket %d\n", i);
			continue;
		}
		
		if(bind(send_data[i].nl_fd, (struct sockaddr *) &nl_address, sizeof(nl_address)) < 0) {
			fprintf(stderr, "Unable to bind socket %d\n", i);
			close(send_data[i].nl_fd);
			continue;
		}

		pthread_create(&(rx_thread[i]), NULL, &handle_rx, (void *)&(send_data[i]));
	}




	//Send the family ID request message to the netlink controller




	/**
		Wait and lose all threads and sockets
	*/
	for(i = 0; i < THREAD_COUNT; ++i) {
		pthread_join(rx_thread[i], NULL);
		close(send_data[i].nl_fd);
	}

	//Step 5. Close the socket and quit

	return 0;
}
#include "mylibrary.h"

int main(int argc, char** argv) {

	char buf[BUFLEN], rbuf[BUFLEN];	// transmitter and receiver buffers
	SOCKET s; 											// socket
	struct in_addr sIPaddr;					// server IP address structure
	struct sockaddr_in saddr, caddr;// server address structure
	uint16_t tport_n, tport_h;			// server port number by htons()
	struct timeval tval;						// for setting timeout
	fd_set cset;										// socket set
	socklen_t caddr_len;						// socket length

	/* Check number of arguments */
	checkArg(argc, 3);

	/* Initialize the socket API (only for Windows) */
	SockStartup();
	
	/* Set IP address and port number of Server */
	setIParg(argv[1], &sIPaddr);
		// setIPin(&sIPaddr);
	tport_n = setPortarg(argv[2], &tport_h);
		//tport_n = setPortin(&tport_h);

	/* Create the socket */
	fprintf(stdout, "Creating the socket...\n");
	s = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	fprintf(stdout, "- OK. Socket fd: %d\n", s);
	
	/* Prepare client address structure */
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(0); // Ask for an unused port number
	caddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/* Bind the socket to any free UDP port */
	fprintf(stdout, "Binding the socket to any free port...\n");
	Bind(s, (struct sockaddr*) &caddr, sizeof(caddr));
	fprintf(stdout, "- OK. Socket bound to ");
	caddr_len = sizeof(caddr);
	getsockname(s, (struct sockaddr*) &caddr, &caddr_len);
	showAddress(&caddr);
	
	/* Prepare server address structure */
	saddr.sin_family = AF_INET;
	saddr.sin_port = tport_n;
	saddr.sin_addr = sIPaddr;
	
	br();
	
	/* Main */
	doSomething();
	
	br();
	
	/* Close the socket connection */
	fprintf(stdout, "Closing the socket connection...\n");
	closesocket(s);
	fprintf(stdout, "- OK. Closed.\n");
	
	/* Release resources (only for Windows) */
	SockCleanup();

	return 0;
}

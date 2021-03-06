#include "../../mylibrary.h"
#include "../types.h"

#define MSG_ERR "-ERR"
#define MSG_OK "+OK"
#define MSG_QUIT "QUIT\r\n"

int main(int argc, char** argv) {

	char buf[BUFLEN], rbuf[BUFLEN];	// transmitter and receiver buffers
	SOCKET s; 											// socket
	struct in_addr sIPaddr;					// server IP address structure
	struct sockaddr_in saddr; 			// server address structure
	uint16_t tport_n, tport_h;			// server port number by htons()
	char filename[FILELEN], fileDwnld[FILELEN], c;
	FILE* fp;
	int nread, i, xdrFlag = 0;
	uint32_t fileBytes, timeStamp;
	
	XDR xdrs_in;											// Input XDR stream 
	XDR xdrs_out;											// Output XDR stream 
	FILE* stream_socket_r;						// FILE stream for reading from the socket
	FILE* stream_socket_w;						// FILE stream for writing to the socket
	message reqMessage, resMessage;

	/* Check number of arguments */
	if (argc == 3) {
		/* Set IP address */
		setIParg(argv[1], &sIPaddr);
	
		/* Set port number  */
		tport_n = setPortarg(argv[2], &tport_h);

	} else if (argc == 4) {
	
		/* Use XDR */
		xdrFlag = 1;
	
		/* Set IP address */
		setIParg(argv[2], &sIPaddr);
	
		/* Set port number  */
		tport_n = setPortarg(argv[3], &tport_h);
		
	} else {
		fprintf(stderr, "ERROR. Wrong arguments. Syntax: %s [-x] address port.\n", argv[0]);
		return 1;
	}

	/* Create the socket */
	fprintf(stdout, "Creating the socket...\n");
	s = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	fprintf(stdout, "- OK. Socket fd: %d\n", s);
	
	/* Prepare server address structure */
	saddr.sin_family = AF_INET;
	saddr.sin_port = tport_n;
	saddr.sin_addr = sIPaddr;

	/* Send connection request */
	fprintf(stdout, "Connecting to target address...\n");
	Connect(s, (struct sockaddr*) &saddr, sizeof(saddr));
	fprintf(stdout, "- OK. Connected to ");
	showAddress(&saddr);
	
	br();
	
	/* Main */
	if (xdrFlag) {
	
		/* Open FILE reading stream and bind it to the corresponding XDR stream */
		stream_socket_r = fdopen(s, "r");
		if (stream_socket_r == NULL) {
			fprintf(stderr, "---ERROR. fdopen() failed.\n");
			return 1;
		}
		xdrstdio_create(&xdrs_in, stream_socket_r, XDR_DECODE);
	
		/* Open FILE writing stream and bind it to the corresponding XDR stream */
		stream_socket_w = fdopen(s, "w");
		if (stream_socket_w == NULL) {
			fprintf(stderr, "---ERROR. fdopen() failed.\n");
			xdr_destroy(&xdrs_in);
			fclose(stream_socket_r);
			return 1;
		}
		xdrstdio_create(&xdrs_out, stream_socket_w, XDR_ENCODE);
	
		while(1) {
			/* Ask for a filename */
			fprintf(stdout, "Insert a filename, 'end' or 'stop' to finish.\n");
			fscanf(stdin, "%s", filename);
			
			if (isEndOrStop(filename)) {
			
				/* End the communication */
				reqMessage.tag = QUIT;
				if (!xdr_message(&xdrs_out, &reqMessage)) {
					fprintf(stdout, "- ERROR sending QUIT message.\n");
					break;
				}
				fprintf(stdout, "- QUIT message sent.\n");
				fflush(stream_socket_w);
				break;
				
			} else {
			
				/* Send a file request */
				reqMessage.tag = GET;
				reqMessage.message_u.filename = filename;
				if (!xdr_message(&xdrs_out, &reqMessage)) {
					fprintf(stdout, "- ERROR sending GET message.\n");
					break;
				}
				fflush(stream_socket_w);
				
				/* Receive a message */
				resMessage.message_u.fdata.contents.contents_len = 0;
				resMessage.message_u.fdata.contents.contents_val = NULL;
				if (!xdr_message(&xdrs_in, &resMessage)) {
					fprintf(stdout, "- ERROR. Response xdr_message() failed.\n");
					break;
				}
				fprintf(stdout, "- Received message.\n");
				
				/* Compute the type of message received and behave according to it */
				if (resMessage.tag == OK) {
				
					fprintf(stdout, "- File received: %s\n", filename);
					fprintf(stdout, "- File size: %u\n", resMessage.message_u.fdata.contents.contents_len);
					fprintf(stdout, "- File timestamp: %u\n", resMessage.message_u.fdata.last_mod_time);
			
					sprintf(fileDwnld, "Dwnld_%s", filename); // filename of received file
				
					// Received and write file
					fp = Fopen(fileDwnld, "wb");
					Fwrite(resMessage.message_u.fdata.contents.contents_val, sizeof(char), resMessage.message_u.fdata.contents.contents_len, fp);
					Fclose(fp);
					fprintf(stdout, "- File written: %s\n", fileDwnld);
				
				} else if (resMessage.tag == ERR) {
					fprintf(stderr, "- Received ERR message.\n");
					break;
				} else {
					fprintf(stderr, "- ERROR. Something goes wrong with the communication protocol.\n");
					break;
				}
	
			}
			
			fflush(stream_socket_r);
			fflush(stream_socket_w);
		}
		
		xdr_destroy(&xdrs_in);
		fclose(stream_socket_r);
		xdr_destroy(&xdrs_out);
		fclose(stream_socket_w);
		
	} else {
	
		while(1) {
			/* Ask for a filename */
			fprintf(stdout, "Insert a filename, 'end' or 'stop' to finish.\n");
			fscanf(stdin, "%s", filename);
		
			if (isEndOrStop(filename)) {
				// End the communication
				sprintf(buf, MSG_QUIT);
				Write(s, buf, strlen(buf)*sizeof(char));
				fprintf(stdout, "- QUIT message sent.\n");
				break;
			} else {
				// Request a filename 
				sprintf(buf, "GET %s\r\n", filename);
				Write(s, buf, strlen(buf));
				fprintf(stdout, "- GET message sent.\n");
			
				// Receive a file
				nread = 0;
			
				do {
					Read(s, &c, sizeof(char));
					rbuf[nread++] = c;
				} while((c != '\n') && (nread < BUFLEN-1));
				rbuf[nread] = '\0';
			
				while((nread > 0) && ((rbuf[nread-1] == '\r') || (rbuf[nread-1] == '\n'))) {
					rbuf[nread-1] = '\0';
					nread--;
				}
			
				/* Compute the type of message received and behave according to it */
				if ((nread >= strlen(MSG_OK)) && (strncmp(rbuf, MSG_OK, strlen(MSG_OK)) == 0)) {
			
					fprintf(stdout, "--- File received: %s\n", filename);
			
					// OK, right response received
					sprintf(fileDwnld, "Dwnld_%s", filename); // filename of received file
				
					// Read the file size
					Read(s, rbuf, sizeof(uint32_t));
					fileBytes = ntohl(*(uint32_t*)rbuf);
					fprintf(stdout, "--- File size: %u\n", fileBytes);
				
					// Read the file timestamp
					Read(s, rbuf, sizeof(uint32_t));
					timeStamp = ntohl(*(uint32_t*)rbuf);
					fprintf(stdout, "--- File timestamp: %u\n", timeStamp);
				
					// Received and write file
					fp = Fopen(fileDwnld, "wb");
					for (i=0; i<fileBytes; i++) {
						Read(s, &c, sizeof(char));
						Fwrite(&c, sizeof(char), 1, fp);
					}
					Fclose(fp);
					fprintf(stdout, "--- File written: %s\n", fileDwnld);
				} else {
					// Protocol error
					fprintf(stderr, "--- ERROR. Something goes wrong with the communication protocol.\n");
					break;
				}
			}
		}
	}
	
	/* Close the socket connection */
	fprintf(stdout, "Closing the socket connection...\n");
	closesocket(s);
	fprintf(stdout, "- OK. Closed.\n");

	return 0;
}

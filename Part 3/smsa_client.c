////////////////////////////////////////////////////////////////////////////////
//
//  File          : smsa_client.c
//  Description   : This is the client side of the SMSA communication protocol.
//
//   Author        : Mohanish Sheth
//
//   Last Modified : 12/13/2013 
//

// Include Files
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// Project Include Files
#include <smsa_network.h>
#include <smsa.h>
#include <cmpsc311_log.h>

// Global variables
int Socket = -1;
int Flag, FlagW;
unsigned char buf[SMSA_NET_HEADER_SIZE+SMSA_BLOCK_SIZE];
int Length;

// Functional Prototypes
int Client_Connect (void);
void Construct ( unsigned char *block, uint32_t op);
int Deconstruct (uint16_t *len, uint32_t op);
//
// Functions
////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_client_operation
// Description  : This the client operation that sends a reques to the SMSA
//                server.   It will:
//
//                1) if mounting make a connection to the server 
//                2) send any request to the server, returning results
//                3) if unmounting, will close the connection
//
// Inputs       : op - the operation code for the command
//                block - the block to be read/writen from (READ/WRITE)
// Outputs      : 0 if successful, -1 if failure

int smsa_client_operation( uint32_t op, unsigned char *block ) {

	// Local variable 
	uint16_t len;
       	int rb = 0;

	// Declare variable to store the data from op code
	SMSA_DISK_COMMAND op_code = op>>26;

	// calling fucntion to extract data.
	if (op_code == SMSA_MOUNT){
		logMessage (LOG_INFO_LEVEL,"Connect to Server");
		// To check if the connect is already established.
		if (Socket == -1){
			if (Client_Connect() == -1){
			logMessage (LOG_INFO_LEVEL,"Error connecting to server.\n");
			return(-1);
			}
		}
	}
	
	// Assigning the size of len based on the op code.
	if (op_code == SMSA_DISK_WRITE){
		FlagW = 1;
		Flag = 1;
	}
	else 
		Flag=0;

	//Calling Calling a function that will construct the packet.
	Construct(block, op);

	// Calling write to write across the network.
	if (write (Socket, &buf, Length) < 0){
		logMessage (LOG_INFO_LEVEL, "Error sending packet over network.\n");
		return(-1);
	}	
	
	// To recieve what server did we call read
	// but this time if op is write we dont want to send buf
	// and if op is any thing other than write we want to send 
	// buf.
	if (op_code == SMSA_DISK_WRITE)
		Flag=0;
	else 
		Flag=1;

	// Calling constuct to create new package.
	Construct (block, op);

	// Calling read to read across the network.
	if (read(Socket, &buf, Length) < 0) {
		logMessage (LOG_INFO_LEVEL,"Error reading across the network.\n");
		return(-1);
	}

	// Calling a fucntion to recieve what we read across the network.
	if (Deconstruct(&len , op) == -1){
		logMessage (LOG_INFO_LEVEL,"Error recieveing packet across the network.\n");
		return(-1);
	}	
	
	// Copy buf from the packet to block.
	if (len > SMSA_NET_HEADER_SIZE){
		
		if (read (Socket, &block[rb], len-SMSA_NET_HEADER_SIZE-rb) < 0){
			logMessage (LOG_INFO_LEVEL, "Error reading the block.\n");
			return(-1);
		}
			
	}
	// Close the socket if the utility of this function is done.
	if (op_code == SMSA_UNMOUNT){
		close(Socket);
		Socket = -1;
	}
		
	// return 0 for success.
	return (0);
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : Client_Connect 
// Description  : This function makes connection to server.
//                
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int Client_Connect (void){
	
	struct sockaddr_in caddr;
	
	// Setting up the server and bind it to the define port
	// from smsa_network.h
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(SMSA_DEFAULT_PORT);
	
	// Connecting address to sever.
	if (inet_aton (SMSA_DEFAULT_IP, &caddr.sin_addr) == 0){
		logMessage (LOG_INFO_LEVEL,"Error in connecting address.\n");
		return(-1);
	}

	// Creating socket
	Socket = socket(PF_INET, SOCK_STREAM, 0);
	// Checking if socket created
	if (Socket == -1){
		logMessage (LOG_INFO_LEVEL, "Error creating socket.\n");
		return(-1);
	}

	// Connecting socket to server.
	if (connect (Socket, (const struct sockaddr *)&caddr, sizeof(struct sockaddr)) == -1){
		logMessage(LOG_INFO_LEVEL,"Error connecting to socket to server.\n");
		return(-1);
	}

	// return 0 for success
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : Construct
// Description  : This will create the package 
//
// Inputs       : buf - the buf which will hold the package.
// 		  op - the operation code for the command
//                block - the block to be read/writen from (READ/WRITE)
// Outputs      : none

void Construct ( unsigned char *block, uint32_t op){
	
	// local varialbes
	uint16_t len, ret;

	if (Flag == 1)
		len = SMSA_NET_HEADER_SIZE + SMSA_BLOCK_SIZE;
	else
		len = SMSA_NET_HEADER_SIZE;

	// Converting to network byte order
	len = htons(len);
	op = htonl(op);
	ret = htons(0);
	
	// storing the packet in buf.
	Length = 0;
	memcpy(&buf[Length], &len, sizeof(len)); 
	Length += sizeof(uint16_t);
	memcpy(&buf[Length], &op, sizeof(op));
	Length += sizeof(uint32_t);
	memcpy(&buf[Length], &ret, sizeof(ret));
	Length += sizeof(uint16_t);
	
	// If writing include the block to packet.
	if (Flag == 1 && FlagW == 1 && block != NULL){
	
		FlagW = 0;
		// Adding block to the packet.
		memcpy(&buf[Length], block, SMSA_BLOCK_SIZE);
		
		Length += SMSA_BLOCK_SIZE;
	}	
	
	return;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : Deconstruct 
// Description  : This will deconsrtuct the package recieved from the server. 
//
// Inputs       : buf - the buf which will hold the package.
// 		  op - the operation code for the command
//                block - the block to be read/writen from (READ/WRITE)
// Outputs      : 0 for success -1 for failure

int  Deconstruct (uint16_t *len, uint32_t op){
	
	// Local variables
	uint32_t op_code;
	uint16_t length, ret;
	
	// deconstructing the packet received from the server
	// and converting to host byte order.
	Length = 0;
	memcpy( &length, buf, sizeof(uint16_t));
	Length += sizeof(uint16_t);
	length = ntohs(length);
	memcpy(&op_code, &buf[Length], sizeof(uint32_t));
	Length+= sizeof(uint32_t);
	op_code = ntohs(op_code);
	memcpy (&ret, &buf[Length], sizeof(uint16_t));
	Length += sizeof(uint16_t);
	ret = ntohs(ret);
	
	// To check if the package was recieved was succesfull.
	if (ret != 0){
		logMessage (LOG_INFO_LEVEL, "Return value not equal to 0.\n");
		if (op_code != op)
			logMessage (LOG_INFO_LEVEL,"Differnet op codes.\n");
		return(-1);
	}

	// if this lenght is greater than header size 
	// that means buf is returned.
	*len = length;

	return(0);
}

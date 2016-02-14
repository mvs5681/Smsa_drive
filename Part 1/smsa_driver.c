////////////////////////////////////////////////////////////////////////////////
//
//  File           : smsa_driver.c
//  Description    : This is the driver for the SMSA simulator.
//
//   Author        : Mohanish Sheth
//   Last Modified : 
//

// Include Files

// Project Include Files
#include <smsa_driver.h>
#include <cmpsc311_log.h>

// Defines

// Functional Prototypes
uint32_t op_generator (SMSA_DISK_COMMAND op_code, SMSA_DRUM_ID Drum_id, SMSA_BLOCK_ID Block_id);
//
// Global data

// Interfaces

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_vmount
// Description  : Mount the SMSA disk array virtual address space
//
// Inputs       : none
// Outputs      : -1 if failure or 0 if successful

int smsa_vmount( void ) {
	// Calling a function that will generate a op code which will be used as a 
	// argument in the function smsa_operation.
	// declaring variable that will store the return value
	// from the function op_generator which is the op code genrated.
	//Calling op_generator and passing arguments the define operation code for
	//vmount, passing 0 for drum_id, unused bits and block_id.
	
	uint32_t op_code = op_generator(SMSA_MOUNT,0,0);
	
	// declaring varaible that will store the return value from
	//function smsa_operation. The return value is -1 or 0. 
	//Calling smsa_operation and passing the op_code as argument 
	//and passing Null pointer as its not being used
	int return_value = smsa_operation(op_code, NULL);
	
	if (return_value == -1){
		logMessage(LOG_INFO_LEVEL,"Error mounting disk:%d",return_value);
	}
	
	return(return_value);// Returning the value that is stored in the 
	// variable; -1 means error and 0 means success. 	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_vunmount
// Description  :  Unmount the SMSA disk array virtual address space
//
// Inputs       : none
// Outputs      : -1 if failure or 0 if successful

int smsa_vunmount( void )  {
// Calling a function that will generate a op code which will be used as a 
// argument in the function smsa_operation.
	// declaring variable that will store the return value
	// from the function op_generator which is the op code genrated.
	//Calling op_generator and passing arguments the define operation code for
	//vmount, passing 0 for drum_id, unused bits and block_id.
        uint32_t op_code = op_generator(SMSA_UNMOUNT,0,0);
	
	// declaring varaible that will store the return
	// value from function smsa_operation. The return value is -1 or 0. 
	//Calling smsa_operation and passing the op_code as argument 
	//and passing Null pointer as its not being used
	int return_value = smsa_operation(op_code, NULL);
	if (return_value == -1){
		logMessage(LOG_INFO_LEVEL,"Error mounting disk:%d",return_value);
	}
	return(return_value);// Returning the value that is stored in return_value. 
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_vread
// Description  : Read from the SMSA virtual address space
//
// Inputs       : addr - the address to read from
//                len - the number of bytes to read
//                buf - the place to put the read bytes
// Outputs      : -1 if failure or 0 if successful

int smsa_vread( SMSA_VIRTUAL_ADDRESS addr, uint32_t len, unsigned char *buf ) {
	
	// Decalaring variable that will control the loop later in this function.

	int rb = 0, i; // The rb is the number of readed bytes. 
	int return_value; // 
	
	logMessage (LOG_INFO_LEVEL,"Inside vread[%d]",rb);
	// Decalring a tempory char array which is pointed by the buff at every 
	// iteration of the loop.

	unsigned char Temp[SMSA_BLOCK_SIZE];
	// Decalring a variable that will help to check if len is within the range

	uint32_t end_point;

	// Extracting the drum id from the SMSA_VIRTUAL_ADDRESS 
	// Decalaring a variable that will hold the drum id from the function that 
	// extracts it from addr.
	// SMSA_DRUM_ID is the typedef which is being used here.

	SMSA_DRUM_ID drum_id = addr>>16;

	// Decalring a variable that will act like a flag for the SEEK_DRUM operation 
	// Intially it is set to 0 implying seek drum then if there is no need to 
	// seek drum it is set to 1.
	int Flag =0;	

	// Since the virtual address doen't guranty that its within the range, we 
	// have to check whether the drum id is within 0-15 if not we cannot terminate 
	// the function and return -1 for failure. 
	if (drum_id <0 || drum_id > 15){
	// Using the log function to display the error.
		logMessage (LOG_INFO_LEVEL, "The Drum_id from the addr is out of range: [%d]", drum_id);	
		return (-1);
	}

	// Similarly extracting the block id from the SMSA_VIRTUAL_ADDRESS. 
	
	SMSA_BLOCK_ID block_id = (addr & 0xFFFF)>>8;

	// The remaining bits in the virtual address is the offset bytes.
	uint32_t offset = (addr & 0xFF);

	// To check is len within the range add len to addr and the extract drum_id 
	//  and block_id from it the end point is withing the legal range than 
	//  len is within the range
	end_point = addr + len;

	if ((end_point>>16) >16 || (end_point>>16)<0){
		// Use the log function to determine the error
		logMessage (LOG_INFO_LEVEL,"The lenght is out of range:[%d]",end_point);

		return (-1);
	}
	

	// Create a loop that will run parallel to creation of op code and 
	// smsa_operation.
	do{
	
	if (Flag == 0){
	// Setting the flag= 1 as for next iteration we dont need to seek 
	// new drum, if it is needed flag will be set to 0 later in this 
	// function.
	
		Flag = 1;
	
	// Seek drum which we extracted from the addr.
	// Using op_code genterator function to create a op code for 
	// SEEK_DRUM.
	
	uint32_t op_seekd = op_generator (SMSA_SEEK_DRUM, drum_id,0);
	
	// Calling SMSA_operation to seek the drum id.
	
	return_value = smsa_operation (op_seekd , NULL);
	
	if (return_value == -1){
	// Use the log function 
	logMessage(LOG_INFO_LEVEL,"There was error seeking the drum[%d]",return_value);
	// terminate the program
		return (return_value);
	}
	}
	if (block_id <= (((end_point & 0xFFFF)>>8)) ){	
	// Similarly we have to seek the extracted block_id from the disk.
	
	uint32_t op_seekb = op_generator (SMSA_SEEK_BLOCK, 0,block_id);

	// Calling Smsa_operation to seek block id
	
	return_value = smsa_operation (op_seekb, NULL);

	if (return_value == -1){
	// Use the log function 
	logMessage(LOG_INFO_LEVEL,"There was error seeking block[%d]",return_value);
	// terminate the program
		return (return_value);
	}
	}
	// Now creating a op_code for VREAD function.
	
	uint32_t op_read = op_generator (SMSA_DISK_READ, drum_id,block_id);

	// Now calling the smsa operation and pass teh op_read and temp as
	// arguments. Temp stores number of bytes read from buf.
	
	return_value = smsa_operation (op_read , Temp);

	if (return_value == -1){
	// Use the log function 
	logMessage(LOG_INFO_LEVEL,"There was a error in reading[%d]",return_value);
	// terminate the program
		return (return_value);
	}

	// Assigning value to varible i which is a Temp array index.
	if (i < block_id){
		i = offset;
	}
	else{
		i=0; // Setting i =0 implies block_id is +1 than the previous
		     // block_id
		block_id++;
	}

	if (block_id > SMSA_MAX_BLOCK_ID){
		// if block id is  equal to greater than 255 implies new drum
		drum_id = drum_id + 1; // as disk is linear it seeks to  next
				       // drum. 
	
		// Setting the flag to 0 as we need to seek new drum
			Flag = 0;

		 // Since the drum changes the block_id has to start from 
		// zero. So setting block_id to zero.
		block_id = 0;
	}

	do{
		buf[rb] = Temp[i]; // Storing data in Temp array from buf.
		rb++; // Incremeanting rb
		i++;  // Incremeanting i 
	}while(i<SMSA_MAX_BLOCK_ID && rb<len);
	}while (rb<len);
	
	return (return_value);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_vwrite
// Description  : Write to the SMSA virtual address space
//
// Inputs       : addr - the address to write to
//                len - the number of bytes to write
//                buf - the place to read the read from to write
// Outputs      : -1 if failure or 0 if successful

int smsa_vwrite( SMSA_VIRTUAL_ADDRESS addr, uint32_t len, unsigned char *buf )  {

	// Decalaring variable that will control the loop later in this function.

	int rb = 0, i; // The rb is the number of readed bytes. 
	int return_value;

	unsigned char Temp[SMSA_BLOCK_SIZE];
	// Decalring a variable that will help to check if len is within the range

	uint32_t end_point;

	// Extracting the drum id from the SMSA_VIRTUAL_ADDRESS 
	SMSA_DRUM_ID drum_id = addr>>16;

	// Decalring a variable that will act like a flag for the SEEK_DRUM operation
	// and a flag for first block 
	int Flag_b =0, Flag_d = 1;	

 
	if (drum_id <0 || drum_id > 15){
	// Using the log function to display the error.
		logMessage (LOG_INFO_LEVEL, "The Drum_id from the addr is out of range: [%d]", drum_id);	
		return (-1);
	}

	// Similarly extracting the block id from the SMSA_VIRTUAL_ADDRESS. 
	
	SMSA_BLOCK_ID block_id = (addr & 0xFFFF)>>8;

	// The remaining bits in the virtual address is the offset bytes.
	uint32_t offset = (addr & 0xFF);
	
	end_point = addr + len;
	
	//for debugging purpose
	logMessage(LOG_INFO_LEVEL,"The offset is:[%d]",offset); 

	if ((end_point>>16) >16 || (end_point>>16)<0){
		// Use the log function to determine the error
		logMessage (LOG_INFO_LEVEL,"The lenght is out of range[%d]",end_point);

		return (-1);
	}
	
	// Seek drum for first block
	uint32_t op1 = op_generator (SMSA_SEEK_DRUM, drum_id, block_id);
	return_value = smsa_operation (op1, NULL);
	
	if (return_value == -1){
		logMessage(LOG_INFO_LEVEL,"Error in seeking drum");
		return(-1);
	}
	
	do{
		// To seek drum only if drum changes.
		if (Flag_d == 0){
		
			// Creating op code to seek drum
	  		 uint32_t seekd = op_generator (SMSA_SEEK_DRUM, drum_id, block_id);
	   		return_value = smsa_operation (seekd , NULL);
			Flag_d = 1;
		// Error checking
		if (return_value == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking drum.");
			return(-1);
		}
		}
		
		// Seeking block
		
		uint32_t seek = op_generator (SMSA_SEEK_BLOCK, drum_id, block_id);
	   	return_value = smsa_operation (seek, NULL);
		
		// Error checking
		if (return_value == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
		
		// Reading 
		uint32_t read = op_generator (SMSA_DISK_READ,drum_id,block_id);
		return_value = smsa_operation (read,Temp);
		// Error checking
		if (return_value == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
	
		// Seeking block for write
		
		 seek = op_generator (SMSA_SEEK_BLOCK, drum_id, block_id);
	   	return_value = smsa_operation (seek, NULL);
		
		// Error checking
		if (return_value == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
		// Setting the map index for Temp	
		if (Flag_b == 0){
			i = offset;
			Flag_b = 1;	
		}	
		else 
			i =0;
		do{
		    Temp[i] = buf[rb];
		    rb++; i++;
		}while (rb<len && i<SMSA_BLOCK_SIZE);
		
		// Writing to disk array.
	   	uint32_t write = op_generator (SMSA_DISK_WRITE, drum_id, block_id);
	   	return_value = smsa_operation (write,Temp);
	  	
		// Error checking
		if (return_value == -1){
		logMessage(LOG_INFO_LEVEL,"Error in writing to disk array.");
		return(-1);
		}
		
		block_id ++; // Incrementing block_id
		
		// If block id > 256 it implies new drum so increment drum by 1 
	       // setting flag_d = 0 so on the next iteration we seek new drum	
	 	 if (block_id > SMSA_MAX_BLOCK_ID -1){
	   		drum_id++;
	   		Flag_d = 0;
			block_id = 0;
	  	 }
		
				
	}while (rb<len );	
	return(return_value);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : op_generator
// Description  : Creates an op- code which used in smsa_operation function 
//
// Inputs       : op_code - Defined Disk Command
//                Drum_id - Drum number 
//                Unused  - Unused bits/for changes added on later
//                Block_id- Block number
// Outputs      : Returns the op code generated

uint32_t op_generator (SMSA_DISK_COMMAND op_code, SMSA_DRUM_ID Drum_id, SMSA_BLOCK_ID Block_id){
	
	// First storing op_code in a temporary variable and then shift left
	// by 26 bits.
	SMSA_DISK_COMMAND Temp0 = op_code<<26;

	// Similarly shifting Drum_id left by 22 bits.
	uint32_t Temp1 = Drum_id<<22;

		
	// Note the Block_id is not shifted because its bits are the least 
	// significant bits as defined by the structure of op code.
	
	// Now combining all four variables to get one 32 bit number,
	// by performing the bit or operation.
	// Decalring a variable that will hold the value after the OR operation
	uint32_t ADD = (Temp0 | Temp1 | Block_id);

	return (ADD); // Returning the generated op code.
}



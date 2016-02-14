////////////////////////////////////////////////////////////////////////////////
//
//  File           : smsa_driver.c
//  Description    : This is the driver for the SMSA simulator.
//
//   Author        : Mohanish Sheth
//   Last Modified : 
//

// Include Files
#include <stdint.h>
#include <stdlib.h>
// Project Include Files
#include <smsa_driver.h>
#include <cmpsc311_log.h>
#include <smsa_cache.h>

// Notes:
// Before reading call get cache line, if it returns null than read from the disk
// else return as the data already exist in the cache.
// After reading call put cache to store the new data in cache memory. 
// TO ask:
// use malloc to instead of temp array. 

// Defines

// Functional Prototypes
uint32_t op_generator (SMSA_DISK_COMMAND op_code, SMSA_DRUM_ID Drum_id, SMSA_BLOCK_ID Block_id);

int extract (SMSA_VIRTUAL_ADDRESS addr,SMSA_DRUM_ID *drum,SMSA_BLOCK_ID *block,uint32_t *offset);
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

int smsa_vmount( int lines ) {
		
	// Generate op_code for mounting the virtual disk.
	uint32_t op_code = op_generator(SMSA_MOUNT,0,0);
	
	// call intia cache and pass the # of line.
	if (smsa_init_cache (lines) == -1){
		logMessage(LOG_INFO_LEVEL, "Error in intializing cache.\n");
		return(-1);
	}
	
	//Calling smsa_operation and passing the op_code as argument 
	int return_value = smsa_operation(op_code, NULL);

	// Using log message to check if mounting was sucessfull or not.
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
	
	// Generate op_code for unmounting the virtual disk.
        uint32_t op_code = op_generator(SMSA_UNMOUNT,0,0);
		 
	// Call close cache to turn it off. 
	/*if (smsa_close_cache() == -1){
		logMessage (LOG_INFO_LEVEL,"Error closing the cache.\n");
		return(-1);
	}*/
	//Calling smsa_operation and passing the op_code as argument 
	int return_value = smsa_operation(op_code, NULL);

	// Using the log message to check if unmounting was succesfull or not.
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
	
	
	int rb = 0, i; // rb and i loop contollers.. 
		
	//unsigned char Temp[SMSA_BLOCK_SIZE];// Decalring a tempory char array 
	unsigned char *Temp=NULL;
	unsigned char *cache_ptr=NULL;
	int Flag =0;// To seek drum only when needed	
	
	// decalring variables that will hold drum,block and offset 
	SMSA_DRUM_ID drum_id;
	SMSA_BLOCK_ID block_id;
	uint32_t offset;

	// Calling a function that will extract the drum, block and offset from addr
	if (extract(addr,&drum_id,&block_id,&offset) == -1)
	{
		return(-1);// Error the addr is not in range.
	}
	
	// Using the log message to check if the len is within the range.	
	if ((((addr+len)>>16) >16 || (addr+len)>>16)<0){
		logMessage (LOG_INFO_LEVEL,"The lenght is out of range:[%d]",addr+len);
		return (-1);
	}
	
	//Creating a loop that will read through the virtual disk array
	do{
	
	// Call get cache line to see if its in cache memory.
	cache_ptr = smsa_get_cache_line (drum_id, block_id);

	// If funciton returns Null than read new data.
	if (cache_ptr == NULL){

	if (Flag == 0){

		Flag = 1;// So it doesn't seek drum again
	
	// Calling smsa operation to seek drum.
	if (smsa_operation (op_generator (SMSA_SEEK_DRUM, drum_id,0) , NULL) == -1){
		logMessage(LOG_INFO_LEVEL,"There was error seeking the drum[%d]",-1);
		return (-1);
	}
	}

	// Calling smsa operation to seek block
	if (smsa_operation (op_generator (SMSA_SEEK_BLOCK, 0,block_id), NULL) == -1){
		logMessage(LOG_INFO_LEVEL,"There was error seeking block[%d]",-1);
		return (-1);
	}
	
	// Use malloc to get memory from os and pass that to read.
	Temp = (unsigned char *)malloc(SMSA_BLOCK_SIZE);
	
	
	// Calling smsa operation to read the virtual disk array
	if(smsa_operation (op_generator (SMSA_DISK_READ, drum_id,block_id) , Temp) == -1){
		logMessage(LOG_INFO_LEVEL,"There was a error in reading[%d]",-1);
		return (-1);
	
		}
	
	}
	// else the pointer points to the line in cache.
	else{
		logMessage (LOG_INFO_LEVEL, "Get pointer not null.\n");
		Temp = cache_ptr;
	
	}

	// Assigning value to varible i which is a Temp array index.
	if (i < block_id)
		i = offset;
	
	else{
		i=0; // implying entering new block
		block_id++;//when in new block increment block_id
	}

	if (block_id > SMSA_MAX_BLOCK_ID){

		drum_id = drum_id + 1;// Incrementing drum_id if block_id>255.
		Flag = 0;// Need to seek new drum
		block_id = 0;// Entering new drum
	}

	do{
		buf[rb]=Temp[i]; // Storing data in Temp array from buf.
		rb++; // Incremeanting rb
		i++;  // Incremeanting i 
	}while(i<SMSA_MAX_BLOCK_ID && rb<len);


	// Call smsa_put_cache to update the cache memory.
	if (smsa_put_cache_line (drum_id, block_id, Temp) == -1){
		
		logMessage (LOG_INFO_LEVEL, "Error while putting in cache.\n");
		return(-1);
	}

	if (Temp != NULL){
		Temp = NULL;
	}
	
	}while (rb<len);
	
	return (0);
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

	
	int rb = 0, i; // rb and i are loop controllers 

	// variables for dynamic memory allocation	
	unsigned char *Temp=NULL;
	unsigned char *cache_ptr = NULL;
	
	// Decalring a variable that will act like a flag for the SEEK_DRUM operation
	// and a flag for first block 
	int Flag_b =0, Flag_d = 0;
	
	// decalring variables that will hold drum,block and offset 
	SMSA_DRUM_ID drum_id;
	SMSA_BLOCK_ID block_id;
	uint32_t offset;
	
	// Calling function to extract drum,block and offset from addr.
	if(extract(addr,&drum_id,&block_id,&offset) == -1){
		return(-1);
	}

	// Using log message to check if len is within the range. 
	if (((addr+len)>>16) >16 || ((addr+len)>>16)<0){
		logMessage (LOG_INFO_LEVEL,"The lenght is out of range[%d]",addr+len);
		return (-1);
	}
	

	do{
		// Calling function smsa-get cache to see if data already in cache memory.
		cache_ptr = smsa_get_cache_line (drum_id, block_id);
			
		if (cache_ptr == NULL ){	
			// if null do normal write function
			
		// malloc memory to allocate memory dynamically
		Temp = (unsigned char *)malloc(SMSA_BLOCK_SIZE);
		
		// Seek drum for first block and 
		// To seek drum only if drum changes.
		if (Flag_d == 0){
			Flag_d = 1;// To seek again when needed.
			
			// Calling smsa operation to seek drum.
	  		if(smsa_operation (op_generator (SMSA_SEEK_DRUM, drum_id, block_id) , NULL) == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking drum.");
			return(-1);
			}
		}
		
		// Calling Smsa operation to seek block for read
		if(smsa_operation (op_generator (SMSA_SEEK_BLOCK, drum_id, block_id), NULL) == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
		
		// Calling smsa operation to read 
		if(smsa_operation (op_generator (SMSA_DISK_READ,drum_id,block_id),Temp) == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
				
		
	}
		else 
		{
			logMessage (LOG_INFO_LEVEL, "cache ptr not null.\n");
			Temp = cache_ptr;
		}	
		
		// Calling smsa operation to seek block for write
		if(smsa_operation (op_generator (SMSA_SEEK_BLOCK, drum_id, block_id), NULL) == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
		// Setting the map index for Temp	
		if (Flag_b == 0){
			i = offset;// Writing for very first time
			Flag_b = 1;	
		}	
		else 
			i =0;
		do{
		    Temp[i] = buf[rb];
		    rb++; i++;
		}while (rb<len && i<SMSA_BLOCK_SIZE);
		
			// Calling smsa operation to write
	   	if(smsa_operation (op_generator (SMSA_DISK_WRITE, drum_id, block_id),Temp) == -1){
	  		logMessage(LOG_INFO_LEVEL,"Error in writing to disk array.");
			return(-1);
		}
		
		block_id ++; // Incrementing block_id
	
	 	 if (block_id > SMSA_MAX_BLOCK_ID -1){// implies new drum
	   		drum_id++;
	 		Flag_d = 0;// To seek new drum
			block_id = 0; // entering new drum
	  	 }
		
		
		if (Temp != NULL ){
			Temp = NULL;
		}

	}while (rb<len );	
	return(0);
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

////////////////////////////////////////////////////////////////////////////////
//
// Function     : extract
// Description  : Creates an op- code which used in smsa_operation function 
//
// Inputs       : addr, block, drum and offset
// 		
// Outputs      : Returns 0 if success or -1 for failure

int extract (SMSA_VIRTUAL_ADDRESS addr,SMSA_DRUM_ID *drum,SMSA_BLOCK_ID *block,uint32_t *offset){
	
	*drum = addr>>16; // Extracting drum

	// Using the log function to check if drum is within the range.
	if (*drum <0 || *drum > 15){
		logMessage (LOG_INFO_LEVEL, "The Drum_id from the addr is out of range: [%d]", drum);	
		return (-1);
	}

	*block = (addr & 0xFFFF)>>8;// Extracting block

	*offset = (addr & 0xFF);// Extracting offset

	return(0);
}

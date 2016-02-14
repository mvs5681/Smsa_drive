
//
//  File           : smsa_driver.c
//  Description    : This is the driver for the SMSA simulator.
//
//   Author        : Mohanish Sheth
//   Last Modified : 11/23/2013
//

// Include Files
#include <stdint.h>
#include <stdlib.h>
// Project Include Files
#include <smsa_driver.h>
#include <cmpsc311_log.h>
#include <smsa_cache.h>
#include <smsa_network.h>

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

int seek (SMSA_DRUM_ID drm, SMSA_BLOCK_ID blk);
//
// Global data
SMSA_DRUM_ID Cdrm;
SMSA_BLOCK_ID Cblk;
// Interfaces

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_vmount
// Description  : Mount the SMSA disk array virtual address space
//
// Inputs       : none
// Outputs      : -1 if failure or 0 if successful

int smsa_vmount( int lines ) {
		
	// call intia cache and pass the # of line.
	if (smsa_init_cache (lines) == -1){
		logMessage(LOG_INFO_LEVEL, "Error in intializing cache.\n");
		return(-1);
	}
	
	//Calling smsa_operation and passing the op_code as argument 
	if (smsa_client_operation(op_generator(SMSA_MOUNT,0,0),NULL) == -1){
		logMessage(LOG_INFO_LEVEL,"Error mounting disk:");
		return(-1);
	}
	
	// Setting the current drum and block to zero
	Cdrm = 0;
	Cblk = 0;
	
	return(0);// Returning the value that is stored in the 
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
	
	// Call close cache to turn it off. 
	/*if (smsa_close_cache() == -1){
		logMessage (LOG_INFO_LEVEL,"Error closing the cache.\n");
		return(-1);
	}*/

	// Calling Smsa operation to unmount the disk.
	if (smsa_client_operation(op_generator(SMSA_UNMOUNT,0,0),NULL) == -1){
		logMessage(LOG_INFO_LEVEL,"Error mounting disk:");
		return(-1);
	}

	return(0);// Returning the value that is stored in return_value. 
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
	// Flag for setting i
	int Flag = 0;	
	//unsigned char Temp[SMSA_BLOCK_SIZE];// Decalring a tempory char array 
	unsigned char *Temp=NULL;
	unsigned char *cache_ptr=NULL;
	
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
	
	// Seeking drum and block for first time.
	 if (seek(drum_id, block_id) == -1){
	 	logMessage (LOG_INFO_LEVEL,"Error Seeking.\n");
		return(-1);
	 }	 
	//Creating a loop that will read through the virtual disk array
	do{

	// Assigning value to varible i which is a Temp array index.
	if (Flag == 0){
		Flag = 1;
		i = offset;
	}
	else
		i=0; // implying entering new block

	if (block_id > SMSA_MAX_BLOCK_ID-1){

		drum_id = drum_id + 1;// Incrementing drum_id if block_id>255.
		block_id = 0;// Entering new drum
		Cblk = 0;
		seek(drum_id,block_id);
	}	
	
	// Call get cache line to see if its in cache memory.
	cache_ptr = smsa_get_cache_line (drum_id, block_id);

	// If funciton returns Null than read new data.
	if (cache_ptr == NULL){

	// Use malloc to get memory from os and pass that to read.
	Temp = (unsigned char *)malloc(SMSA_BLOCK_SIZE);
		
	// Calling smsa operation to read the virtual disk array
	if(smsa_client_operation (op_generator (SMSA_DISK_READ, drum_id,block_id) , Temp) == -1){
		logMessage(LOG_INFO_LEVEL,"There was a error in reading[%d]",-1);
		return (-1);
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
	block_id++;
	Cblk++;
	
	}
	// else the pointer points to the line in cache.
	else{
		logMessage (LOG_INFO_LEVEL, "Get pointer not null.\n");
		Temp = cache_ptr;
		
		do{
		buf[rb]=Temp[i]; // Storing data in Temp array from buf.
		rb++; // Incremeanting rb
		i++;  // Incremeanting i 
		}while(i<SMSA_MAX_BLOCK_ID && rb<len);

		// seek next block 
		block_id++;
		Cblk++;
		smsa_client_operation(op_generator(SMSA_SEEK_BLOCK,0,block_id),NULL);
			
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
	
	// flag for first block 
	int Flag_b =0;
	
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
	
	if (seek (drum_id,block_id) == -1){
		logMessage (LOG_INFO_LEVEL,"Error in seeking.\n");
		return(-1);
	}

	do{
		// Setting the map index for Temp	
		if (Flag_b == 0){
			i = offset;// Writing for very first time
			Flag_b = 1;	
		}	
		else 
			i =0;
		
		if (block_id > SMSA_MAX_BLOCK_ID -1){// implies new drum
	   		drum_id++;
	 		block_id = 0; // entering new drum
			Cblk = 0;
			seek (drum_id, block_id);
	  	 }

		// Calling function smsa-get cache to see if data already in cache memory.
		cache_ptr = smsa_get_cache_line (drum_id, block_id);
			
		if (cache_ptr == NULL ){	
		// if null do normal write function
			
		// malloc memory to allocate memory dynamically
		Temp = (unsigned char *)malloc(SMSA_BLOCK_SIZE);
			
		// Calling smsa operation to read 
		if(smsa_client_operation (op_generator (SMSA_DISK_READ,drum_id,block_id),Temp) == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
			
		block_id++;// Increment block and Cblk.
		Cblk++;
	
		do{
		    Temp[i] = buf[rb];
		    rb++; i++;
		}while (rb<len && i<SMSA_BLOCK_SIZE);
		
		}
	
		else // if get cache returns pointer in the cache memory
		{
			Temp = cache_ptr;
			do{
			    Temp[i] = buf[rb];
			    rb++; i++;
			}while (rb<len && i<SMSA_BLOCK_SIZE);
			
			   block_id++;
			   Cblk++;
		}	
		
		// Calling smsa operation to seek block for write
		if(smsa_client_operation (op_generator (SMSA_SEEK_BLOCK, drum_id, block_id-1), NULL) == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
		
		// Calling smsa operation to write
	   	if(smsa_client_operation (op_generator (SMSA_DISK_WRITE, drum_id, block_id),Temp) == -1){
	  		logMessage(LOG_INFO_LEVEL,"Error in writing to disk array.");
			return(-1);
		}
		
		// Calling smsa put cache to update cache memory
		if (smsa_put_cache_line (drum_id, block_id-1, Temp) == -1){
			logMessage (LOG_INFO_LEVEL, "Error while putting in cache.\n");
			return(-1);
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
// Description  : extract drum,block and offset from the addr. 
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

////////////////////////////////////////////////////////////////////////////////
//
// Function     : seek
// Description  : seeks drum and block when needed. 
//
// Inputs       : drum and block
// 		
// Outputs      : Returns 0 if success or -1 for failure

int seek (SMSA_DRUM_ID drm , SMSA_BLOCK_ID blk){

	if (drm != Cdrm){
			
		// Calling smsa operation to seek drum.
		if(smsa_client_operation (op_generator (SMSA_SEEK_DRUM, drm, blk) , NULL) == -1){
		logMessage(LOG_INFO_LEVEL,"Error in seeking drum.");
		return(-1);
		}
	           
		   Cdrm = drm;
	  	   Cblk = 0;
	}
				
	
	if (blk != Cblk){
		if(smsa_client_operation (op_generator (SMSA_SEEK_BLOCK, drm, blk), NULL) == -1){
			logMessage(LOG_INFO_LEVEL,"Error in seeking block.");
			return(-1);
		}
			Cblk  = blk;
	}
	
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
//  File           : smsa_cache.c
//  Description    : This is the cache for the SMSA simulator.
//
//   Author        : Mohanish Sheth
//
//   Last Modified : 11/25/2013
//

// Include Files
#include <stdint.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Project Include Files
#include <smsa_cache.h>

// GlobalVariable 
SMSA_CACHE_LINE *Cache=NULL;
uint32_t NUM_Cache_Line; 

// Functions
void export (SMSA_DRUM_ID drm, SMSA_BLOCK_ID blk, unsigned char *buf, int index);

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_init_cache
// Description  : Setup the block cache
//
// Inputs       : lines - the number of cache entries to create
// Outputs      : 0 if successful test, -1 if failure

int smsa_init_cache( uint32_t lines ) {
	
	int i;

	// Use calloc to allocate memory
	Cache = calloc (lines, sizeof(SMSA_CACHE_LINE));
 	// setting to -1 
		for (i =0; i<NUM_Cache_Line; i++){
			Cache[i].drum = -1;
			Cache[i].block = -1;
		}	
	// storing the # lines in G.V
	NUM_Cache_Line = lines;
	
	// Using log message to check if cache points to null
	if (Cache == NULL){
		logMessage(LOG_INFO_LEVEL, "CACHE is pointing to NULL\n");
		return(-1);
	}
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_close_cache
// Description  : Clear cache and free associated memory
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int smsa_close_cache( void ) {
	
	int i;

	// Setting a loop that will free cache line and set it to null.
	for (i=0; i<NUM_Cache_Line; i++){
		if (Cache[i].line != NULL){	
		free(Cache[i].line);
		Cache[i].line = NULL;
		}
	}

	//Returning the allocated memory to OS.
	if (Cache != NULL){
	free(Cache);
	Cache=NULL;
	}
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_get_cache_line
// Description  : Check to see if the cache entry is available
//
// Inputs       : drm - the drum ID to look for
//                blk - the block ID to lookm for
// Outputs      : pointer to cache entry if found, NULL otherwise

unsigned char *smsa_get_cache_line( SMSA_DRUM_ID drm, SMSA_BLOCK_ID blk ) {
	
	int i; // i is loop controller & time is store return value. 

	// Setting up loop that will check for the drum_id and block_id in cache
	for (i=0; i<NUM_Cache_Line; i++){
	
		if (Cache[i].drum == drm && Cache[i].block == blk){
			
			// Updating the time in the struct for the cache LRU 
			// policy purpose
			if( gettimeofday(&Cache[i].used,NULL) == -1){
				logMessage (LOG_INFO_LEVEL, "Error updating time.\n");
			}
			
			// Returning the pointer pointing to that line in cache
			return(Cache[i].line);
		}
	}
	// if not found in cache return null.
	// use a log message to notify it is returning null.
		return(NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : smsa_put_cache_line
// Description  : Put a new line into the cache
//
// Inputs       : drm - the drum ID to place
//                blk - the block ID to lplace
//                buf - the buffer to put into the cache
// Outputs      : 0 if successful, -1 otherwise

int smsa_put_cache_line( SMSA_DRUM_ID drm, SMSA_BLOCK_ID blk, unsigned char *buf ) {
	
	// local variabels.
	int i, iLRU=0;
	long result;
	
	// looping through cache to find null and enter new data.
	for (i=0; i<NUM_Cache_Line; i++){
	
		if (Cache[i].line == NULL){
			
			// Calling a function to enter new data.
			export (drm,blk,buf,i);
			
			// Updating the time for LRU policy.
			if( gettimeofday(&Cache[i].used, NULL) == -1){
				logMessage (LOG_INFO_LEVEL, "Error updating time.\n");
				return(-1);
			}
			
			// Setting iLRU for LRU policy.
			iLRU = i;			
			return(0); // for success.	
		}	
	}

	// looping through loop if no space available then LRU kicks in saves the day.
	for(i=0; i<NUM_Cache_Line; i++){

		// Use the inbuilt function from util.c
		result = compareTimes (&Cache[iLRU].used , &Cache[i].used);
		
		// If Cache at i was LRU then set iLRU to i
		 if (result > 0)
			iLRU = i;
	}

	//Now calling the function to store new data.
	export(drm,blk,buf,iLRU);
	
	// updating the time stamp in struct for LRU policy.
	if( gettimeofday(&Cache[iLRU].used, NULL) == -1){
		logMessage(LOG_INFO_LEVEL, "Error in updating time stamp.\n");
		return(-1);
	}
		
	return(0); // for success
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : export 
// Description  : stores data in cache 
//
// Inputs       : drm - the drum ID to place
//                blk - the block ID to lplace
//                buf - the buffer to put into the cache
//                i   - the index number of the struct.
// Outputs      : void function

void export (SMSA_DRUM_ID drm, SMSA_BLOCK_ID blk, unsigned char *buf, int index){
	
	// Storing data in the struct.
	Cache[index].drum = drm;
	Cache[index].block = blk;
	Cache[index].line = buf;
	
	return;
}




/* 
 * Build tool
 *
 * Recursively descend into all subdirectories (starting from where program is run) looking for .c files.
 * In each directory, .c files compiled to .o files, and linked together into static library (.a).
 * All .a libraries moved to starting directory and linked into single executable.
 * 
 * Program is comprised of buildtool.c (this file) and buildtool.h.
 * For testing and demonstration purposes, testmain.c, testbuildtool.h, and testBuildTool (dir) are included.
 * 
 * See readme.txt for more information.
 * 
 * @author= ChristopherPouch
 * 2021
 */

#include <stdio.h>
#include "buildtool.h"
#include <unistd.h>   
#include <sys/stat.h> 
#include <dirent.h>
#include <string.h> 
#include <errno.h>
#include <stdlib.h> 
#include <libgen.h> 

//Absolute top directory path (where the program started). 
//All .a and .o will be moved here for linking.
char cwdTop[1000*sizeof(char)]; 

//Temporary initial form of cwdTop. Needs formatting to ensure it works for system() call.
//For example, accounting for possible spaces in directory names.
char cwdTopTemp[1000*sizeof(char)]; 


/*
 * Main method
 * Start recursive search for .c files.
 * Process those files, then link the results into single executable.
 */
int main(int argc, char *argv[]){

	//Get the cwd path (where the executable is run from), the top level where all .a will be moved to.
	getcwd(cwdTopTemp, sizeof(cwdTopTemp));

	//The return from getcwd() may be broken if any directories have spaces in their names.
	//Therefore we need to format by adding quotes.
	formatTopDirPath(cwdTopTemp); //Result will be copied to cwdTop.
	
	printf("\n\nEntering directory subsystem...\n");
	printf("Creating .a, .o files...\n");
	
	//Starting from CWD, find and process all .c files in the subsystem. 
	recursiveDescent("."); 

	//All .c files have been converted to .o or .a and moved to top directory.
	//In that top directory, link them all together
	linkAll();
	
	//Many .o and .a files were created, but we don't need them anymore, 
	//because they are now represented by a single executable.
	//So delete all these .o and .a files.
	printf("\nCleaning up .a and .o files from subdirectories...");
	cleanup(".");
	cleanupTop(".");

	printf("\n\nAll processes completed.\nUse command ./buildtool to run executable.\n\n");
	return 0;
}


/**
 * Recursively descend into directory system.
 * Find and process .c files in subdirectories.
 */
int recursiveDescent(const char *path){
	
	int containsDotC; //Flag
	int ascendDir, descendDir; //Check chdir worked.
	DIR *cwd = opendir(path); //Pointer to current working directory
	struct dirent *dir; //Struct filled out and returned by readdir()
	
	//Loop until all directories in cwd have been entered
	while( (dir=readdir(cwd)) != NULL){

		//If the file is a directory (and not '.' or '..').
		if( dir->d_type==DT_DIR && strcmp(dir->d_name, ".")!=0 && strcmp(dir->d_name, "..")!=0 ){
			descendDir = chdir(dir->d_name); //New cwd
			
			if(descendDir==0){
				//Entered directory.
			}else{
				printf("Could not enter %s, %s\n", dir->d_name, strerror(errno));
			}
			//Now in new cwd, call descent again.
			recursiveDescent(".");
		}
	}
	//Process all .c files in cwd.
	//Only if we're not in the topmost (starting) directory.
	char locationn[1000*sizeof(char)]; //cwd
	getcwd(locationn, sizeof(locationn));
	if(strcmp(locationn, cwdTopTemp)!=0){
		containsDotC = 0; //Flag. Zero until we find a c file.
		rewinddir(cwd);
		while( (dir=readdir(cwd)) != NULL){

			//If it is a .c file, process.
			if(checkExtension(dir->d_name)==0){
				containsDotC = 1; //c file found, so below getDotA will be called.
				int compiled = getDotO(dir->d_name);  //Complie to .0	
				if(compiled == 1){
					printf("A problem occurred, terminating process. \n");
					return 1;
				}
			}
		}
	}
	//If cwd has .c files, make a .a static library.
	//But only if not in the top directory.
	char location[1000*sizeof(char)];
	getcwd(location, sizeof(location));
	if(containsDotC==1){
		if(strcmp(location, cwdTopTemp)==0){
			//Arrived back at the top, so do nothing, LinkAll() will do the rest.
		}else{
			getDotA(cwd);
		}
		
	}

	//Loop exited so there is nothing else to read in CWD.
	//Therefore we move up one level higher.
	//But only if we're NOT yet back to where we started.
	if(strcmp(location, cwdTopTemp)!=0){
		ascendDir = chdir( "../" );
	}

	int closed = closedir(cwd);
	if(closed!=0){
		printf("Error closing directory.");
	}

	return 0;
}


/**
 * Check whether a file name ends in .c
 * @return 0 for .c
 * @return 1 for .o
 * @return 2 for .a
 * @return 3 for other.
 */
int checkExtension(char *fileName){
	
	int n = strlen(fileName); 
	char z = fileName[n-1]; //Last char in file name.
	char y = fileName[n-2]; //Second to last char in file name.
	
	if(z=='c' && y=='.'){ 			  
		return 0; //File is .c
	}else if(z=='o' && y=='.'){
		return 1; //File is .o
	}else if(z=='a' && y=='.'){
		return 2; //File is .a
	}else{
		return 3;
	}  
	
}


/**
 * Compile .c file to .o
 * @param char* name of the .c file.
 * @return 0 for success, 1 for failure.
 */
int getDotO(char *name){

	int checkCompile;

	//Make complete compile command. Ex. "gcc -c file1.c"
	char command[ 50*sizeof(char) ] = "gcc -c \""; 
	strcat(command, name);
	strcat(command, "\""); //File name should be in quotes in case it has spaces.
	checkCompile = system(command); //Create .o file from .c

	//If a file failed to compile.
	if(checkCompile == -1){
		printf("%s failed to compile: %s", name, strerror(errno));
		return 1;
	}
	return 0; //Success.
}


/**
 * Create static library from all .o files in cwd.
 * Move the created .a to the starting directory.
 * @param string of all cwd .o files, separated by a space.
 * @return 0 for success, 1 for failure.
 */
int getDotA(DIR *cwd){
	
	int checkMake, checkMove;
	struct dirent *dir;
	char cwdString[150*sizeof(char)]; //Get simple name of cwd to name .a after
	char cwdPath[1000*sizeof(char)]; //Getcwd() returns NULL if path doesn't fit in here.
	char command[1000*sizeof(char)] = "ar -rc \"";
	char nameDotA[20*sizeof(char)] = "\"";
	
	//Get make command
	getcwd(cwdPath, sizeof(cwdPath)); //Full path of cwd.
	strcpy(cwdString, basename(cwdPath)); //Obtain simple name of cwd.
	strcat(nameDotA,cwdString); //For move.
	strcat(nameDotA, ".a\"" );  //For move.
	strcat(command, cwdString);
	strcat(command, ".a\""); //Close quote.

	//Get move command
	char moveCommand[1000*sizeof(char)] = "mv ";
	strcat(moveCommand, nameDotA);
	strcat(moveCommand, cwdTop);

	rewinddir(cwd); //The cwd passed in has already been looked through.
	while( (dir=readdir(cwd)) != NULL){

		//If it is a .o file, add the name to command string.
		if(checkExtension(dir->d_name)==1){
			strcat(command, " \"");
			strcat(command, dir->d_name);
			strcat(command, "\""); //Close quote.
		}
	}

	//printf("\nFinal ar command: %s\n", command);
	checkMake = system(command);
	checkMove = system(moveCommand);
	
	if(checkMake==-1){
		printf("Failed to create static library (.a): %s", strerror(errno));
	}
	if(checkMove==-1){
		printf("Failed to move static library: %s", strerror(errno));
	}

	return 0;
}


/**
 * Ensure path is valid by adding quotes. 
 * Ex: user/alexis sanchez becomes user/"alexis sanchez"
 * @param char *path
 */
void formatTopDirPath(char *path){

	int cwdIndex = 0;
	int pathIndex = 0;

	//Go through every char in path, insert quotes as needed.
	while( *(path+pathIndex) != '\0'  &&  *(path+pathIndex) != EOF){

		if( *(path+pathIndex) != '/' ){	
			*(cwdTop+cwdIndex) = *(path+pathIndex);
			pathIndex++;
			cwdIndex++;
		}else{
			*(cwdTop+cwdIndex) = '\"'; //Close quote
			cwdIndex++;

			*(cwdTop+cwdIndex) = '/'; //Forward slash
			cwdIndex++;

			*(cwdTop+cwdIndex) = '\"'; //Open quote
			cwdIndex++;
			pathIndex++;
		}

		*cwdTop = ' '; //Loose ends.
		*(cwdTop+cwdIndex) = '\"';	
	}
}


/**
 * Link all .o and .a files together into single executable.
 * Process only files in  cwd (cwd at the time of function call).
 */
int linkAll(){
	
	printf("\nLinking all .a and .o files into single executable.\n");
	printf("WARNINGS EXPECTED (will resolve when linking is complete.\n");

	//We are now back in top (starting) directory.
	DIR *top = opendir("."); //Pointer to current working directory
	struct dirent *topdir; //Struct filled out and returned by readdir()

	//Prepare system() command
	char linkCommand[1000*sizeof(char)] = "gcc";
	char name[20*sizeof(char)] = " -o buildtool";
	char tempFileName[20*sizeof(char)];

	//Look for .c files, get .o from them.
	while( (topdir=readdir(top)) != NULL){

		//Second condition: build tool is an outside file, should not be linked to project.
		if(checkExtension(topdir->d_name)==0 && strcmp(topdir->d_name, "buildtool.c")!=0 ){
			getDotO(topdir->d_name);
		}
	}

	rewinddir(top);
	while( (topdir=readdir(top)) != NULL){
		//If it's a .a or .o file.
		if(checkExtension(topdir->d_name)==1 || checkExtension(topdir->d_name)==2 ){

			//Add new name (in quotes) onto the command.
			strcpy(tempFileName, " \""); //Overwrite previous.
			strcat(tempFileName, topdir->d_name);
			strcat(tempFileName, "\"");
			strcat(linkCommand, tempFileName);
		}
	}
	strcat(linkCommand, name);
	
	//Link into single executable, buildtool.
	system(linkCommand);

	printf("\nLinking completed.\n");
	printf("WARNINGS EXPECTED (will resolve when linking is complete).\n");
	printf("\nExecutable \"buildtool\" created.");
	
	return 0;
}


/**
 * After buildtool executable is made, clean up all the .o and .a files.
 */
void cleanup(const char *path){

	DIR *cwd = opendir(path); //Pointer to current working directory
	struct dirent *dir; //Struct filled out and returned by readdir()

	//While loop until all directories in cwd have been entered
	while( (dir=readdir(cwd)) != NULL){

		//If the file is a directory (and not '.' or '..').
		if( dir->d_type==DT_DIR && strcmp(dir->d_name, ".")!=0 && strcmp(dir->d_name, "..")!=0 ){

			chdir(dir->d_name); //New cwd
			
			//Now in new cwd, call descent again.
			cleanup("."); //dir->d_name
		}
	}

	//If cwd has .o or .a files, delete them.
	//But only if not in the top directory.
	char location[1000*sizeof(char)];
	getcwd(location, sizeof(location));
	
	if(strcmp(location, cwdTopTemp)==0){
		//Back in top directory.
	}else{
		//system("rm *.a"); They all got moved up!
		system("rm *.o");
	}

	//Loop exited so there is nothing else to read in CWD.
	//Therefore we move up one level higher.
	//But only if we're NOT yet back to where we started.
	if(strcmp(location, cwdTopTemp)!=0){
		chdir( "../" );
	}

	int closed = closedir(cwd);
}


/**
 * Remove all the extra .a and .o files from top (starting) directory.
 */
void cleanupTop(const char *path){

	printf("\nCleaning up .a and .o files from top directory...");

	system("rm *.a");
	system("rm *.o");

	printf("\nDone with cleanup.\n");
}
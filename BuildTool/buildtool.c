#include <stdio.h>
#include "buildtool.h"
#include <unistd.h>   //For calling kernel functions
#include <sys/stat.h> //For using stat function
#include <dirent.h>
#include <string.h> //For strcpy()
#include <errno.h>
#include <stdlib.h> //For system()
#include <libgen.h> //Get the simple name (not path) of cwd.


char cwdTop[1000*sizeof(char)]; //"Functioning" top directory path.
char cwdTopTemp[1000*sizeof(char)]; //Returned from getcwd() top directory path.

/* 
 * Build tool
 * @author= chrispouch
 */
int main(int argc, char *argv[]){

	//Get the cwd path- the top level where all .a will be moved to.
	getcwd(cwdTopTemp, sizeof(cwdTopTemp));

	//The return from getcwd() may be broken if any directories have spaces in their names.
	//Therefore we need to format by adding quotes.
	formatTopDirPath(cwdTopTemp);
	
	printf("\n\nEntering directory subsystem...\n");
	printf("Creating .a, .o files...\n");
	recursiveDescent(".");

	//Temporary: check where we end..
	char end[5000*sizeof(char)];
	getcwd(end, sizeof(end));
	

	//Finally, link it all together.
	linkAll();
	
	printf("\nCleaning up .a and .o files from subdirectories...");
	cleanup(".");
	cleanupTop(".");

	printf("\n\nAll processes completed.\nUse command ./buildtool to run executable.\n\n");
	return 0;

}



/**
 * Recursively descends into file system.
 * Finds .c files in subdirectories
 */
int recursiveDescent(const char *path){
	
	int containsDotC;
	int ascendDir, descendDir; //Check chdir worked.
	DIR *cwd = opendir(path); //Pointer to current working directory
	struct dirent *dir; //Struct filled out and returned by readdir()

	
	//While loop until all directories in cwd have been entered
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
	//ONLY IF we're not in the topmost (starting) directory.
	char locationn[1000*sizeof(char)];
	getcwd(locationn, sizeof(locationn));
	if(strcmp(locationn, cwdTopTemp)!=0){
		containsDotC = 0; //Zero until we find a c file.
		rewinddir(cwd);
		while( (dir=readdir(cwd)) != NULL){

			//If it is a .c file, process.
			if(checkExtension(dir->d_name)==0){
				
				containsDotC = 1; //C file found, so below getDotA will be called.
				int compiled = getDotO(dir->d_name);  //COMPILE .O
				
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
			//Arrived back at the top.
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
 * Compile .c file to get .o
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
 * Link all the newly created .o and .a files together.
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
 * This can be avoided with -l input from user, but runs by default.
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

	//Je ne comprends pas...
	int closed = closedir(cwd);
}


/**
 * Remove all the extra .a and .o files from top (starting) directory.
 * This can be avoided with -l input from user, but runs by default.
 */
void cleanupTop(const char *path){

	printf("\nCleaning up .a and .o files from top directory...");

	system("rm *.a");
	system("rm *.o");

	printf("\nDone with cleanup.\n");
}
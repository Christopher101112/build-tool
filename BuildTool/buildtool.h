/*
 * Header file for all buildtool.c functions.
 */
#include <dirent.h> 


//Check whether a file name ends in .c
int checkExtension(char *fileName);

//Find all .c files in the directory tree, process them.
int recursiveDescent(const char *path);

//Compile a .c file to obtain .o file.
int getDotO(char *name);

//Put all cwd .o files into a .a static library.
int getDotA(DIR *cwd);

//Make sure the absolute path to top of file system is valid.
void formatTopDirPath(char *path);

//In the starting directory, link all the newly created .o and .a files together.
//Create one single executable.
int linkAll();

void cleanup(const char *path);

void cleanupTop(const char *path);
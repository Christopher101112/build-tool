# build-tool
Program to recursively descend into any directory system and compile all .c files to .o, linking them to create a single executable.

2021
Build Tool
@Author = ChristopherPouch





DEMONSTRATION

To see a 2-minute YouTube video explaining and demonstrating this program, follow this link: https://youtu.be/Cx9P7gWl-aU





CONTENTS

This program is fully functioning and is comprised of:
“buildtool.c”,
“buildtool.h”

For testing purposes, the following are included (but not necessary for program to work):
"testmain.c",
"testbuildtool.h",
"TestBuildTool" directory.





OVERVIEW

This program is designed to Recursively descend into a directory system (starting from where program is run) looking for .c files.

In each directory, .c files are compiled to .o files, and linked together into static libraries (.a). Then all .a libraries are moved to starting directory and linked into a single executable. The program ignores other types of files, as well as empty directories.

The program uses system() calls (which allow execution of command line commands) to create, move, and delete files. 





HOW TO RUN

*Program can only enter subdirectory system from where it is executed, ie cwd. 

*Program will fail if abnormalities are encountered in the subdirectory system: for example if the program encounters two separate main() methods. 

*Program relies on LINUX commands and only works on systems where such commands are recognized.

0) Read * notes above.
1) From command line, change your cwd to be the location of buildtool.c.
2) Make sure buildtool.h is in the same directory.
3) Compile buildtool.c (command "gcc buildtool.c"). a.out will be created in cwd.
4) Execute a.out (command "./a.out"). Buildtool program will run, creating single executable named "buildtool".
5) If you want, execute this single executable (command "./buildtool").

If you run the program in the provided directory, which contains subdirectory system for testing, you will see statements printed to the console as verification that it worked.




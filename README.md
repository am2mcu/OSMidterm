in this project our purpose is to recieve a file's info by getting the
address of the folder from user ,in linux.

the idea is to first create a process for the parent directory and then create process for each file ,
in parent directory. then for each file's subFile we create a thread to go through them and give info's 
like"(largest file path with it's size, smallest file path with it;s size, number of each extention in 
there,the file's total size , all the subFolders and extentions in the file's count).next we create a 
pipe for communication such that each thread give it's info to the process and then all the processes 
give their info to the parent process ,then in parent process we sum all the info's togheter and give 
largestFile path and size AND smallest file path and size AND count of all the files and extention AND
parent directory size AND count of each extention individual as an output to user.

here are the steps to do so:
1. include the necessary header files
2. declare global variables 
3. a mutex lock that ensures mutually exclusive access to shared resources when multiple threads are involved.
4. variables"largestFilePath" and "smallestFilePath" to store the path of the largest and smallest file
5. declare variables "off_t largestFileSize" and "off_t smallestFileSize" to store the size of largest and 
   smallest file
6. declare variable "off_t parentSize" to store the total size of parent directory
7. declare "int pipefd[2]" to establish a pipe for inter-process communication.
8. "countFiles" function to counts the number of files in a given directory and its subdirectories.
    It takes a directory path as an argument.
9. "first" function to create a thread for each process that counts the number of files in a given directory 
   and its subdirectories. It takes a directory path as an argument.
10. the "main" function is the entry of the program
11. The "folderPath" variable is assigned the path of the current directory.
12. A call to the "first()" function is made with folderPath as the argument.
13. The pipe's write end is closed.(close(pipefd[1]))
14. then A loop reads from the pipe and updates the fileCount variable with the count received from
    the child processes.
15. the pipe's read end is closed(close(pipefd[0]))
16. finally the total file count is printed

this project is a ncdu like project .((ncdu (NCurses Disk Usage) is a command line version of the most popular 
“du command“. It is based on ncurses and provides a fastest way to analyse and track what files and directories 
are using your disk space in Linux.)

for using this project you should either use linux operating system ,or you can download a virtual machine on your current
OS and then download UBUNTU on it .then you can use this project

// athurs: Armin Mazloumi & GhazalFiroozi (students from the Ferdowsi University of mashhad,Iran)
// recpective professor: DR. mohammad AllahBakhsh

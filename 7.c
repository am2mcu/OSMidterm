#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#define MAX_PATH_LENGTH 1024

#define folderPath "./"

off_t fileCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char* largestFilePath = NULL;
char* smallestFilePath = NULL; 
off_t largestFileSize = 0;
off_t smallestFileSize = __INT64_MAX__; 
off_t parentDirSize = 0;



int pipe_fd[2];

void sendToParent(off_t fc, off_t pds, off_t lfs, off_t sfs, const char* lfp, const char* sfp) {
    char buffer[4 * sizeof(off_t) + 2*MAX_PATH_LENGTH];
    
    memcpy(buffer, &fc, sizeof(off_t));
    memcpy(buffer + sizeof(off_t), &pds, sizeof(off_t));
    memcpy(buffer + 2*sizeof(off_t), &lfs, sizeof(off_t));
    memcpy(buffer+ 3*sizeof(off_t), &sfs, sizeof(off_t));
    strcpy(buffer + 4*sizeof(off_t), lfp);
    strcpy(buffer + 4*sizeof(off_t)+ MAX_PATH_LENGTH, sfp);
    

    write(pipe_fd[1], buffer, sizeof(buffer));
    
}

void receiveFromThreads() {

    char buffer[4 * sizeof(off_t) + 2 * 1024];  // Assuming file paths are limited to 1024 characters
    ssize_t bytesRead;
    
    printf("Total number of files in '%s': %d\n", folderPath, fileCount);

    while ((bytesRead = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
    	
    
        off_t fc, pds, lfs, sfs;
        memcpy(&fc, buffer, sizeof(off_t));
        memcpy(&pds, buffer + sizeof(off_t), sizeof(off_t));
        memcpy(&lfs, buffer + 2 * sizeof(off_t), sizeof(off_t));
        memcpy(&sfs, buffer + 3 * sizeof(off_t), sizeof(off_t));

        char lfp[MAX_PATH_LENGTH];
        char sfp[MAX_PATH_LENGTH];
        
        // Use strncpy to ensure null termination of the paths
        strncpy(lfp, buffer + 4 * sizeof(off_t), MAX_PATH_LENGTH);
        lfp[MAX_PATH_LENGTH] = '\0';  // Ensure null termination

        strncpy(sfp, buffer + 4 * sizeof(off_t) + MAX_PATH_LENGTH, MAX_PATH_LENGTH);
        sfp[MAX_PATH_LENGTH] = '\0';  // Ensure null termination

        // Process the received data as needed
        //printf("Received from thread - Count, Size, large, small: %ld, %ld, %ld, %ld, %s, %s\n", fc, pds, lfs, sfs, lfp, sfp);
        
        fileCount += fc;
    }
    
    printf("Total size of files in '%s': %d\n", folderPath, parentDirSize);
    printf("Total number of files in '%s': %d\n", folderPath, fileCount);
    printf("Largest file: %s (Size: %lld bytes)\n", largestFilePath, largestFileSize);
    printf("Smallest file: %s (Size: %lld bytes)\n", smallestFilePath, smallestFileSize);
}




void* countFiles(void* arg) {
    const char* directory = (const char*)arg;
    //printf("directory to new thread: %s\n", directory);
    
    
   

    DIR* dir = opendir(directory);
    if (dir == NULL) {
        printf("Unable to open directory '%s'.\n", directory);
        pthread_exit(NULL);
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
    //printf("seen: %s\n", entry->d_name);
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;  // Skip current directory and parent directory
        }
        

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);
        
      

        struct stat fileStat;
        if (stat(path, &fileStat) != -1) {
            if (S_ISDIR(fileStat.st_mode)) {
                pthread_t thread;
                
                //printf("path to new thread: %s\n", path);
                pthread_create(&thread, NULL, countFiles, (void*)path);  // Create a thread to count files in subdirectory
                pthread_join(thread, NULL);  // Wait for the thread to finish
                
                
            } else if (S_ISREG(fileStat.st_mode)) {
                pthread_mutex_lock(&mutex);
            	
                fileCount++;
                parentDirSize += fileStat.st_size;
                
                

                if (fileStat.st_size > largestFileSize) {
                    largestFileSize = fileStat.st_size;
                    
                    if (largestFilePath != NULL) {
                        free(largestFilePath);
                    }
                    largestFilePath = strdup(path);
                }
    
                if (fileStat.st_size < smallestFileSize) {
                    smallestFileSize = fileStat.st_size;
                    
                    if (smallestFilePath != NULL) {
                        free(smallestFilePath);
                    }
                    smallestFilePath = strdup(path);
                    
                }
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    
    closedir(dir);
    
    //printf("%s, %s\n", largestFilePath, smallestFilePath);
    sendToParent(fileCount, parentDirSize, largestFileSize, smallestFileSize, largestFilePath, smallestFilePath);

    pthread_exit(NULL);
}





void* first(void* arg) {
    const char* directory = (const char*)arg;
    
    

    DIR* dir = opendir(directory);
    if (dir == NULL) {
        printf("Unable to open directory '%s'.\n", directory);
        pthread_exit(NULL);
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;  // Skip current directory and parent directory
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        struct stat fileStat;
        if (stat(path, &fileStat) != -1) {
            if (S_ISDIR(fileStat.st_mode)) {
                pid_t pid = fork();
                if (pid < 0) {
					fprintf(stderr, "Fork failed\n");
					return 1;
				} else if (pid == 0) {
					pthread_t thread;
					pthread_create(&thread, NULL, countFiles, (void*)path);  // Create a thread to count files in subdirectory
				    pthread_join(thread, NULL);  // Wait for the thread to finish
				    exit(0);
				} else {
					// Parent process
					wait(NULL);
				}
            } else if (S_ISREG(fileStat.st_mode)) {
                pthread_mutex_lock(&mutex);
                fileCount++;
                parentDirSize += fileStat.st_size;
    
                if (fileStat.st_size > largestFileSize) {
                    largestFileSize = fileStat.st_size;
                    if (largestFilePath != NULL) {
                        free(largestFilePath);
                    }
                    largestFilePath = strdup(path);
                }
    
                if (fileStat.st_size < smallestFileSize) {
                    smallestFileSize = fileStat.st_size;
                    if (smallestFilePath != NULL) {
                        free(smallestFilePath);
                    }
                    smallestFilePath = strdup(path);
                }
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    

    closedir(dir);
    

	printf("Total size of files in '%s': %d\n", directory, parentDirSize);
    printf("Total number of files in '%s': %d\n", directory, fileCount);
    printf("Largest file: %s (Size: %lld bytes)\n", largestFilePath, largestFileSize);
    printf("Smallest file: %s (Size: %lld bytes)\n", smallestFilePath, smallestFileSize);
    
    

    close(pipe_fd[1]);  // Close the write end of the pipe in the main process
    receiveFromThreads();
    
    
    
    

    if (largestFilePath != NULL) {
        free(largestFilePath);
    }
    
    if (smallestFilePath != NULL) {
        free(smallestFilePath);
    }


}




int main() {
    
    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        return 1;
    }
    
    first(folderPath);
    
    

    return 0;
}

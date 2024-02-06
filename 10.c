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

off_t txt = 0;
off_t pdf = 0;
off_t jpg = 0;
off_t png = 0;

off_t fileCountP = 0;
char* largestFilePathP = NULL;
char* smallestFilePathP = NULL; 
off_t largestFileSizeP = 0;
off_t smallestFileSizeP = __INT64_MAX__; 
off_t parentDirSizeP = 0;

off_t txtP = 0;
off_t pdfP = 0;
off_t jpgP = 0;
off_t pngP = 0;



int pipe_fd[2];

void sendToParent(off_t fc, off_t pds, off_t lfs, off_t sfs, const char* lfp, const char* sfp, off_t txt, off_t pdf, off_t jpg, off_t png) {
    char buffer[4 * sizeof(off_t) + 2*MAX_PATH_LENGTH];
    
    memcpy(buffer, &fc, sizeof(off_t));
    memcpy(buffer + sizeof(off_t), &pds, sizeof(off_t));
    memcpy(buffer + 2*sizeof(off_t), &lfs, sizeof(off_t));
    memcpy(buffer+ 3*sizeof(off_t), &sfs, sizeof(off_t));
    memcpy(buffer+ 4*sizeof(off_t), &txt, sizeof(off_t));
    memcpy(buffer+ 5*sizeof(off_t), &pdf, sizeof(off_t));
    memcpy(buffer+ 6*sizeof(off_t), &jpg, sizeof(off_t));
    memcpy(buffer+ 7*sizeof(off_t), &png, sizeof(off_t));
    strcpy(buffer + 8*sizeof(off_t), lfp);
    strcpy(buffer + 8*sizeof(off_t)+ MAX_PATH_LENGTH, sfp);
    

    write(pipe_fd[1], buffer, sizeof(buffer));
    
}

void receiveFromThreads() {

    char buffer[4 * sizeof(off_t) + 2 * 1024];  // Assuming file paths are limited to 1024 characters
    ssize_t bytesRead;
    
    //printf("Total number of files in '%s': %d\n", folderPath, fileCount);

    while ((bytesRead = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
    	//printf("!!!!!!!!!!!!%d\n", fileCountP);
    
        off_t fc, pds, lfs, sfs, txtF, pdfF, jpgF, pngF;
        memcpy(&fc, buffer, sizeof(off_t));
        memcpy(&pds, buffer + sizeof(off_t), sizeof(off_t));
        memcpy(&lfs, buffer + 2 * sizeof(off_t), sizeof(off_t));
        memcpy(&sfs, buffer + 3 * sizeof(off_t), sizeof(off_t));

        memcpy(&txtF, buffer + 4 * sizeof(off_t), sizeof(off_t));
        memcpy(&pdfF, buffer + 5 * sizeof(off_t), sizeof(off_t));
        memcpy(&jpgF, buffer + 6 * sizeof(off_t), sizeof(off_t));
        memcpy(&pngF, buffer + 7 * sizeof(off_t), sizeof(off_t));

        char lfp[MAX_PATH_LENGTH];
        char sfp[MAX_PATH_LENGTH];
        
        // Use strncpy to ensure null termination of the paths
        strncpy(lfp, buffer + 8 * sizeof(off_t), MAX_PATH_LENGTH);
        lfp[MAX_PATH_LENGTH] = '\0';  // Ensure null termination

        strncpy(sfp, buffer + 8 * sizeof(off_t) + MAX_PATH_LENGTH, MAX_PATH_LENGTH);
        sfp[MAX_PATH_LENGTH] = '\0';  // Ensure null termination


        // Process the received data as needed
        //printf("Received from thread - Count, Size, large, small: %ld, %ld, %ld, %ld, %s, %s\n", fc, pds, lfs, sfs, lfp, sfp);
        
        fileCountP += fc;
        parentDirSizeP += pds;
        
        if (lfs > largestFileSizeP) {
            largestFileSizeP = lfs;
            
            if (largestFilePathP != NULL) {
                free(largestFilePathP);
            }
            largestFilePathP = strdup(lfp);
        }
    
        if (sfs < smallestFileSizeP) {
            smallestFileSizeP = sfs;
            
            if (smallestFilePathP != NULL) {
                free(smallestFilePathP);
            }
            smallestFilePathP = strdup(sfp);
            
        }
        
        txtP += txtF;
        pdfP += pdfF;
        jpgP += jpgF;
        pngP += pngF;
    }
    
    printf("Total size of files in '%s': %d\n", folderPath, parentDirSizeP);
    printf("Total number of files in '%s': %d\n", folderPath, fileCountP);
    printf("Largest file: %s (Size: %lld bytes)\n", largestFilePathP, largestFileSizeP);
    printf("Smallest file: %s (Size: %lld bytes)\n", smallestFilePathP, smallestFileSizeP);
    
    printf("Number of each file type in '%s':\n- .txt: %d\n- .pdf: %d\n- .jpg: %d\n- .png: %d\n", folderPath, txtP, pdfP, jpgP, pngP);
}




void* countFiles(void* arg) {
    const char* directory = (const char*)arg;
    //printf("directory to new thread: %s\n", directory);
    
    //printf("start@@@@@@@@@@@@@%d\n", fileCount);
   

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
                //printf("path to new thread: %s\n", path);
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
                
                
                char *extension = strrchr(entry->d_name, '.');
                //printf("%s\n", entry->d_name);
                if (extension != NULL) {
                	//printf("%s\n", extension);
                    if (strcmp(extension, ".txt") == 0) {
                        txt++;
                    }
                    else if (strcmp(extension, ".pdf") == 0) {
                        pdf++;
                    }
                    else if (strcmp(extension, ".jpg") == 0) {
                        jpg++;
                    }
                    else if (strcmp(extension, ".png") == 0) {
                        png++;
                    }
                }
                
                
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    //printf("@@@@@@@@@@@@@%d\n", fileCount);
    
    closedir(dir);
    
    //printf("%d, %d, %d, %d, %s, %s\n", fileCount, parentDirSize, largestFileSize, smallestFileSize, largestFilePath, smallestFilePath);
    sendToParent(fileCount, parentDirSize, largestFileSize, smallestFileSize, largestFilePath, smallestFilePath, txt, pdf, jpg, png);
    fileCount = 0;
    largestFilePath = NULL;
    smallestFilePath = NULL; 
    largestFileSize = 0;
    smallestFileSize = __INT64_MAX__; 
    parentDirSize = 0;
    txt = 0;
    pdf = 0;
    jpg = 0;
    png = 0;
	
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
                //printf("path to new thread: %s\n", path);
                fileCountP++;
                parentDirSizeP += fileStat.st_size;
    
                if (fileStat.st_size > largestFileSizeP) {
                    largestFileSizeP = fileStat.st_size;
                    if (largestFilePathP != NULL) {
                        free(largestFilePathP);
                    }
                    largestFilePathP = strdup(path);
                }
    
                if (fileStat.st_size < smallestFileSizeP) {
                    smallestFileSizeP = fileStat.st_size;
                    if (smallestFilePathP != NULL) {
                        free(smallestFilePathP);
                    }
                    smallestFilePathP = strdup(path);
                }
                
                char *extension = strrchr(entry->d_name, '.');
                if (extension != NULL) {
                    if (strcmp(extension, ".txt") == 0) {
                        txtP++;
                    }
                    else if (strcmp(extension, ".pdf") == 0) {
                        pdfP++;
                    }
                    else if (strcmp(extension, ".jpg") == 0) {
                        jpgP++;
                    }
                    else if (strcmp(extension, ".png") == 0) {
                        pngP++;
                    }
                }
                
                
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    

    closedir(dir);
    

    //printf("Total size of files in '%s': %d\n", directory, parentDirSize);
    //printf("Total number of files in '%s': %d\n", directory, fileCount);
    //printf("Largest file: %s (Size: %lld bytes)\n", largestFilePath, largestFileSize);
    //printf("Smallest file: %s (Size: %lld bytes)\n", smallestFilePath, smallestFileSize);
    
    //printf("\n");
    
    

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

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>



int fileCount = 0;  // Global variable to store the total file count
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char* largestFilePath = NULL;  // Global variable to store the path of the largest file
char* smallestFilePath = NULL;  // Global variable to store the path of the smallest file
off_t largestFileSize = 0;  // Global variable to store the size of the largest file
off_t smallestFileSize = __INT64_MAX__;  // Global variable to store the size of the smallest file
off_t parentDirSize = 0;

#define MAX_EXTENSIONS 100
const char *extensions[MAX_EXTENSIONS];
int counts[MAX_EXTENSIONS] = {0};
int extensionCount = 0;

int isExtensionExists(const char *extension, const char *extensions[], int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(extensions[i], extension) == 0) {
            return 1;
        }
    }
    return 0;
}

void printExtensionCounts(const char *extensions[], const int counts[], int count) {
    for (int i = 0; i < count; i++) {
        printf("Number of %s files: %d\n", extensions[i], counts[i]);
    }
}

void* countFiles(void* arg) {
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
                pthread_t thread;
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
                
                char *extension = strrchr(entry->d_name, '.');
            	if (extension != NULL) {
                	if (!isExtensionExists(extension, extensions, extensionCount)) {
                    // New extension found, add it to the extensions array
                    extensions[extensionCount] = extension;
                    extensionCount++;
                }
                // Increment the count for the current extension
                for (int i = 0; i < extensionCount; i++) {
                    if (strcmp(extension, extensions[i]) == 0) {
                        counts[i]++;
                        break;
                    }
                }
            }
                
                pthread_mutex_unlock(&mutex);
            }
        }
    }

    closedir(dir);

    pthread_exit(NULL);
}

int main() {
    const char* folderPath = "./";

    pthread_t mainThread;
    pthread_create(&mainThread, NULL, countFiles, (void*)folderPath);  // Create a thread for the main directory
    pthread_join(mainThread, NULL);  // Wait for the main thread to finish

    printf("Total number of files in '%s': %d\n", folderPath, parentDirSize);
    printf("Total number of files in '%s': %d\n", folderPath, fileCount);
    printf("Largest file: %s (Size: %lld bytes)\n", largestFilePath, largestFileSize);
    printf("Smallest file: %s (Size: %lld bytes)\n", smallestFilePath, smallestFileSize);
    printExtensionCounts(extensions, counts, extensionCount);

    if (largestFilePath != NULL) {
        free(largestFilePath);
    }
    
    if (smallestFilePath != NULL) {
        free(smallestFilePath);
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>


void printErrorMessage() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));

}

void lsCommand(const char *path) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (dir == NULL) {
        //fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", path);
        printErrorMessage();;
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Exclude . and .. entries (hidden files)
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir);
}


void cdCommand(const char *path) {
    if (chdir(path) != 0) {
        /* 
        test error message
        perror("cd");
        printf("Failed to change directory to '%s'\n", path); 
        */
       // Must use given error message function
       printErrorMessage();;

    } 
    /* else {
        printf("Changed directory to '%s'\n", path);
    } */
}

// Function to handle redirection of "ls" command output
void redirectLsOutput(const char *sourcePath, const char *destinationPath) {
    FILE *destinationFile = fopen(destinationPath, "w");
    if (destinationFile == NULL) {
        //perror("Failed to create destination file");
        printErrorMessage();;
        return;
    }

    // Construct the command
    char command[200]; // Adjust the size as needed
    snprintf(command, sizeof(command), "ls %s", sourcePath);

    // Run the command and capture its output
    FILE *commandStream = popen(command, "r");
    if (commandStream == NULL) {
        //perror("Failed to run command");
        printErrorMessage();;
        fclose(destinationFile);
        return;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), commandStream) != NULL) {
        fputs(buffer, destinationFile); // Redirect command output to the file
    }

    fclose(commandStream);
    fclose(destinationFile);
}

void catFile(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        //perror("File not found");
        printErrorMessage();;
        return;
    }

    char ch;
    while ((ch = fgetc(file)) != EOF) {
        putchar(ch);
    }

    fclose(file);
}

void removeFile(const char *filePath) {
    if (remove(filePath) == 0) {
        //printf("File removed successfully: %s\n", filePath); // For my use
    } else {
        printErrorMessage();;
        //perror("Failed to remove file"); // For my use
    }
}

// Define a global PATH variable
char current[1024] = "";


void batchMode(const char *filePath) {
    //printf("bathmode\n");
        // Run with input file

        char full_file_path[2048]; // Declare full_file_path here with the initial value of filePath

        // Check if the currentPath is not empty (i.e., path is set)
        if (strlen(current) > 0) {
            // Check if filePath already contains current; if not, append current to full_file_path
            if (strstr(filePath, current) == filePath) {
                strcpy(full_file_path, filePath);
            } else {
                snprintf(full_file_path, sizeof(full_file_path), "%s/%s", current, filePath);
            }
        } else {
            // If current is empty, full_file_path is the same as filePath
            strcpy(full_file_path, filePath);
        }

        // Print the opening message
        /* printf("Opening file: %s\n", full_file_path); */

        // Open the file with the full path
        FILE *inputFile = fopen(full_file_path, "r");

        if (inputFile == NULL) {
            printErrorMessage();; // Handle the error as needed
            exit(1);
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&line, &len, inputFile)) != -1) {
            // Remove leading and trailing whitespace characters
            char *trimmedLine = line;
            while (isspace(*trimmedLine)) {
                trimmedLine++;
            }
            char *end = trimmedLine + strlen(trimmedLine) - 1;
            while (end > trimmedLine && isspace(*end)) {
                end--;
            }
            *(end + 1) = '\0';

            // Skip processing if the trimmed line is empty
            if (strlen(trimmedLine) == 0) { // TEST CASE 15
                continue;
            }

            // Tokenize to handle many spaces before second arg, e.g ls     ..
            char *token = strtok(trimmedLine, " \t");  // Tokenize using spaces and tabs

            // Check if the token is a filename
            if (access(token, F_OK) == 0 && access(token, X_OK) != 0) {
                /* printf("bathmoderecursion\n"); */
                batchMode(token); // Recursively call batchMode on the nested file
            }

            // Exit allows null characters after it only, but nothing else
            else if (strcmp(trimmedLine, "exit") == 0) {
                token = strtok(NULL, "");  // Get the rest of the line after "exit"
                if (token == NULL || strlen(token) == 0) {
                    break;
                } else {
                    printErrorMessage();;
                }
            }

            // Skip processing if the trimmed line is empty or contains only '&'
            else if (strlen(trimmedLine) == 0 || strcmp(trimmedLine, "&") == 0) {
                continue;
            }

            // TO DO: path
            // Check if the user entered the custom 'path' command
            else if (strcmp(token, "path") == 0) {
                //printf("yaaaaaaaaaaaaaaaaaaaa\n");
                // Tokenize the 'path' command arguments
                char *arguments = strtok(NULL, "\n");
                if (arguments != NULL) {
                    // Handle the custom 'path' command
                    strcpy(current, arguments);
                } else {
                    //dont change anything, it means no arguments after "path" and so current reset to empty
                    strcpy(current, "");
                    //printf("Error: 'path' command requires arguments.\n");
                }
            }

            else if (strcmp(token, "echo") == 0) {
                token = strtok(NULL, "");  // Get the rest of the line after "echo"
                if (token != NULL) {
                    printf("%s\n", token);  // Print the message after "echo"
                } else {
                    printErrorMessage();;
                }
            }

            else if (strcmp(token, "cd") == 0) {
                token = strtok(NULL, " \t");  // Get the next token after "cd"
                if (token != NULL) {
                    cdCommand(token);  // Change to the specified directory
                } else {
                    printErrorMessage();;
                }
            }


            else if (strcmp(token, "cat") == 0) {
                token = strtok(NULL, " \t");  // Get the next token after "cat"
                if (token != NULL) {
                    catFile(token);
                } else {
                    printErrorMessage();;
                }
            }

            else if (strcmp(token, "rm") == 0) {
                token = strtok(NULL, " \t");  // Get the next token after "rm"
                if (token != NULL) {
                    if (strcmp(token, "-f") == 0) {
                        token = strtok(NULL, " \t"); // Get the next token after "-f"
                        if (token != NULL) {
                            removeFile(token);
                        } else {
                            //perror("Invalid syntax");
                            printErrorMessage();;
                        }
                    } else {
                        removeFile(token);
                    }
                } else {
                    printErrorMessage();;
                }
            }

            else if (token != NULL) {
                if (strcmp(token, "ls") == 0) {
                    token = strtok(NULL, " \t");  // Get the next token after "ls"
                    if (token != NULL) {
                        // Look for '>' delimiter without spaces
                        char *redirectionToken = strstr(token, ">");
                        if (redirectionToken != NULL) {
                            *redirectionToken = '\0';  // Terminate the source path
                            char *destinationPath = redirectionToken + 1; // Get the part after '>'
                            if (*destinationPath != '\0') {
                                redirectLsOutput(token, destinationPath);
                            } else {
                                //perror("Invalid syntax");
                                printErrorMessage();;
                            }
                        } else {
                            // Check if the path is valid before listing
                            if (access(token, F_OK) != -1) {
                                // Check if it's a directory
                                struct stat fileStat;
                                if (stat(token, &fileStat) == 0 && S_ISDIR(fileStat.st_mode)) {
                                    lsCommand(token);
                                } else {
                                    printf("%s\n", token);  // Print the file name
                                }
                            } else {
                                fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", token);
                            }
                        }
                    } else {
                        lsCommand(".");
                    }
                } else {
                    printErrorMessage();;
                }
            }

            else {
                printErrorMessage();; 
            }
        }

        free(line); // Free allocated memory
        fclose(inputFile);
}

void interactiveMode() {
    while (1) {
        printf("witsshell> ");
        char command[100];
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';  // Remove newline character

        // Tokenize to handle many spaces before second arg, e.g ls     ..
        char *token = strtok(command, " \t");  // Tokenize using spaces and tabs as delimeters

        char full_command[2048];// Adjust the size as needed
        if (strlen(current) > 0) {
            strcpy(full_command, current);
            strcat(full_command, "/");
            strcat(full_command, command);
        }
            // Check F_OK and X_OK for the full command path
        if (access(full_command, F_OK) == 0 && access(full_command, X_OK) != 0) {
                /* printf("pass from INT to BATCH\n"); */
                // Run batchMode with the full command
                batchMode(full_command);
        } 
         else if (access(command, F_OK) == 0 && access(command, X_OK) != 0) { 
            /* printf("pass from INT to BATCH\n"); */
            // If current is empty, just run batchMode on command
            batchMode(command);
        }


        else if (strcmp(command, "exit") == 0) {
            token = strtok(NULL, "");  // Get the rest of the line after "exit"
            if (token == NULL || strlen(token) == 0) {
                break;
            } else {
                printErrorMessage();;
            }
        } 

        /// Skip processing if the trimmed line is empty or contains only '&'
        else if (strlen(command) == 0 || strcmp(command, "&") == 0) {
            continue;
        }

        // TO DO: Path
        // Check if the user entered the custom 'path' command
        else if (strcmp(token, "path") == 0) {
            //printf("yaaaaaaaaaaaaaaaaaaaa\n");
            // Tokenize the 'path' command arguments
            char *arguments = strtok(NULL, "\n");
            if (arguments != NULL) {
                // Handle the custom 'path' command
                 strcpy(current, arguments);
            } else {
                //dont change anything, it means no arguments after "path" and so current reset to empty
                strcpy(current, "");
                //printf("Error: 'path' command requires arguments.\n");
            }
        }

        else if (strcmp(token, "echo") == 0) {
            token = strtok(NULL, "");  // Get the rest of the line after "echo"
            if (token != NULL) {
                printf("%s\n", token);  // Print the message after "echo"
            } else {
                printErrorMessage();;
            }
        }

        else if (strcmp(token, "cd") == 0) {
            token = strtok(NULL, " \t");  // Get the next token after "cd"
            if (token != NULL) {
                cdCommand(token);  // Change to the specified directory
            } else {
                printErrorMessage();;
            }
        }


        else if (strcmp(token, "cat") == 0) {
            token = strtok(NULL, " \t");  // Get the next token after "cat"
            if (token != NULL) {
                catFile(token);
            } else {
                printErrorMessage();;
            }
        }

        else if (strcmp(token, "rm") == 0) {
            token = strtok(NULL, " \t");  // Get the next token after "rm"
            if (token != NULL) {
                if (strcmp(token, "-f") == 0) {
                    token = strtok(NULL, " \t"); // Get the next token after "-f"
                    if (token != NULL) {
                        removeFile(token);
                    } else {
                        //perror("Invalid syntax");
                        printErrorMessage();;
                    }
                } else {
                    removeFile(token);
                }
            } else {
                printErrorMessage();;
            }
        }

        else if (token != NULL) {
            if (strcmp(token, "ls") == 0) {
                token = strtok(NULL, " \t");  // Get the next token after "ls"
                if (token != NULL) {
                    // Look for '>' delimiter without spaces
                    char *redirectionToken = strstr(token, ">");
                    if (redirectionToken != NULL) {
                        *redirectionToken = '\0';  // Terminate the source path
                        char *destinationPath = redirectionToken + 1; // Get the part after '>'
                        if (*destinationPath != '\0') {
                            redirectLsOutput(token, destinationPath);
                        } else {
                            //perror("Invalid syntax");
                            printErrorMessage();;
                        }
                    } else {
                        // Check if the path is valid before listing
                        if (access(token, F_OK) != -1) {
                            // Check if it's a directory
                            struct stat fileStat;
                            if (stat(token, &fileStat) == 0 && S_ISDIR(fileStat.st_mode)) {
                                    lsCommand(token);
                            } else {
                                printf("%s\n", token);  // Print the file name
                            }
                        } else {
                            fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", token);
                        }
                    }
                } else {
                    lsCommand(".");
                }
            } else {
                //printf("yaaaaaaaaaaaaaaaaaaaa\n");
                printErrorMessage();;
            }
        }
        else {
            printErrorMessage();; 
        }
    }
}


// ...

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 2) {
        printErrorMessage();;
        return 1;
    }

    if (argc == 1) {    //INTERACTIVE MODE---------------------------------------     
        interactiveMode();
    } 
    
    else if (argc == 2) {    //BATCH MODE--------------------------------------------
        batchMode(argv[1]);
    }

    return 0;
}








// Lab 8: jsh.c
// The goal of this lab is to write a fake shell. 
//      See http://web.eecs.utk.edu/~jplank/plank/classes/cs360/360/labs/Lab-8-Jsh/index.html
//      for more information and lab specifications. 

// COSC 360
// Swasti Mishra 
// Mar 24, 2022
// James Plank 

// Libraries
# include <fcntl.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/wait.h>

# include "fields.h"
# include "jrb.h"

int main( int argc, char** argv ){
    IS input ; 
    input = new_inputstruct( NULL ) ;
    char* prompt, *inputFile, *outputFileTruncate, *outputFileAppend ;
    int status, pid, hasInput = 0, hasOutputTruncate = 0, hasOutputAppend = 0, parentWait = 1 ;
    
    // Checks number of arguments and responds accordingly 
    if( argc == 1 ){
        prompt = strdup( "jsh: " ) ;
    }
    else if( argc == 2 ){
        if( strcmp( argv[1], "-" ) == 0 ){
            prompt = strdup( "" ) ;
        }
        else{
            prompt = strdup( argv[1] ) ;
        }
    }
    else{
        fprintf( stderr, "usage: jsh [prompt]\n" ) ;
        exit(1) ;
    }

    // Go through arguments 
    while( get_line(input) >= 0 ){
        int pipeCount = 0 ;
        JRB pidJrb, temp ;
        pidJrb = make_jrb() ;

        printf( "%s", prompt ) ;
        if( strcmp( input -> fields[0], "exit" ) == 0 ){ 
            break ;
        }

        // Add up pipes
        for( int i = 0 ; i < input->NF ; i++ ){
            if( strcmp( input->fields[i], "|" ) == 0 ){
                pipeCount++ ;
            }
        }

        // Goes through the commands 
        char*** newargv = (char***) malloc( sizeof(char**) * ( pipeCount + 1 ) ) ;
        
        for( int pipe = 0, i = 0; pipe < pipeCount + 1; pipe++ ){
            int cmdInd = 0 ;
            char** command = (char**) malloc( sizeof(char*) * (input->NF + 1 ) ) ;
            while( (i < input->NF ) && ( strcmp( input -> fields[i], "|" ) != 0 ) ){
                if( strcmp( input->fields[i], "&" ) == 0 ){
                    command[cmdInd] = NULL ;
                    parentWait = 0 ;
                }
                else if( strcmp( input->fields[i], ">" ) == 0 ){
                    hasOutputTruncate = 1 ;
                    outputFileTruncate = strdup( input->fields[ i + 1 ] ) ;
                    command[cmdInd] = NULL ;
                    cmdInd++ ; 
                    i++ ;
                }
                else if( strcmp( input->fields[i], ">>" ) == 0 ){
                    hasOutputAppend = 1 ;
                    outputFileAppend = strdup( input->fields[ i + 1 ] ) ;
                    command[cmdInd] = NULL ;
                    cmdInd++ ; 
                    i++ ;
                }
                else if( strcmp( input->fields[i], "<" ) == 0 ){
                    hasInput = 1 ;
                    inputFile = strdup( input->fields[ i + 1 ] ) ;
                    command[cmdInd] = NULL ;
                    cmdInd++ ; 
                    i++ ;
                }
                else{   
                    command[cmdInd] = strdup( input->fields[i] ) ;
                }
                i++ ; 
                cmdInd++ ;
            }
            command[cmdInd] = NULL ;
            newargv[pipe] = command ;
            i++ ; 
            cmdInd++ ;
        }

        // Pipe fork 
        int prev_fd = -1 ;
        if( pipeCount != 0 ){
            for ( int i = 0 ; i < pipeCount + 1 ; i++ ){
                int pipefd[2] ;
                if ( pipe(pipefd) < 0 ){
                    perror( "jsh: pipe" ) ;
                    exit(1) ;
                }
                fflush(stdin)  ;
                fflush(stdout) ; 
                fflush(stderr) ;
                pid = fork() ; 
                
                if( pid == 0 ){
                    
                    // If it uses "<" redirection, open the file as stdin
                    if( hasInput && i == 0 ){
                        int fd1 = open( inputFile, O_RDONLY ) ;
                        if( fd1 < 0 ){
                            perror( inputFile ) ;
                            exit(1) ;
                        }
                        if( dup2( fd1, 0 ) != 0 ){
                            perror( inputFile ) ;
                            exit(1) ;
                        }
                        close(fd1) ;
                        free(inputFile) ;
                    }

                    // If it uses ">" redirection, open the file as stdout
                    if( i == pipeCount ){
                        if( hasOutputTruncate ){
                            int fd2 = open( outputFileTruncate, O_WRONLY | O_TRUNC | O_CREAT, 0644 ) ;
                            if( fd2 < 0 ){
                                perror( outputFileTruncate ) ;
                                exit(1) ;
                            }
                            if ( dup2(fd2, 1) != 1 ) {
                                perror( outputFileTruncate ) ;
                                exit(1);
                            }
                            close(fd2);
                            free (outputFileTruncate);
                        }

                        // If it uses ">>" redirection, open as stdout without truncation
                        if( hasOutputAppend ){
                            int fd3 = open( outputFileAppend, O_WRONLY | O_APPEND | O_CREAT, 0644 ) ;
                            if( fd3 < 0 ){
                                perror( outputFileAppend ) ;
                                exit(1) ;
                            }
                            if( dup2( fd3, 1 ) != 1 ){
                                perror( outputFileAppend ) ;
                                exit(1) ;
                            }
                            close(fd3) ;
                            free( outputFileAppend ) ;
                        }
                    }

                    // More piping stuff 
                    if( i == 0 ){
                        dup2( pipefd[1], 1 ) ;
                    }
                    if( ( i <  pipeCount ) && ( 0 < i ) ){
                        dup2( prev_fd, 0 ) ;
                        dup2( pipefd[1], 1 ) ;
                    }
                    if( i == pipeCount ){
                        dup2( prev_fd, 0 ) ;
                    }

                    close( prev_fd ) ;
                    close( pipefd[0] ) ;
                    close( pipefd[1] ) ;

                    execvp( newargv[i][0], newargv[i] ) ;
                    perror( newargv[i][0] ) ;
                    exit(1) ;       
                }
                
                // Tree keeps track of PIDs
                else{
                    jrb_insert_int( pidJrb, pid, new_jval_i(pid) ) ;
                    close( prev_fd ) ;
                    prev_fd = pipefd[0] ;
                    close( pipefd[1] ) ;

                    if( i == pipeCount ){
                        close( pipefd[0] ) ;
                    }
                }
            }

            // Waits until PIDs are removed
            if( parentWait ){
                while( !jrb_empty(pidJrb) ){
                    pid = wait( &status ) ;
                    temp = jrb_find_int( pidJrb, pid ) ;
                    if( temp != NULL ){
                        jrb_delete_node(temp) ;
                    }
                }
            }
        }

        // Forks the child 
        else{
            fflush(stdin)  ;
            fflush(stdout) ; 
            fflush(stderr) ;
            pid = fork() ;
            if( pid == 0 ){

                // Opens the input file for "<"
                if( hasInput ){
                    int fd1 = open( inputFile, O_RDONLY ) ;
                    if( fd1 < 0 ){
                        perror(inputFile) ;
                        exit(1) ;
                    }
                    if( dup2(fd1, 0) != 0 ){
                        perror(inputFile) ;
                        exit(1) ;
                    }
                    close(fd1) ;
                    free(inputFile) ;
                }

                // Opens the output file for ">"
                if( hasOutputTruncate ){
                    int fd2 = open(outputFileTruncate, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                    if ( fd2 < 0 ) {
                        perror(outputFileTruncate);
                        exit(1);
                    }

                    if ( dup2(fd2, 1) != 1 ) {
                        perror(outputFileTruncate);
                        exit(1);
                    }
                    close(fd2);
                    free (outputFileTruncate);
                }

                // Opens the output file for ">>"
                if( hasOutputAppend ){
                    int fd3 = open( outputFileAppend, O_WRONLY | O_APPEND | O_CREAT, 0644 ) ;
                    if( fd3 < 0 ){
                        perror( outputFileAppend ) ;
                        exit(1) ;
                    }
                    if( dup2(fd3, 1) != 1 ){
                        perror(outputFileAppend) ;
                        exit(1) ;
                    }
                    close(fd3) ;
                    free(outputFileAppend) ;
                }

                execvp( newargv[0][0], newargv[0] ) ;
                perror( newargv[0][0] ) ;

                exit(0) ;
            }
            else if( strcmp( input->fields[input->NF - 1 ], "&") != 0 ){
                while ( wait( &status ) != pid ) ;
            }
                
        }

        // Reset the variable flags 
        hasInput = 0 ;
        hasOutputTruncate = 0 ; 
        hasOutputAppend = 0 ;
        parentWait = 1 ;
    }

    while ( wait(&status) != -1 ) ;

    return 0 ;
}

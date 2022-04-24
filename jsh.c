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
    IS is ; 
    is = new_inputstruct( NULL ) ;
    char* prompt, *input, *outputFT, *outputFA ;
    int status, pid ;
    int parentWait = 1, inputFlag = 0, OTFlag = 0, OAFlag = 0 ;
    
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
    while( get_line(is) >= 0 ){
        int pipeCounter = 0 ;
        JRB pidJRB, tmp ;
        pidJRB = make_jrb() ;

        // Break if we are given the command to exit 
        printf( "%s", prompt ) ;
        if( strcmp( is->fields[0], "exit" ) == 0 ){ 
            break ;
        }

        // Add up pipes
        for( int i = 0 ; i < ( is->NF ) ; i++ ){
            if( strcmp( is->fields[i], "|" ) == 0 ){
                pipeCounter++ ;
            }
        }

        // Goes through the commands 
        char*** newArgv = (char***) malloc( sizeof(char**) * ( pipeCounter + 1 ) ) ;
        for( int pipe = 0, i = 0; pipe < ( pipeCounter + 1 ); pipe++ ){
            int cmd = 0 ;
            char** command = (char**) malloc( sizeof(char*) * ( ( is->NF ) + 1 ) ) ;
            
            // Check each field in the line 
            while( (i < is->NF ) && ( strcmp( is->fields[i], "|" ) != 0 ) ){
                
                // Tells us if we should wait for another task to complete
                if( strcmp( is->fields[i], "&" ) == 0 ){
                    command[cmd] = NULL ;
                    parentWait = 0 ;
                }
                
                // Opens file as stdout and truncates 
                else if( strcmp( is->fields[i], ">" ) == 0 ){
                    OTFlag = 1 ;
                    outputFT = strdup( is->fields[ i + 1 ] ) ;
                    command[cmd] = NULL ;
                    cmd++ ; 
                    i++ ;
                }

                // Opens file as stdout and appends
                else if( strcmp( is->fields[i], ">>" ) == 0 ){
                    OAFlag = 1 ;
                    outputFA = strdup( is->fields[ i + 1 ] ) ;
                    command[cmd] = NULL ;
                    cmd++ ; 
                    i++ ;
                }

                // Opens file as stdin 
                else if( strcmp( is->fields[i], "<" ) == 0 ){
                    inputFlag = 1 ;
                    input = strdup( is->fields[ i + 1 ] ) ;
                    command[cmd] = NULL ;
                    cmd++ ; 
                    i++ ;
                }
                
                // Anything else 
                else{
                    command[cmd] = strdup( is->fields[i] ) ;
                }
                i++ ; 
                cmd++ ;
            }
            command[cmd] = NULL ;
            newArgv[pipe] = command ;
            i++ ; 
            cmd++ ;
        }

        // If there's a pipe.... 
        int pipeFD = -1 ;
        if( pipeCounter != 0 ){
            for ( int i = 0 ; i < ( pipeCounter + 1 ) ; i++ ){
                int pipefd[2] ;
                
                // Connect processes
                if ( pipe(pipefd) < 0 ){
                    perror( "jsh: pipe" ) ;
                    exit(1) ;
                }

                // Flush and fork
                fflush(stdin)  ;
                fflush(stdout) ; 
                fflush(stderr) ;
                pid = fork() ; 
                

                if( pid == 0 ){
                    
                    // If it uses "<" redirection, open the file as stdin
                    if( ( inputFlag == 1 ) && ( i == 0 ) ){
                        int fd1 = open( input, O_RDONLY ) ;
                        if( fd1 < 0 ){
                            perror( input ) ;
                            exit(1) ;
                        }
                        if( dup2( fd1, 0 ) != 0 ){
                            perror( input ) ;
                            exit(1) ;
                        }
                        close(fd1) ;
                        free(input) ;
                    }

                    // If it uses ">" redirection, open the file as stdout
                    if( i == pipeCounter ){
                        if( OTFlag ){
                            int fd2 = open( outputFT, O_WRONLY | O_TRUNC | O_CREAT, 0644 ) ;
                            if( fd2 < 0 ){
                                perror( outputFT ) ;
                                exit(1) ;
                            }
                            if ( dup2(fd2, 1) != 1 ) {
                                perror( outputFT ) ;
                                exit(1);
                            }
                            close(fd2);
                            free (outputFT);
                        }

                        // If it uses ">>" redirection, open as stdout without truncation
                        if( OAFlag ){
                            int fd3 = open( outputFA, O_WRONLY | O_APPEND | O_CREAT, 0644 ) ;
                            if( fd3 < 0 ){
                                perror( outputFA ) ;
                                exit(1) ;
                            }
                            if( dup2( fd3, 1 ) != 1 ){
                                perror( outputFA ) ;
                                exit(1) ;
                            }
                            close(fd3) ;
                            free( outputFA ) ;
                        }
                    }

                    // More piping stuff 
                    if( i == 0 ){
                        dup2( pipefd[1], 1 ) ;
                    }
                    if( ( i <  pipeCounter ) && ( 0 < i ) ){
                        dup2( pipeFD, 0 ) ;
                        dup2( pipefd[1], 1 ) ;
                    }
                    if( i == pipeCounter ){
                        dup2( pipeFD, 0 ) ;
                    }

                    // Close all the pipes 
                    close( pipeFD ) ;
                    close( pipefd[0] ) ;
                    close( pipefd[1] ) ;

                    // Call exec on the commands 
                    execvp( newArgv[i][0], newArgv[i] ) ;
                    perror( newArgv[i][0] ) ;
                    exit(1) ;       
                }
                
                // Tree keeps track of pids
                else{
                    jrb_insert_int( pidJRB, pid, new_jval_i(pid) ) ;
                    close( pipeFD ) ;
                    pipeFD = pipefd[0] ;
                    close( pipefd[1] ) ;

                    if( i == pipeCounter ){
                        close( pipefd[0] ) ;
                    }
                }
            }

            // Cleans up zombie processes
            if( parentWait ){
                while( !jrb_empty(pidJRB) ){
                    pid = wait( &status ) ;
                    tmp = jrb_find_int( pidJRB, pid ) ;
                    if( tmp != NULL ){
                        jrb_delete_node(tmp) ;
                    }
                }
            }
        }

        // Flush and forks the child 
        else{
            fflush(stdin)  ;
            fflush(stdout) ; 
            fflush(stderr) ;
            pid = fork() ;
            if( pid == 0 ){

                // Opens the input file for "<"
                if( inputFlag ){
                    int fd1 = open( input, O_RDONLY ) ;
                    if( fd1 < 0 ){
                        perror(input) ;
                        exit(1) ;
                    }
                    if( dup2(fd1, 0) != 0 ){
                        perror(input) ;
                        exit(1) ;
                    }
                    close(fd1) ;
                    free(input) ;
                }

                // Opens the output file for ">"
                if( OTFlag ){
                    int fd2 = open(outputFT, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                    if ( fd2 < 0 ) {
                        perror(outputFT);
                        exit(1);
                    }

                    if ( dup2(fd2, 1) != 1 ) {
                        perror(outputFT);
                        exit(1);
                    }
                    close(fd2);
                    free (outputFT);
                }

                // Opens the output file for ">>"
                if( OAFlag ){
                    int fd3 = open( outputFA, O_WRONLY | O_APPEND | O_CREAT, 0644 ) ;
                    if( fd3 < 0 ){
                        perror( outputFA ) ;
                        exit(1) ;
                    }
                    if( dup2(fd3, 1) != 1 ){
                        perror(outputFA) ;
                        exit(1) ;
                    }
                    close(fd3) ;
                    free(outputFA) ;
                }

                // Execute again
                execvp( newArgv[0][0], newArgv[0] ) ;
                perror( newArgv[0][0] ) ;
                exit(0) ;
            }
            
            // In case of no ampersand
            else if( strcmp( is->fields[( is->NF ) - 1 ], "&") != 0 ){
                while ( wait( &status ) != pid ) ;
            }
                
        }

        // Reset the variable flags 
        inputFlag = 0 ;
        OTFlag = 0 ; 
        OAFlag = 0 ;
        parentWait = 1 ;
    }
    
    // Wait for the child process to finish 
    while ( wait(&status) != -1 ) ;

    return 0 ;
}


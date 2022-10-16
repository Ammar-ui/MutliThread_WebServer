#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "web-util.h"


int main(int argc, char **argv)
{
    int i, port, listenfd, socketfd, hit;
    socklen_t length;
    static struct sockaddr_in cli_addr;  // static = initialised to zeros
    static struct sockaddr_in serv_addr; // static = initialised to zeros


    /* =======================================================================
     * Check for command-line errors, and print a help message if necessary.
     * =======================================================================
     */
    if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
     (void)printf("hint: web-server Port-Number Top-Directory\t\tversion %d\n\n"
    "\tweb-server is a small and very safe mini web server\n"
    "\tweb-server only servers out file/web pages with extensions named below\n"
    "\tand only from the named directory or its sub-directories.\n"
    "\tThere is no fancy features = safe and secure.\n\n"
    "\tExample: web-server 8181 /home/webdir\n\n"
    "\tOnly Supports:", VERSION);

        for(i=0;extensions[i].ext != 0;i++)
            (void)printf(" %s",extensions[i].ext);

     (void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
    "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n");
        exit(0);
    }


    /* =======================================================================
     * Check the legality of the specified top directory of the web site.
     * =======================================================================
     */
    if( !strncmp(argv[2],"/"   ,2 ) || !strncmp(argv[2],"/etc", 5 ) ||
        !strncmp(argv[2],"/bin",5 ) || !strncmp(argv[2],"/lib", 5 ) ||
        !strncmp(argv[2],"/tmp",5 ) || !strncmp(argv[2],"/usr", 5 ) ||
        !strncmp(argv[2],"/dev",5 ) || !strncmp(argv[2],"/sbin",6) ){
        (void)printf("ERROR: Bad top directory %s, see web-server -?\n",argv[2]);
        exit(3);
    }


    /* =======================================================================
     * See if we can change the current directory to the one specified.
     * =======================================================================
     */
    if(chdir(argv[2]) == -1){ 
        (void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
        exit(4);
    }


    /* =======================================================================
     * Detach the process from the controlling terminal, after all input args
     * checked out successfuly; then create a child process to continue as a
     * daemon, whose job is to create a communication socket to listen for
     * http requests.
     * =======================================================================
     *
     * Become deamon + unstopable and no zombies children (= no wait())
     * =======================================================================
     */
    if(fork() != 0)  return 0;         // parent terminates, returns OK to shell
    (void)signal(SIGCLD, SIG_IGN);     // ignore child death
    (void)signal(SIGHUP, SIG_IGN);     // ignore terminal hangups

    for(i=0;i<32;i++) (void)close(i);  // close open files
    (void)setpgrp();                   // break away from process group

    logs(MSG6,atoi(argv[1]),"web-server starting at port","pid",getpid());


    /* =======================================================================
     * Setup the network socket
     * =======================================================================
     */
    if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0) {
        logs(ERROR,1,"system call","socket",0);
        exit(3);
    }

    port = atoi(argv[1]);
    if(port < 0 || port >60000) {
        logs(ERROR,2,"Invalid port number (try 1->60000)",argv[1],0);
        exit(3);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
        logs(ERROR,3,"system call","bind",0);
        exit(3);
    }

    if(listen(listenfd,64) <0) {
        logs(ERROR,4,"system call","listen",0);
        exit(3);
    }


    /* =======================================================================
     * Loop forever, listening to connection attempts and accept them.
     * For each accepted connection, serve it and return. 
     * =======================================================================
     */
    for(hit=1; ;hit++) {
        length = sizeof(cli_addr);
        if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
            logs(ERROR,5,"system call","accept",0);
        else {
            serve(socketfd,hit);           // serve and return
            (void)close(socketfd);         // close read-write descriptor
        }
    }
}

#include "np_simple/utils.hpp"
#include "np_simple/np_simple_shell.hpp"
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

int main(int argc, char const *argv[]){
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return 0;
	}

	int server_port = atoi(argv[1]);
	int master_socket, slave_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	pid_t pid;
	int opt = 1;
	
	if(signal(SIGCHLD,SIGCHLD_handler) == SIG_ERR) { perror("signal error"); }

	master_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (master_socket < 0) { perror("socket error"); }
	
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt error");
    }
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(server_port);

	if ( bind(master_socket, (struct sockaddr *)&address, sizeof(address) ) < 0 ){ perror("bind error"); }
	if ( listen(master_socket, 1) < 0 ) { perror("listen error"); }

	while(true){
		slave_socket = accept(master_socket, (struct sockaddr *)&address,(socklen_t*)&addrlen);
		if ( slave_socket < 0 ) { perror("accept error");}
		while((pid = fork()) < 0) { usleep(1000);}

		if(pid == 0){ // child
			close(master_socket);
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			dup2(slave_socket, STDIN_FILENO);
			dup2(slave_socket, STDOUT_FILENO);
			dup2(slave_socket, STDERR_FILENO);
			close(slave_socket);
			simple_shell();
			_exit(EXIT_SUCCESS);

		}else{ // parent
			close(slave_socket);
		}
		
	}

	return 0;
}

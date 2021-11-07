#include "np_multi_proc/utils.hpp"
#include "np_multi_proc/np_multi_proc_shell.hpp"
#include "np_multi_proc/semaphore.hpp"
#include "np_multi_proc/sharememory.hpp"
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>

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
	int serverlogfd;
    char defaultname[] = "(no name)";
	
	//  signal handler //
	if(signal(SIGCHLD, SIGCHLD_handler) == SIG_ERR) { perror("signal error"); }
	if(signal(SIGINT, SIGINT_handler) == SIG_ERR) { perror("signal error"); }
	if(signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) { perror("signal error"); }

	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction =  SIGUSR2_handler;
	act.sa_flags = SA_SIGINFO | SA_RESTART;
	if(sigaction(SIGUSR2, &act, NULL) < 0) { perror("signal error"); }

	// create share memory and semaphore //
    shmid = shmget(IPC_PRIVATE, sizeof(ShareMemory), IPC_CREAT | 0600);
    if(shmid < 0) { perror("get shm error"); }
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if(semid < 0) { perror("semget error"); }

	// attach local memory to shared memory and initialize // 
    shmaddr = (ShareMemory *)shmat(shmid, NULL, 0);
    for(int i=0;i<MAX_USERS;i++){
        shmaddr->users[i].userid = i;
        shmaddr->users[i].conn = false;
    }
	for(int i=0;i<MAX_USERS;i++){
		for(int j=0;j<MAX_USERS;j++){
			shmaddr->userPipeManager[i][j].exist = false;
		}
	}

	// create socket, bind, listen
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
			serverlogfd = dup(STDOUT_FILENO);
			dup2(slave_socket, STDIN_FILENO);
			dup2(slave_socket, STDOUT_FILENO);
			dup2(slave_socket, STDERR_FILENO);
			close(slave_socket);
            int minid = findminUserId(shmaddr);
			userSetInfo(
				shmaddr,
				getpid(),
				minid,
				defaultname,
				inet_ntoa(address.sin_addr),
				ntohs(address.sin_port),
				true,
				0,
				0);
			fprintf(stdout, "%s", welcomemsg().c_str());
			broadcastmsg(shmaddr, loginmsg(shmaddr, minid));
			shell(shmaddr, minid, serverlogfd);
			broadcastmsg(shmaddr, logoutmsg(shmaddr, minid));
			lock(semid);
			for(int i=0;i<MAX_USERS;i++){
				for(int j=0;j<MAX_USERS;j++){
					if(shmaddr->userPipeManager[i][j].exist){
						shmaddr->userPipeManager[i][j].exist = false;
						close(shmaddr->userPipeManager[i][j].recvfd);
					}
				}
			}
			unlock(semid);
			shmdt(shmaddr);
			close(serverlogfd);
			_exit(EXIT_SUCCESS);

		}else{ // parent
			close(slave_socket);
		}
	}

	return 0;
}

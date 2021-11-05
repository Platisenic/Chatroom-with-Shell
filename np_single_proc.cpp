#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <algorithm>
#include <string>
#include <vector>
#include "np_single_proc/UserInfo.hpp"
#include "np_single_proc/utils.hpp"
#include "np_single_proc/np_single_proc_shell.hpp"


int main(int argc, char const *argv[]){
	if (argc != 2) { fprintf(stderr, "Usage: %s [port]\n", argv[0]); return 0;}

	if(signal(SIGCHLD,SIGCHLD_handler) == SIG_ERR){
        fprintf(stderr, "Cannot handle SIGHLD");
    }

	int slave_socket;
	int opt = 1;

	int master_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (master_socket < 0) { perror("socket error"); }
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt error");
    }

	struct sockaddr_in address;
	int server_port = atoi(argv[1]);
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(server_port);
	int addrlen = sizeof(address);
	if ( bind(master_socket, (struct sockaddr *)&address, sizeof(address) ) < 0 ){ perror("bind error"); }

	if ( listen(master_socket, MAX_USERS) < 0 ) { perror("listen error"); }

	fd_set	rfds, afds;
	int nfds = master_socket;
    FD_ZERO(&afds);
    FD_SET(master_socket, &afds);

	std::vector<UserInfo> users;
	for(int i=0; i<MAX_USERS; i++){
		users.push_back(UserInfo(i, false));
	}

	int storestdfd[3] = {0};

	while(true){
        memcpy(&rfds, &afds, sizeof(rfds));
        while (select(nfds + 1, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0) {}
		if(FD_ISSET(master_socket, &rfds)){
			slave_socket = accept(master_socket, (struct sockaddr *)&address,(socklen_t*)&addrlen);
			if ( slave_socket < 0 ) { perror("accept error");}
			FD_SET(slave_socket, &afds);
			nfds = std::max(nfds, slave_socket);
			int minid = findminUserId(users);
			users[minid].setinfo(
				slave_socket,
				"(no name)",
				inet_ntoa(address.sin_addr),
				ntohs(address.sin_port),
				true);
			sendmessages(slave_socket, welcomemsg());
			broadcastmsg(users, loginmsg(users, minid));
			sendmessages(slave_socket, "% ");
		}
		for(int sock=0; sock<=nfds; ++sock){
			if(sock != master_socket && FD_ISSET(sock, &rfds)){
				int userid = findUserIdBySock(users, sock);
				storestdfd[0] = dup(STDIN_FILENO);
				storestdfd[1] = dup(STDOUT_FILENO);
				storestdfd[2] = dup(STDERR_FILENO);
				dup2(sock, STDIN_FILENO);
				dup2(sock, STDOUT_FILENO);
				dup2(sock, STDERR_FILENO);
				shell(users, userid, storestdfd[1]);
				fflush(stdout);
				fflush(stderr);
				dup2(storestdfd[0], STDIN_FILENO);
				dup2(storestdfd[1], STDOUT_FILENO);
				dup2(storestdfd[2], STDERR_FILENO);
				close(storestdfd[0]);
				close(storestdfd[1]);
				close(storestdfd[2]);
				if(users[userid].conn){ // still connected
					sendmessages(sock, "% ");
				}else{ // leave
					broadcastmsg(users, logoutmsg(users, userid));
					for(int id=1;id<MAX_USERS;id++){
						if(users[id].conn){
							for(auto it=users[id].userPipeManager.begin(); it!=users[id].userPipeManager.end();){
								if(it->send_userid == userid){
									close(it->m_pipe_read);
									users[id].userPipeManager.erase(it);
								}else{
									it++;
								}
							}
						}
					}
					for(auto p: users[userid].userPipeManager){
						close(p.m_pipe_read);
					}
					for(auto it=users[userid].pipeManager.begin(); it!=users[userid].pipeManager.end();it++){
						close(it->second.m_pipe_read);
						close(it->second.m_pipe_write);
					}
					close(sock);
					FD_CLR(sock, &afds);
				}
			}
		}
		
	}

	return 0;
}

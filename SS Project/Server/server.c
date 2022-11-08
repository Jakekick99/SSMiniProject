#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8080
struct savings_account{
	char no[10];
	char name[20];
	int balance;
	char pin[5];
};
struct joint_account{
	char no[10];
	char name1[20];
	char name2[20];
	int balance;
	char pin[5];
};

int main(){
	
	// AF_LOCAL is the POSIX local process to process communication
	// SOCK_STREAM specifies that it is TCP
	// 0 means we are going to use Internet Protocol
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	int read_len=-99;
	char buffer[1024]={0};
	//printf("The fd value is %d",socket_fd);
	if(socket_fd==-1){
		printf("Fatal error creating socket.... \nRetry again.\n\n\nQuiting.....");
		return 0;
	}
	// Now, we specify the pre requisites for the bind() function
	
	struct sockaddr_in address; // This struct is used to deal with all internet address
	
	address.sin_family = AF_INET;  			// Refers to addresses in the IPv4 family
    	address.sin_addr.s_addr = htonl(INADDR_ANY);	// Refers to the binding localhost
    	address.sin_port = htons(8080);			// We are setting the incoming port to listen to as 8080
	
	int addrlen = sizeof(address);
	
	// we are now binding the 
	if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        printf("Fatal error binding socket.... \nRetry again.\n\n\nQuiting.....");
        return(0);
    }
    
    if (listen(socket_fd, 3) < 0) {
        printf("Too many clients in queue while server is busy.\nRetry again.\nQuiting.....");
        return(0);
    }
    while(1){
    	
    	int new_client_socket = -99;
    	if ((new_client_socket = accept(socket_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen)) < 0) {
        	printf("Unable to accept connection.\nRetry again.\nQuiting.....");
        	close(socket_fd);
        }
        else{
        
        	if (!fork())
            {
                read_len = read(new_client_socket, buffer, 1024);
                printf("Buffer has %s\n",buffer);
                if(strcmp(buffer,"admin")==0){
                	printf("admin\n");
                	admin(new_client_socket, socket_fd);
                	
                }
                else if(strcmp(buffer,"sav")==0){
                	printf("savings\n");
                	client_s(new_client_socket,socket_fd);
                }
                else{
                	printf("joint\n");
                	client_j(new_client_socket,socket_fd);
                }
                write(new_client_socket, "END\0", strlen("END\0"));
                return(0);
            }
        
        }
    }  
    system("fuser -k 8080/tcp");
    return 0;
}

int admin(int client_socket, int server_socket){
	char buffer[1024];
	char buffer2[1024];
	int master_file_fd = open("./DB/master.txt", O_RDONLY);
	int read_len = read(master_file_fd, buffer2, 1024);
	int fd;
	struct flock lock;
	buffer2[read_len-1]='\0';
	strcpy(buffer,"Enter ADMIN Password :");
	write(client_socket, buffer, strlen(buffer));
	read_len = read(client_socket, buffer, 1024);
	buffer[read_len]='\0';
	if(strcmp(buffer,buffer2)!=0){
		strcpy(buffer,"Wrong password. Terminating. \n\nSend a char back to finish");
		write(client_socket, buffer, strlen(buffer));
		read_len = read(client_socket,buffer,1024);
		close(master_file_fd);
		return 0;
	}
	else{
		while(strcmp(buffer,"4\0")!=0){
			strcpy(buffer,"1. Add an account\n2. Delete an account\n3. Modify account\n4. Exit (4 or any other key)\n Your response:");
			write(client_socket, buffer, strlen(buffer));
			read_len = read(client_socket, buffer, 1024);
			buffer[read_len]='\0';
			if(strcmp(buffer,"1\0")==0){
				strcpy(buffer2,"1. Savings account\n2. Joint account\n3.Cancel (3 or any other key)\n Your response:");
				write(client_socket, buffer2, strlen(buffer2));
				read_len = read(client_socket, buffer2, 1024);
				buffer2[read_len]='\0';
				if(strcmp(buffer2,"1\0")==0){
					//Create a savings account
					struct savings_account s;
					strcpy(buffer2,"Enter account number:");
					send(client_socket, buffer2, strlen(buffer2), 0);
					read_len = read(client_socket, buffer2, 1024);
					buffer2[read_len]='\0';
					char path[100] = "DB/savings/";
					strcat(path,buffer2); 
					fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0777);
					if(fd<0){
						strcpy(buffer2,"Account number already exists\nPress any key to continue\0");
						write(client_socket, buffer2, strlen(buffer2));
						read_len = read(client_socket, buffer2, 10);
						buffer2[read_len]='\0';
					}
					else{
						lock.l_type=F_WRLCK;  
	   					lock.l_whence=SEEK_SET;
	   					lock.l_start=0;  
	   					lock.l_len=0;
	   					lock.l_pid=getpid();
	   					fcntl(fd,F_SETLKW,&lock);
						strcpy(s.no,buffer2);
						strcpy(buffer2,"Enter account name:\0");
						write(client_socket, buffer2, strlen(buffer2));
						read_len = read(client_socket, buffer2, 30);
						buffer2[read_len]='\0';
						strcpy(s.name,buffer2);
						strcpy(buffer2,"Enter account pin:\0");
						write(client_socket, buffer2, strlen(buffer2));
						read_len = read(client_socket, buffer2, 4);
						buffer2[read_len]='\0';
						strcpy(s.pin,buffer2);
						s.balance = 0;
						lseek(fd,0,SEEK_SET);
						write(fd,&s,sizeof(struct savings_account));
						printf("Ac no:%s, name:%s, pin:%s, bal:%d\n",s.no,s.name,s.pin,s.balance);
						strcpy(buffer2,"Account created successfully\nPress any key to continue\0");
						lock.l_type=F_UNLCK;
						fcntl(fd,F_SETLK,&lock);
						write(client_socket, buffer2, strlen(buffer2));
						read_len = read(client_socket, buffer2, 10);
					}
				}	
				else if(strcmp(buffer2,"2\0")==0){
					//create a joint account
					struct joint_account j;
					strcpy(buffer2,"Enter account number:\0");
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer2, 1024);
					char path[100] = "DB/joint/";
					strcat(path,buffer2); 
					fd = open(path, O_CREAT | O_EXCL, 0644);
					if(fd<0){
						strcpy(buffer2,"Account number already exists\nPress any key to continue\0");
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 10);
					}
					else{
						lock.l_type=F_WRLCK;  
	   					lock.l_whence=SEEK_SET;
	   					lock.l_start=0;  
	   					lock.l_len=0;
	   					lock.l_pid=getpid();
	   					fcntl(fd,F_SETLKW,&lock);
						strcpy(j.no, buffer2);
						strcpy(buffer2,"Enter account name 1:\0");
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 30);
						strcpy(j.name1,buffer2);
						strcpy(buffer2,"Enter account name 2:\0");
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 30);
						strcpy(j.name2,buffer2);
						strcpy(buffer2,"Enter account pin:\0");
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 4);
						strcpy(j.pin,buffer2);
						j.balance = 0;
						lseek(fd,0,SEEK_SET);
						write(fd,&j,sizeof(struct joint_account));
						strcpy(buffer2,"Account created successfully\nPress any key to continue\0");
						lock.l_type=F_UNLCK;
						fcntl(fd,F_SETLK,&lock);
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 10);
					}
				}
				else{
					//cancelling acc creation
					buffer2[0]='\0';
					buffer[0]='\0';
				}
			}
			else if(strcmp(buffer,"2\0")==0){
				//deleting account
				strcpy(buffer2,"Enter account number:\0");
				send(client_socket, buffer2, strlen(buffer), 0);
				read_len = read(client_socket, buffer2, 1024);
				char path[100] = "DB/";
				strcat(path,buffer2); 
				if(remove(path)==0){
					strcpy(buffer2,"Account deleted\nPress any key to continue\0");
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer2, 10);
				}
				else{
					strcpy(buffer2,"Account doesn't exist\nPress any key to continue\0");
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer2, 10);
				}
			}
			else if(strcmp(buffer,"3\0")==0){
				//modifying account
				struct savings_account s;
					strcpy(buffer2,"Enter account number:\0");
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer2, 1024);
					char path[100] = "DB/";
					strcat(path,buffer2); 
					fd = open(path, O_RDWR, 0644);
					if(fd<0){
						strcpy(buffer2,"Account doesn't exist\nPress any key to continue\0");
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 10);
					}
					else{
						lock.l_type=F_WRLCK;  
	   					lock.l_whence=SEEK_SET;
	   					lock.l_start=0;  
	   					lock.l_len=0;
	   					lock.l_pid=getpid();
	   					fcntl(fd,F_SETLKW,&lock);
						strcpy(s.no,buffer2);
						strcpy(buffer2,"Enter account name:\0");
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 30);
						strcpy(s.name,buffer2);
						strcpy(buffer2,"Enter account pin:\0");
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 4);
						strcpy(s.pin,buffer2);
						s.balance = 0;
						lseek(fd,0,SEEK_SET);
						write(fd,&s,sizeof(struct savings_account));
						strcpy(buffer2,"Account created successfully\nPress any key to continue\0");
						lock.l_type=F_UNLCK;
						fcntl(fd,F_SETLK,&lock);
						send(client_socket, buffer2, strlen(buffer), 0);
						read_len = read(client_socket, buffer2, 10);
					}
			}
			else{
				//wrong choice
				//strcpy(buffer,"Wrong choice, try again\n1. Savings account\n2. Joint account\n3.Cancel\0");
			}
		}
	}
}
int client_s(int client_socket, int server_socket){
	char buffer[1024];
	char buffer2[1024];
	int fd;
	int read_len;
	struct flock lock;
	struct savings_account s;
	char pin[5];
	strcpy(buffer,"Enter Savings Account number\0");
	write(client_socket, buffer, strlen(buffer));
	read_len = read(client_socket, buffer, 1024);
	buffer[read_len]='\0';
	char path[100] = "DB/savings/";
	strcat(path,buffer);
	printf("%s\n",path);
	fd = open(path, O_RDWR);
	if(fd<0){
		strcpy(buffer,"Account doesn't exist\nPress any key to continue\0");
		write(client_socket, buffer, strlen(buffer));
		read_len = read(client_socket, buffer, 10);
		buffer[read_len]='\0';
		return 0;
	}
	else{
	    read(fd,&s,sizeof(struct savings_account));
		printf("%d\n",fd);
		strcpy(pin,s.pin);
		printf("Correct PIN=%s\n",pin);
		strcpy(buffer,"Enter PIN\0");
		write(client_socket, buffer, strlen(buffer));
		read_len = read(client_socket, buffer, 10);
		buffer[read_len]='\0';
		printf("Pin from client:%s\n",buffer);
		buffer[4]='\0';
		int result = strcmp(pin,buffer);
		printf("Comparison:%d\n",result);
		if(result == 0){
			strcpy(buffer2,"account no=");
			strcat(buffer2,s.no);
			strcat(buffer2,"\naccount_name=\0");
			strcat(buffer2,s.name);
			char balance[100];
			sprintf(balance, "%d", s.balance);
			strcat(buffer2,"\nbalance=\0");
			strcat(buffer2,balance);
			strcat(buffer2,"\npin=\0");
			strcat(buffer2,s.pin);
			strcat(buffer2,"\n\n\0");
			strcat(buffer2,"1. withdraw\n2. deposit\n3. Modify PIN\n4. Exit\0");
			strcpy(buffer,"placeholder\0");
			printf("%s\n",buffer2);
			
			while(strcmp(buffer,"4\0")!=0){
				write(client_socket, buffer2, strlen(buffer2));
				read_len = read(client_socket, buffer, 1024);
				buffer[read_len]='\0';
				if(strcmp(buffer,"1\0")==0){
					strcpy(buffer,"How much to withdraw\n\0");
					send(client_socket, buffer, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
					int req;
					sscanf(buffer, "%d", &req);
					if(req>s.balance){
						strcpy(buffer,"Not enough balance\n\0");
					}
					else{
						strcpy(buffer,"Done\n\0");
						s.balance-=req;
						lseek(fd,0,SEEK_SET);
						write(fd,&s,sizeof(struct savings_account));
					}
					send(client_socket, buffer2, strlen(buffer), 0);
				}
				else if(strcmp(buffer,"2\0")==0){
					strcpy(buffer,"How much to deposit\n\0");
					send(client_socket, buffer, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
					buffer[read_len]='\0';
					int req;
					sscanf(buffer, "%d", &req);
					s.balance+=req;
					strcpy(buffer,"Done\n\0");
					lseek(fd,0,SEEK_SET);
					write(fd,&s,sizeof(struct savings_account));
					send(client_socket, buffer, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
				}
				else if(strcmp(buffer,"3\0")==0){
					strcpy(buffer,"What is the new PIN\n\0");
					send(client_socket, buffer, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 4);
					buffer[read_len]='\0';
					strcpy(s.pin,buffer);
					strcpy(buffer,"Done\n\0");
					lseek(fd,0,SEEK_SET);
					write(fd,&s,sizeof(struct savings_account));
					send(client_socket, buffer, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
				}
				else{
					strcpy(buffer2,"Quitting.\nSend any char to continue\0");
					send(client_socket, buffer2, strlen(buffer2), 0);
					return 0;
				}
			}
		}
		else{
			strcpy(buffer2,"Account PIN is wrong\nQuitting\n Send any char to continue\0");
			send(client_socket, buffer2, strlen(buffer2), 0);
			read_len = read(client_socket, buffer, 1024);
			return 0;
		}
	}
	
	//enter acc number
	
	//account not found retry
	
	//account found enter pin
	
	//pin wrong try again
	
	//pin correct
	
	//withdraw
	
	//deposit
	
	//change pin
	
	//view balance and details shown as default
}

int client_j(int client_socket, int server_socket){
	char buffer[1024]; //= malloc(1024);
	char buffer2[1024]; //= malloc(1024);
	int fd;
	int read_len;
	struct flock lock;
	strcpy(buffer,"Enter Joint Account number\0");	
	write(client_socket, buffer, strlen(buffer));
	read_len = read(client_socket, buffer, 10);
	buffer[read_len]='\0';
	char path[100] = "DB/";
	printf("%s\n",buffer);
	strcat(path,buffer);
	printf("%s\n",path);
	fd = open(path, O_RDWR, 0644);
	printf("%d\n",fd);
	if(fd<0){
		strcpy(buffer,"Account doesn't exist\nPress any key to continue\0");
		send(client_socket, buffer, strlen(buffer), 0);
		read_len = read(client_socket, buffer, 10);
	}
	else{
		char* pin;
		struct joint_account s;
		read(fd,&s,sizeof(struct joint_account));
		pin = s.pin;
		//buffer="Enter PIN\nPress any key to continue\0";
		send(client_socket, buffer, strlen(buffer), 0);
		read_len = read(client_socket, buffer, 10);
		if(strcmp(pin,buffer)==0){
			//buffer2="account no=";
			strcat(buffer2,s.no);
			strcat(buffer2,"\naccount_name_1=");
			strcat(buffer2,s.name1);
			strcat(buffer2,"\naccount_name_2=");
			strcat(buffer2,s.name2);
			strcat(buffer2,"\nbalance=");
			strcat(buffer2,s.balance);
			strcat(buffer2,"\npin=");
			strcat(buffer2,s.pin);
			strcat(buffer2,"\n\n");
			strcat(buffer2,"1. withdraw\n2. deposit\n3. Modify PIN\n4. Exit\0");
			//buffer="e";
			while(strcmp(buffer,"4\0")!=0){
				send(client_socket, buffer2, strlen(buffer), 0);
				read_len = read(client_socket, buffer, 1024);
				if(strcmp(buffer,"1\0")==0){
					//buffer2="How much to withdraw\n\0";
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
					int req;
					sscanf(buffer, "%d", &req);
					if(req>s.balance){
						//buffer2="Not enough balance\n\0";
					}
					else{
						//buffer2="Done\n\0";
						s.balance-=req;
						lseek(fd,0,SEEK_SET);
						write(fd,&s,sizeof(struct joint_account));
					}
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
				}
				else if(strcmp(buffer,"2\0")==0){
					//buffer2="How much to deposit\n\0";
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
					int req;
					sscanf(buffer, "%d", &req);
					s.balance+=req;
					//buffer2="Done\n\0";
					lseek(fd,0,SEEK_SET);
					write(fd,&s,sizeof(struct joint_account));
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
				}
				else if(strcmp(buffer,"3\0")==0){
					//buffer2="What is the new PIN\n\0";
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 4);
					strcpy(s.pin,buffer);
					//buffer2="Done\n\0";
					lseek(fd,0,SEEK_SET);
					write(fd,&s,sizeof(struct joint_account));
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
				}
				else if(strcmp(buffer,"4\0")==0){
					break;
				}
				else{
					//buffer2="Invlaid entry.\nPress any key to continue\0";
					send(client_socket, buffer2, strlen(buffer), 0);
					read_len = read(client_socket, buffer, 1024);
				}
			}
		}
		else{
			//buffer2="Account PIN is wrong\nPress any key to continue\0";
			send(client_socket, buffer2, strlen(buffer), 0);
			read_len = read(client_socket, buffer2, 10);
		}
	}
	//enter acc number
	
	//account not found retry
	
	//account found enter pin
	
	//pin wrong try again
	
	//pin correct
	
	//withdraw
	
	//deposit
	
	//change pin
}

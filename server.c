/*
** server.c
** -------------------------------------------
** Owner: Daniel Better
** -------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

//dir includes
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAX_BUF_SIZE 256

#define MAX_CLIENT_CONNECTION 10




//------------------------------------------------------------------------------------

void do_processing(int new_fd, int mode, int select_flag, struct sockaddr_storage their_addr, int connection_flag, char* msg_buffer, char* cwd, int* clientIndex, fd_set* master)
{
    //variables decleration

	int bytes_sent;
	FILE *fp;
	long int file_size;
	//dir variables
	DIR *mydir;
	char dirbuffer[MAX_BUF_SIZE];
	char buffer[MAX_BUF_SIZE];
	struct dirent *mydirentp;
	
	int dir_flag = 0;

	int bytes_rcvd, bytes_read;
	char rcvd_buffer[MAX_BUF_SIZE];
	char *temp_rcvd_ptr;

	socklen_t to_len;
	int from_len;

	to_len = sizeof(struct sockaddr_storage);

	if (mode == 0) //TCP connection
	{
	     	bytes_rcvd = recv(new_fd, rcvd_buffer, MAX_BUF_SIZE, 0);		
		if (bytes_rcvd == -1)
		{
			perror("recv");
			exit(1);
		}	

		rcvd_buffer[bytes_rcvd] = '\0';
    	        if (rcvd_buffer[strlen(rcvd_buffer)-1] == '\n')
		{
     			 rcvd_buffer[strlen(rcvd_buffer)-1] = '\0';
		}
        	if (rcvd_buffer[strlen(rcvd_buffer)-1] == '\r')
		{
     			rcvd_buffer[strlen(rcvd_buffer)-1] = '\0';
		}
	

	//TCP "ls" command
		if (strcmp("ls", rcvd_buffer) == 0)
		{
			if (chdir(cwd) != 0)
			{
				printf("I've been asked to change the dir to %s", cwd);
				perror(NULL);
			}
		   //open directory
			mydir = opendir(cwd);
	  		while (mydirentp = readdir(mydir))
			{	
				memcpy(dirbuffer, mydirentp->d_name, mydirentp->d_reclen);
				dirbuffer[mydirentp->d_reclen] = '\0';
				bytes_sent = send(new_fd, &(mydirentp->d_reclen), sizeof (unsigned short), 0); /*first 														send the size*/
   
				bytes_sent = send(new_fd, dirbuffer, mydirentp->d_reclen, 0);  //now send the dir name
			}
			
			bzero(dirbuffer, MAX_BUF_SIZE);
			unsigned short end_flag_dirent = 0;
			bytes_sent = send(new_fd, &end_flag_dirent, sizeof(unsigned short), 0);
			closedir(mydir);

		}
	//TCP "put" command
		else if ((rcvd_buffer[0] == 'p') && (rcvd_buffer[1] == 'u') && (rcvd_buffer[2] == 't'))
		{
			fp = fopen(&(rcvd_buffer[4]), "a+b"); //append read write binary mode 
			if (fp == NULL)
			{
				printf("Error opening the file to writing mode\n");
			}
			else
			{
				printf("OK! I understand you want to send me\n%s\n", &(rcvd_buffer[4]));
			}
			
			long int file_size = 0; //will be updated by the recv following recv() command
			bytes_rcvd = recv(new_fd, &file_size, sizeof(long int), 0); //recieve the size of the expected 											      file to arrive from the CLIENT
			int n = 9999; //temporary variable, only for purposes of initalization and testing 
			while(file_size > 0)
			{
				bzero(buffer, MAX_BUF_SIZE); //need to change it to a portable command like memset
				n = read(new_fd, buffer, MAX_BUF_SIZE-1); //read from new_fd to buffer up to 										    MAX_BUF_SIZE-1
				if (n < 0)
				{
					printf("Error reading from socket\n");
					break;
				}

				n = fwrite(buffer, 1, n, fp); //write the 'n' bytes read from buffer to fp
			
				if (n < 0)
				{
					printf("Error writing to file\n");
					break;
				}

				file_size -= n;
			}

			fseek(fp, 0, SEEK_END);
			file_size = ftell(fp);
			rewind(fp);
			fclose(fp);

			printf("Recieved a total of %ld bytes, to the file name %s\n", file_size, &(rcvd_buffer[4]));
			
		}

	//TCP "get" command
		else if ((rcvd_buffer[0] == 'g') && (rcvd_buffer[1] = 'e') && (rcvd_buffer[2] == 't')
			&& (rcvd_buffer[3] == ' '))
		{
			int file_flag;
			bzero(buffer, MAX_BUF_SIZE);
			fp = fopen(&(rcvd_buffer[4]), "rb");
			if (fp == NULL)
			{	
				file_flag = -1;
				bytes_sent = send(new_fd, &file_flag, sizeof file_flag, 0);
				if (bytes_sent == 0)
				{
					printf("Error sending data\n");
				}
			}
			else
			{
			    file_flag = 1;
			    bytes_sent = send(new_fd, &file_flag, sizeof file_flag, 0); //send file flag = 1
    			    if (bytes_sent == 0)
			    {
				printf("Error sending data\n");
			    }

			   printf("Found file %s on server\n", &(rcvd_buffer[4]));
			   
		       //calculate file size		   
			   fseek(fp, 0, SEEK_END);
			   file_size = ftell(fp);
			   rewind(fp);

		       //send the file size data to the client
			   bytes_sent = send(new_fd, &file_size, sizeof (file_size), 0);
			   if (bytes_sent == 0)
			   {
				printf("Error sending the data\n");
			   }

			   printf("File contains %ld bytes!\n", file_size);
			   printf("Sending file!\n");  

			   while (1)
			   {
				// read data into buffer, 
				// a loop is for the write, because not all of the data may be written
				// in one call; write will return how many bytes were written. ptr keeps
				// track of where in the buffer we are, while we decrement bytes_read
				// to keep track of how many bytes are left to write
				    bzero(buffer, MAX_BUF_SIZE);
				    bytes_read = fread(buffer, 1, MAX_BUF_SIZE, fp);
				    if (bytes_read == 0) //We're done reading from this file
				    {
					break;
				    }
	
				    if (bytes_read < 0)
				    {
					printf("Error reading from file\n");
				    }

			    	    void *ptr = buffer;
				    while (bytes_read > 0)
				    {
					int bytes_written = write(new_fd, buffer, bytes_read); 
					if (bytes_written <= 0)
					{
						printf("Error writing to socket\n");
					}
					bytes_read -= bytes_written;
					ptr += bytes_written; //to move forward in the file
				    }
								
		            }
			    printf("Done sending the file!\n\n");
			    fclose(fp);

			}
		}

	//TCP "cd" command
		else if ((rcvd_buffer[0] == 'c') && (rcvd_buffer[1] == 'd'))
		{	
			if (isblank(rcvd_buffer[2])) //the command is change directory to a new directory	
			{
				temp_rcvd_ptr = &(rcvd_buffer[3]);
			}
			else	//the command is change directory backword (cd..) or (cd.)
			{
				temp_rcvd_ptr = &(rcvd_buffer[2]);	
			}

			if (chdir(temp_rcvd_ptr) == 0) //the directory given by temp_rcvd_ptr is legit
			{
				if (getcwd(temp_rcvd_ptr, sizeof (rcvd_buffer) - 3) != NULL)
				{
					strcpy(cwd, temp_rcvd_ptr); //place dir to cwd 
					printf("current working directory is: %s\n", temp_rcvd_ptr);
					bzero(buffer, MAX_BUF_SIZE);
					strcpy(buffer, "okdir"); 
				     //send confirmation for the directory
					bytes_sent = send(new_fd, buffer, sizeof buffer, 0); 
					if (bytes_sent == 0)
					{
						printf("Error sending the data requested\n");
					}
				     //send the current directory						
					bytes_sent = send(new_fd, temp_rcvd_ptr, MAX_BUF_SIZE - 3, 0);
					if (bytes_sent == 0)
					{
						printf("Error sending the data requested\n");
					}

				}
				

			}
			else
			{
				printf("Incorrect directory given\n");
				bzero(buffer, MAX_BUF_SIZE);
				strcpy(buffer, "Incorrect directory entered");
				bytes_sent = send(new_fd, buffer, sizeof buffer, 0);
				if (bytes_sent == 0)
				{
					printf("Error sending the data requested\n");
				}
			}
		}
	//TCP "quit" command
		else if (strcmp("quit", rcvd_buffer) == 0)
		{
			if (select_flag == 1)
			{
				printf("selectserver: socket %d disconnected\n", new_fd);
				FD_CLR(new_fd, master); //remove socket from master set
				close(new_fd);
			}
			(*clientIndex)--; 
		}	
	}

	else if (mode == 1) //UDP connection protocol
	{
		bzero(rcvd_buffer, MAX_BUF_SIZE);
		strcpy(rcvd_buffer, msg_buffer);
		
	//ls server-handle (UDP)
		if (strcmp("ls", rcvd_buffer) == 0)
		{
			if (chdir(cwd) != 0)
			{
				printf("I've been asked to change the dir to %s", cwd);
				perror(NULL);
			}
		//open directory
			mydir = opendir(cwd);
	  		while (mydirentp = readdir(mydir))
			{	
				memcpy(dirbuffer, mydirentp->d_name, mydirentp->d_reclen);
				dirbuffer[mydirentp->d_reclen] = '\0';
				bytes_sent = sendto(new_fd, &(mydirentp->d_reclen), sizeof (unsigned short), 0, 							(struct sockaddr*)&their_addr, to_len); //first send the size 
				bytes_sent = sendto(new_fd, dirbuffer, mydirentp->d_reclen, 0, 
						(struct sockaddr*)&their_addr, to_len);  //now send the dir name
			}
			
			bzero(dirbuffer, MAX_BUF_SIZE);
			unsigned short end_flag_dirent = 0;
			bytes_sent = sendto(new_fd, &end_flag_dirent, sizeof(unsigned short), 0, 
									(struct sockaddr*)&their_addr, to_len);
			closedir(mydir);
		}

	//cd server-handle UDP
		else if ((rcvd_buffer[0] == 'c') && (rcvd_buffer[1] == 'd'))
		{	
			if (isblank(rcvd_buffer[2])) //the command is change directory to a new directory	
			{
				temp_rcvd_ptr = &(rcvd_buffer[3]);
			}
			else	//the command is change directory backword (cd..) or (cd.)
			{
				temp_rcvd_ptr = &(rcvd_buffer[2]);	
			}

			if (chdir(temp_rcvd_ptr) == 0) //the directory given by temp_rcvd_ptr is legit
			{
				if (getcwd(temp_rcvd_ptr, sizeof (rcvd_buffer) - 3) != NULL)
				{
					strcpy(cwd, temp_rcvd_ptr);
					printf("current working directory is: %s\n", temp_rcvd_ptr);
					bzero(buffer, MAX_BUF_SIZE);
					strcpy(buffer, "okdir"); 
				//send confirmation for the directory
					bytes_sent = sendto(new_fd, buffer, sizeof buffer, 0, 
									(struct sockaddr*)&their_addr, to_len); 
					if (bytes_sent == 0)
					{
						printf("Error sending the data requested\n");
					}
					else if (bytes_sent == -1)
					{
						perror(NULL);
					}

				//send the current directory						
					bytes_sent = sendto(new_fd, temp_rcvd_ptr, MAX_BUF_SIZE -3 , 0,
									 (struct sockaddr*)&their_addr, to_len); 
					if (bytes_sent == 0)
					{
						printf("Error sending the data requested\n");
					}
					else if (bytes_sent == -1)
					{
						perror(NULL);
					}
				}
			}
			else
			{
				printf("Incorrect directory given\n");
				bzero(buffer, MAX_BUF_SIZE);
				strcpy(buffer, "Incorrect directory entered");
				bytes_sent = sendto(new_fd, buffer, sizeof buffer , 0,
									 (struct sockaddr*)&their_addr, to_len); 
				if (bytes_sent == 0)
				{
					printf("Error sending the data requested\n");
				}
				else if (bytes_sent == -1)
				{
						perror(NULL);
				}
			}
		}

	//get server-handle (UDP)
		else if ((rcvd_buffer[0] == 'g') && (rcvd_buffer[1] = 'e') && (rcvd_buffer[2] == 't')
			&& (rcvd_buffer[3] == ' '))
		{
			int file_flag;
			bzero(buffer, MAX_BUF_SIZE);
			fp = fopen(&(rcvd_buffer[4]), "rb");
			if (fp == NULL)
			{	
				file_flag = -1;
				bytes_sent = sendto(new_fd, &file_flag, sizeof file_flag , 0,
									 (struct sockaddr*)&their_addr, to_len);
				if (bytes_sent == 0)
				{
					printf("Error sending data\n");
				}
			}
			else
			{
			   file_flag = 1;
			   bytes_sent = sendto(new_fd, &file_flag, sizeof file_flag , 0, 	 //send file flag = 1
									 (struct sockaddr*)&their_addr, to_len);
    			   if (bytes_sent == 0)
			   {
				printf("Error sending data\n");
			   }

			   printf("Found file %s on server\n", &(rcvd_buffer[4]));
			   
		   //calculate file size		   
			   fseek(fp, 0, SEEK_END);
			   file_size = ftell(fp);
			   rewind(fp);

	 	  //send the file size data to the client
			   bytes_sent = sendto(new_fd, &file_size, sizeof file_size , 0,
									 (struct sockaddr*)&their_addr, to_len);
			   if (bytes_sent == 0)
			   {
				printf("Error sending the data\n");
			   }

			   printf("File contains %ld bytes!\n", file_size);
			   printf("Sending file!\n");  


			   long int file_location = 0;
			   while(1)
			   {
				bzero(buffer, MAX_BUF_SIZE);
				bytes_read = fread(buffer, 1, MAX_BUF_SIZE, fp);
				if (bytes_read == 0) //We're done reading from this file
				{
					break;
				}
	
				if (bytes_read < 0)
				{
					printf("Error reading from file\n");
				}

				bytes_sent = sendto(new_fd, buffer, bytes_read , 0,
									 (struct sockaddr*)&their_addr, to_len); 
				if (bytes_sent == 0)
				{
					printf("Error transfering data\n");
				}
				else if (bytes_sent == -1)
				{
					perror(NULL);
				}
				file_location += bytes_read; //move forward in the file
				fseek(fp, file_location, SEEK_SET);
			   }
			   rewind(fp);
			   printf("Done sending the file!\n\n");
			   fclose(fp);
			}
		}
	//put server-handle udp
		else if ((rcvd_buffer[0] == 'p') && (rcvd_buffer[1] == 'u') && (rcvd_buffer[2] == 't'))
		{
			fp = fopen(&(rcvd_buffer[4]), "a+b"); //append read write binary mode 
			if (fp == NULL)
			{
				printf("Error opening the file to writing mode\n");
			}
			else
			{
				printf("OK! I understand you want to send me\n%s\n", &(rcvd_buffer[4]));
			}
			
			unsigned long file_size = 0;

		 //recieve the size of the file expected to arrive
			bytes_rcvd = recvfrom(new_fd, &file_size, sizeof file_size, 0, 
								(struct sockaddr *)&their_addr, &from_len); 
			while(file_size > 0)
			{	
				bzero(buffer, MAX_BUF_SIZE);
				bytes_rcvd = recvfrom(new_fd, buffer, MAX_BUF_SIZE, 0, 
								(struct sockaddr *)&their_addr, &from_len);
				fwrite(buffer, 1, bytes_rcvd, fp);
				file_size -= bytes_rcvd;
			}

			fseek(fp, 0, SEEK_END);
			file_size = ftell(fp);
			rewind(fp);
			fclose(fp);

			printf("Recieved a total of %ld bytes, to the file name %s\n", file_size, &(rcvd_buffer[4]));
		}	
	}
}


//---------------------------------------------------------------



void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

//---------------------------------------------------------------

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//---------------------------------------------------------------


int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	int pid;

	int bytes_sent, bytes_rcvd;

        struct sockaddr_storage remoteaddr; // client address
        socklen_t addrlen, to_len;
	//file variables
	FILE *fp;
	long int file_size;

    //dir variables
	DIR *mydir;
	char dirbuffer[MAX_BUF_SIZE];
	char buffer[MAX_BUF_SIZE];
	struct dirent *mydirentp;
	
	char in_opt;
	int dir_flag = 0, select_flag = 0; //input handle flags

    //select variables
	fd_set master;		//master file descriptor
	fd_set readfds;		//temp file descriptor list for select()
	int fdmax;		//maximum file descriptor number
	int i, j;
	char remoteIP[INET6_ADDRSTRLEN];

    //udp handle
	int udp_flag = 0;
	typedef struct clientStruct
	{
		int client_id;
		char ip[INET6_ADDRSTRLEN];
		char dir[MAX_BUF_SIZE];
		
	}clientStruct;	

	clientStruct clients[MAX_CLIENT_CONNECTION];
	int clientIndex = 0;
	to_len = sizeof(struct sockaddr_storage);
	socklen_t from_len = sizeof their_addr;
	

	for (i = 0; i < MAX_CLIENT_CONNECTION; i++)
	{
		bzero((clients[i]).dir, MAX_BUF_SIZE);
		strcpy(clients[i].dir, "/");
		clients[i].client_id = -1;
	}

	while(1)
	{
		in_opt = getopt(argc, argv, "usd:");
		if (in_opt == -1) //we finished processing all the arguments 
		{
			break;
		}
	
		switch(in_opt)
		{
			case 'd':
			{
				printf("User invoked -d %s \n", optarg);
				if (chdir (optarg) != 0)
				{
					printf("Invalid directory given, please try again\n");
					exit(1);

				}
				else
				{
					if (getcwd(buffer, sizeof buffer) != NULL)
					{
						printf("Setting default dir to: %s\n\n", optarg);
						dir_flag = 1;
					
					    //set the -d path as defualt directory for incoming users
						for (i = 0; i < MAX_CLIENT_CONNECTION; i++)
						{
							bzero((clients[i]).dir, MAX_BUF_SIZE);
							strcpy(clients[i].dir, optarg);
						}		
					}
				}
				break;
			}
			case 's':
			{
				printf("Select will be used instead of forking:\n");
				select_flag = 1;
				break;
			}
			case 'u':
			{
				printf("Ok, I will accept connections in UDP:\n");
				udp_flag = 1;
				break;
			}
			case '?':
			{
				printf("An unfarmilier handle type char inserted\n");
				break;
			}
			default:
			{
				printf("No opt char inserted\n");
			}
		}
	}

    //computition
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE; // use my IP
	
	if (udp_flag == 0)
	{
		hints.ai_socktype = SOCK_STREAM;
	}
	else //UDP-server mode selected
	{
		hints.ai_socktype = SOCK_DGRAM;
	}
	

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

    //loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

   //select handle
	if (select_flag == 0)
	{
		if ((udp_flag == 0) && (listen(sockfd, BACKLOG) == -1)) 
		{ //not select depedented
			perror("listen");
			exit(1);
		}
		sa.sa_handler = sigchld_handler; // reap all dead processes
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1)
		{
			perror("sigaction");
			exit(1);
		}
		if (udp_flag == 0)
		{
			printf("server: waiting for connections...\n");
		}
	}
	else //select dependent
	{
		FD_ZERO(&master);
		FD_ZERO(&readfds);
		if (listen(sockfd, BACKLOG) == -1)
		{
			printf("Error here!\n");
			perror("listen");
			exit(2);
		}
		FD_SET(sockfd, &master);
		fdmax = sockfd; //up until now, this is the max
	}



   //handle [non -d] directive
	if ((dir_flag == 0) && (chdir("/") != 0))
	{
		printf("Error: I couldn't allocate you with the '/' directory\n");
	}
	
	char tempbuffer[MAX_BUF_SIZE];
	if (getcwd(tempbuffer, sizeof tempbuffer) != NULL)
	{
		printf("Current dir is: %s\n", tempbuffer);
	}


	
	while(1) 
	{// main accept() loop
		if (select_flag == 0)
		{
			sin_size = sizeof their_addr;
			if (udp_flag == 0) //tcp connection
			{
			 	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
				if (new_fd == -1)
				{
					perror("accept");
					continue;
				}

				inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s,
													   sizeof s);
				printf("server: got connection from %s\n", s);

				/*create child process */
				pid = fork();
				if (pid < 0)
				{
					perror("Error on fork");
					exit(1);
				}
				if (pid == 0)
				{
					/*this is the client process*/
					while(1)
					{
						do_processing(new_fd, 0, select_flag, their_addr, 0, tempbuffer,
							   	clients[clientIndex].dir, &clientIndex, &master);
					}
				}
				else
				{
					close(new_fd);
				}
			}

		//----------------------- Start HANDLE UDP CONNECTION -----------------
		  	 else if (udp_flag == 1) //udp_flag == 1
			 {	
			   int ID;		
				bzero(&their_addr, from_len);

				bytes_rcvd = recvfrom(sockfd, tempbuffer, MAX_BUF_SIZE, 0, 
							(struct sockaddr*)&their_addr, &from_len);
				tempbuffer[bytes_rcvd] = '\0';		

			 //place the senders IP as a string on string s
				inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s,
													   sizeof s);
				if (bytes_rcvd == -1)
				{
					perror(NULL);
				}
				
				printf("I Recieved %s from IP: %s\n", tempbuffer, s);

				//check if this is a new connection
				int g, newConnectionFlag = 0;
				for (g = 0; g < MAX_CLIENT_CONNECTION; g++)
				{
					if (strcmp("handeshake", tempbuffer) == 0)
 					{ //meaning this ip is a new client
						newConnectionFlag = 1;
						
					   //send the new client his ID key
						bytes_sent = sendto(sockfd, &clientIndex, sizeof clientIndex, 0, 										(struct sockaddr*)&their_addr, to_len);
						if (bytes_sent == -1)
						{
							perror(NULL);
						}
						break; //end-for 
					}
					else
					{
						newConnectionFlag = 0;
					}
				}
				
				if (newConnectionFlag == 0) //meaning a known-connection
				{
			        //recieve client ID 
					bytes_rcvd = recvfrom(sockfd, &ID, sizeof ID, 0, 
							(struct sockaddr*)&their_addr, &from_len);
					do_processing(sockfd, udp_flag, select_flag, their_addr, 0, tempbuffer, 
									clients[ID].dir, &clientIndex, &master);
				}
				else if (newConnectionFlag == 1)
				{ //we found a new client!

				        strcpy(clients[clientIndex].ip, s);

					//We made an "handeshake" between the client-server already, now lets
					//get to buissness, get command from client
					printf("new connection!\n");
					do_processing(sockfd, udp_flag, select_flag, their_addr, 1, tempbuffer, 
							clients[clientIndex].dir, &clientIndex, &master); //handle request
					clientIndex++;  //increase the client's index (active clients)
				}
			 }
		     }
		//------------------------- END HANDLE UDP CONNECTION -----------------


		//--------------------------Handle -s (select instead of fork) option ---------
		else if (select_flag == 1) 
		{
			readfds = master;  //copy master
			if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1)
			{
				perror("select");
				exit(3);
			}
			// run through the existing connections looking for data to read
			for(i = 0; i <= fdmax; i++)
			{
			    if (FD_ISSET(i, &readfds)) 
			    { // we got one!!
				if (i == sockfd)
			        {
				    // handle new connections
				    addrlen = sizeof remoteaddr;
				    new_fd = accept(sockfd, (struct sockaddr *)&remoteaddr, &addrlen);

				    if (new_fd == -1) 
				    {
				        perror("accept");
				    }
				    else
				    {
				    	clients[clientIndex].client_id = new_fd;
			    		clientIndex++;					
				        FD_SET(new_fd, &master); // add to master set
				        if (new_fd > fdmax)     // keep track of the max
					{
				            fdmax = new_fd;
				        }
				        printf("selectserver: new connection from %s on socket %d\n",
					       inet_ntop(remoteaddr.ss_family,
				               get_in_addr((struct sockaddr*)&remoteaddr),
				               remoteIP, INET6_ADDRSTRLEN),new_fd);
				    }
				}
				else
				{
					//find the relevant user and get his cwd (clients[j].dir)
					int j;
					for (j = 0; j <= fdmax; j++)
					{
						if (i == clients[j].client_id)
						{
							break;
						}
					}

					do_processing(i, udp_flag, select_flag, their_addr, 0, tempbuffer, 
									clients[j].dir, &clientIndex, &master);
				} // END handle data from client
			    } // END got new incoming connection
			} // END looping through file descriptors
		     // END for(;;)
		}
	}//end while loop
}//end main		



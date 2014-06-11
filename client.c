/*
** client.c
** -------------------------------------------
** Owner: Daniel Better
** -------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

//dir includes
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

#define MAX_BUF_SIZE 256

#define SERVERPORT "3490"



//--------------------------------------------------------------


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // server's address information
	socklen_t from_len = sizeof (struct sockaddr);

	int rv;
	char s[INET6_ADDRSTRLEN];
	int bytes_sent; //number of bytes sent by the "send" function
	int bytes_rcvd;
	int msg_len;
	
    //udp handle
	int udp_flag = 0;
	char in_opt;
	int ID;

    //dir testing variables
	DIR *mydir;
	char dirbuffer[1000];
	struct dirent *mydirentp;

    //string working variables
	char tempbuffer[MAX_BUF_SIZE];
	char *tempBufferPtr;
	int bytes_read;

    //file paramaters
	FILE *fp;
	char rcvd_buffer[MAX_BUF_SIZE];
	unsigned long file_size;


	while(1)
	{
		in_opt = getopt(argc, argv, "u");
		if (in_opt == -1) //we finished processing all the arguments 
		{
			break;
		}
	
		switch(in_opt)
		{
			
			case 'u':
			{
				printf("Ok, I will connect in UDP protocol\n");
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




	if (udp_flag == 0) //tcp connection
	{

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		if((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
		{
	  		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	  		return 1;
		}
	
		// loop through all the results and connect to the first we can
		if (udp_flag == 0)
		{
			for(p = servinfo; p != NULL; p = p->ai_next)
			{
				if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
				{
					perror("client: socket");
					continue;
				}

				if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
				{
					close(sockfd);
					perror("client: connect");
					continue;
				}

				break;
			}
		}
	}
	else if (udp_flag == 1) //udp connection
	{
		    memset(&hints, 0, sizeof hints);
		    hints.ai_family = AF_UNSPEC;
		    hints.ai_socktype = SOCK_DGRAM;

		    if ((rv = getaddrinfo(argv[2], SERVERPORT, &hints, &servinfo)) != 0)
		    {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		    }

		    // loop through all the results and make a socket
		    for(p = servinfo; p != NULL; p = p->ai_next)
		    {
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		        {
			    perror("talker: socket");
			    continue;
			}

			break;
		    }

		    if (p == NULL)
	 	    {
			fprintf(stderr, "talker: failed to bind socket\n");
			return 2;
		    }

		    bzero(tempbuffer, MAX_BUF_SIZE);
		    strcpy(tempbuffer, "handeshake");

		//Send handshake to the server
  		    bytes_sent = sendto(sockfd, tempbuffer, MAX_BUF_SIZE, 0, p->ai_addr, p->ai_addrlen);
		//recieve ID key from server
		    bytes_rcvd = recvfrom(sockfd, &ID, sizeof ID, 0, (struct sockaddr *)&their_addr, &from_len);

    		    bzero(tempbuffer, MAX_BUF_SIZE);
	            if (bytes_sent == -1)
		    {
			perror(NULL);
		    }
	}

	



	/*This code block is meant to contain and maintain the "/" starting point of the client/server
	  as requested in the excercise
	*/
	if (chdir("/") != 0)
	{
		printf("Error: I couldn't allocate you with the '/' directory\n");
	}
	else
	{
	 	if (getcwd(tempbuffer, sizeof tempbuffer) != NULL)
		{
			printf("Current local dir is: %s\n", tempbuffer);
		}
	}
	

	
	//main code block
	while (1)
	{
           if (udp_flag == 0) //TCP Connection
	   {
		gets(tempbuffer);
		if (strcmp(tempbuffer, "ls") == 0)		
		{
		   //send "ls" as a messge to the server
			msg_len = strlen(tempbuffer);
			bytes_sent = send(sockfd, tempbuffer, MAX_BUF_SIZE, 0);

			if (bytes_sent == 0)
			{
				printf("Error sending all the data wanted, please try again.\n");
			}

			int done_flag = 0;
			char dir[MAX_BUF_SIZE];
			char curr_dir[MAX_BUF_SIZE];
			int i;
			char curr_ch;
			unsigned short dir_size = 1; 
			
			bzero(curr_dir, MAX_BUF_SIZE); //set the curr_dir buffer to an empty buffer
	
			
			while (dir_size != 0)
			{
				bytes_rcvd = recv(sockfd, &dir_size, sizeof dir_size, 0);
				if ((bytes_rcvd == 0) || (dir_size == 0))
				{
					break;
				}
				bytes_rcvd = recv(sockfd, dirbuffer, dir_size, 0);
				printf("%s\n", dirbuffer); 						
			}
			printf("\n");
			
		}

	   //input from user was "cd" -> transfer that message using the client to the SERVER for handling
		else if ((tempbuffer[0] == 'c') && (tempbuffer[1] == 'd'))
		{
			char validation_buffer[MAX_BUF_SIZE];
			bzero(validation_buffer, MAX_BUF_SIZE);
	
			msg_len = strlen(tempbuffer);
			bytes_sent = send(sockfd, tempbuffer, msg_len, 0);

			if (bytes_sent == 0)
			{
				printf("Error sending all the data wanted, please try again.\n");
			}
			bytes_rcvd = recv(sockfd,  validation_buffer, sizeof  validation_buffer, 0);
			if (strcmp(validation_buffer, "okdir") == 0)
			{

				bytes_rcvd = recv(sockfd, tempbuffer, MAX_BUF_SIZE - 3, 0);
				printf("current working directory is: %s\n", tempbuffer);
			}
			else if (strcmp(validation_buffer,"Incorrect directory entered") == 0)
			{
				printf("Incorrect directory entered, please try again\n");
			}
		}


		else if (strcmp(tempbuffer, "quit") == 0)
		{
			bytes_sent = send(sockfd, tempbuffer, MAX_BUF_SIZE, 0);
			if (bytes_sent == -1)
			{
				perror(NULL);
			}
			exit(1);
		}
		
	 //lcd command - client only
		else if ((tempbuffer[0] == 'l') && (tempbuffer[1] == 'c') && (tempbuffer[2] = 'd'))
		{	
			if (isblank(tempbuffer[3])) //the command is change directory to a new directory	
			{
				tempBufferPtr = &(tempbuffer[4]);
			}
			else	//the command is change directory backword (cd..) or (cd.)
			{
				tempBufferPtr = &(tempbuffer[3]);	
			}

			if (chdir(tempBufferPtr) == 0) //the directory given by tempBufferPtr is legit
			{
				if (getcwd(tempBufferPtr, sizeof (tempbuffer) - 4) != NULL)
				{
					printf("current working directory is: %s\n", tempBufferPtr);
				}
			}
			else
			{
				printf("Incorrect directory given\n");
			}
		}

	//put TCP command
		else if ((tempbuffer[0] == 'p') && (tempbuffer[1] == 'u') && (tempbuffer[2] = 't')
			 && (tempbuffer[3] =' '))
		{		
			fp = fopen(&(tempbuffer[4]), "rb");     //open the file
			if (fp == NULL)
			{
				printf("Error opening the file, or non-existing file, try again\n");
			}
			else
			{

			   printf("Found file %s\n", &(tempbuffer[4]));
			   bytes_sent = send(sockfd, tempbuffer, MAX_BUF_SIZE, 0); //send command put to the server
	   
			   fseek(fp, 0, SEEK_END);
			   file_size = ftell(fp);
			   rewind(fp);

			   bytes_sent = send(sockfd, &file_size, sizeof(long int), 0); //send the size of the file to 												be transfered to the SERVER

			   if (bytes_sent == 0)
			   {
				printf("Error sending the data\n");
			   }

			   printf("File contains %ld bytes!\n", file_size);
			   printf("Sending file!\n"); 
			
			   while (1)
			   {
				    //read data into buffer, 
				    // You need a loop for the write, because not all of the data may be written
				    // in one call; write will return how many bytes were written. ptr keeps
				    // track of where in the buffer we are, while we decrement bytes_read
				    // to keep track of how many bytes are left to write
				  
				    bytes_read = fread(tempbuffer, 1, MAX_BUF_SIZE, fp);
				    if (bytes_read == 0) //We're done reading from this file
				    {
					break;
				    }
	
				    if (bytes_read < 0)
				    {
					printf("Error reading from file\n");
				    }

			    	    void *ptr = tempbuffer;
				    while (bytes_read > 0)
				    {
					int bytes_written = write(sockfd, tempbuffer, bytes_read); 
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
		
	//get tcp command
		else if ((tempbuffer[0] == 'g') && (tempbuffer[1] == 'e') && (tempbuffer[2] == 't') &&
			 (tempbuffer[3] == ' '))
		{
			int file_flag = 0;
			bytes_sent = send(sockfd, tempbuffer, MAX_BUF_SIZE, 0);    //send command get to server
	
			if (bytes_sent == 0)
			{
				printf("Error sending data\n");

			}
			bytes_rcvd = recv(sockfd, &file_flag, sizeof file_flag, 0); //recieve indection on file
									 	    // via file_flag
			if (bytes_rcvd == 0)
			{
				printf("Error reciving data\n");
			}

			if (file_flag == -1)
			{
				printf("Wrong file name entered, please try again\n");
			}
			else if (file_flag == 1)
			{
				printf("Found file %s on server\n", &(tempbuffer[4]));
				bytes_rcvd = recv(sockfd, &file_size, sizeof (file_size), 0); //get file size
				if (bytes_rcvd == 0)
				{
					printf("Error reciving data\n");
				}

				fp = fopen(&(tempbuffer[4]), "a+b"); 		//append read write binary mode
				if (fp == NULL) 
				{
					printf("Error creating file %s, exiting\n", &(tempbuffer[4]));
					exit(1);
				}

				
				int n = 9999; //temporary variable, only for purposes of testing 
				while(file_size > 0)
				{
					bzero(rcvd_buffer, MAX_BUF_SIZE); //need to change it to a portable command 										    like memset
					n = read(sockfd, rcvd_buffer, MAX_BUF_SIZE-1); //read from sockfd to buffer   										                 up to MAX_BUF_SIZE-1
					if (n < 0)
					{
						printf("Error reading from socket\n");
						break;
					}

					n = fwrite(rcvd_buffer, 1, n, fp); //write the 'n' bytes read from buffer to fp
			
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
				printf("Recieved a total of %ld bytes, to the file name %s\n\n", file_size,
											       &(tempbuffer[4]));

			}
		
		}
	    }
	    else if (udp_flag == 1) //UDP connection protocol
	    {
		gets(tempbuffer);

	   //ls client-handle UDP
		if (strcmp(tempbuffer, "ls") == 0)		
		{
			//send "ls" as a messge to the server
			msg_len = strlen(tempbuffer);
			bytes_sent = sendto(sockfd, tempbuffer, MAX_BUF_SIZE, 0, p->ai_addr, p->ai_addrlen);
	
		//send ID to server
			bytes_sent = sendto(sockfd, &ID, sizeof ID, 0, p->ai_addr, p->ai_addrlen);
	
			if (bytes_sent == 0)
			{
				printf("Error sending all the data wanted, please try again.\n");
			}
			if (bytes_sent == -1)
			{
				perror(NULL);
			}
			else
			{
				printf("sent %d bytes\n", bytes_sent);
			}
			int done_flag = 0;
			char dir[MAX_BUF_SIZE];
			char curr_dir[MAX_BUF_SIZE];
			int i;
			char curr_ch;
			
			bzero(curr_dir, MAX_BUF_SIZE); //set the curr_dir buffer to an empty buffer
	
			unsigned short dir_size = 1; 
			while (dir_size != 0)
			{
				bytes_rcvd = recvfrom(sockfd, &dir_size, sizeof dir_size, 0,
								 (struct sockaddr *)&their_addr, &from_len);

				if ((bytes_rcvd == 0) || (dir_size == 0))
				{
					break;
				}
				bytes_rcvd = recvfrom(sockfd, dirbuffer, dir_size, 0, 
								(struct sockaddr *)&their_addr, &from_len);
				printf("%s\n", dirbuffer); 						
			}
			printf("\n");
			
		}
	    //cd server-handle UDP
		else if ((tempbuffer[0] == 'c') && (tempbuffer[1] == 'd'))
		{
			char validation_buffer[MAX_BUF_SIZE];
			bzero(validation_buffer, MAX_BUF_SIZE);
		
			msg_len = strlen(tempbuffer);
			bytes_sent = sendto(sockfd, tempbuffer, msg_len, 0, p->ai_addr, p->ai_addrlen);
		//send ID to server
			bytes_sent = sendto(sockfd, &ID, sizeof ID, 0, p->ai_addr, p->ai_addrlen);

			if (bytes_sent == 0)
			{
				printf("Error sending all the data wanted, please try again.\n");
			}

			bytes_rcvd = recvfrom(sockfd, validation_buffer, sizeof validation_buffer, 0, 
								(struct sockaddr *)&their_addr, &from_len);

			if (bytes_rcvd == -1)
			{
				perror(NULL);
			}


			if (strcmp(validation_buffer, "okdir") == 0)
			{
				bytes_rcvd = recvfrom(sockfd, tempbuffer, MAX_BUF_SIZE -3, 0, 
								(struct sockaddr *)&their_addr, &from_len);
				printf("current working directory is: %s\n", tempbuffer);
			}
			else if (strcmp(validation_buffer,"Incorrect directory entered") == 0)
			{
				printf("Incorrect directory entered, please try again\n");
			}
		}


	//lcd client-handle (client exclusive)

		//check if the inputted command is a lcd command and act accordingly
		else if ((tempbuffer[0] == 'l') && (tempbuffer[1] == 'c') && (tempbuffer[2] = 'd'))
		{	
			if (isblank(tempbuffer[3])) //the command is change directory to a new directory	
			{
				tempBufferPtr = &(tempbuffer[4]);
			}
			else	//the command is change directory backword (cd..) or (cd.)
			{
				tempBufferPtr = &(tempbuffer[3]);	
			}

			if (chdir(tempBufferPtr) == 0) //the directory given by tempBufferPtr is legit
			{
				if (getcwd(tempBufferPtr, sizeof (tempbuffer) - 4) != NULL)
				{
					printf("current working directory is: %s\n", tempBufferPtr);
				}
			}
			else
			{
				printf("Incorrect directory given\n");
			}
		}

	//get client-handle UDP
		else if ((tempbuffer[0] == 'g') && (tempbuffer[1] == 'e') && (tempbuffer[2] == 't') &&
			 (tempbuffer[3] == ' '))
		{
			int file_flag = 0;
		   //send command get to server
			bytes_sent = sendto(sockfd, tempbuffer, MAX_BUF_SIZE, 0, p->ai_addr, p->ai_addrlen); 
		   //send ID to server
			bytes_sent = sendto(sockfd, &ID, sizeof ID, 0, p->ai_addr, p->ai_addrlen);
			if (bytes_sent == 0)
			{
				printf("Error sending data\n");

			}
			bytes_rcvd = recvfrom(sockfd, &file_flag, sizeof file_flag, 0, 
								(struct sockaddr *)&their_addr, &from_len);
			if (bytes_rcvd == 0)
			{
				printf("Error reciving data\n");
			}

			if (file_flag == -1)
			{
				printf("Wrong file name entered, please try again\n");
			}
			else if (file_flag == 1)
			{
				printf("Found file %s on server\n", &(tempbuffer[4]));
				bytes_rcvd = recvfrom(sockfd, &file_size, sizeof file_size, 0, 
								(struct sockaddr *)&their_addr, &from_len);
				if (bytes_rcvd == 0)
				{
					printf("Error reciving data\n");
				}

				fp = fopen(&(tempbuffer[4]), "a+b"); 	//append read write binary mode
				if (fp == NULL) 
				{
					printf("Error creating file %s, exiting\n", &(tempbuffer[4]));
					exit(1);
				}

				while (file_size > 0)
				{
					bytes_rcvd = recvfrom(sockfd, rcvd_buffer, MAX_BUF_SIZE, 0, 
								(struct sockaddr *)&their_addr, &from_len);
					fwrite(rcvd_buffer, 1, bytes_rcvd, fp);
					file_size -= bytes_rcvd;
				}


				fseek(fp, 0, SEEK_END);
				file_size = ftell(fp);
				rewind(fp);
				fclose(fp);
				printf("Recieved a total of %ld bytes, to the file name %s\n\n", file_size,
											       &(tempbuffer[4]));
			}
		
		}
	
	//put client-handle UDP
		else if ((tempbuffer[0] == 'p') && (tempbuffer[1] == 'u') && (tempbuffer[2] = 't')
			 && (tempbuffer[3] =' '))
		{		
			fp = fopen(&(tempbuffer[4]), "rb");     //open the file
			if (fp == NULL)
			{
				printf("Error opening the file, or non-existing file, try again\n");
			}
			else
			{

			   printf("Found file %s\n", &(tempbuffer[4]));

			   //send command put to the server
			   bytes_sent = sendto(sockfd, tempbuffer, MAX_BUF_SIZE, 0, p->ai_addr, p->ai_addrlen); 
	    	   	   //send ID to server
			   bytes_sent = sendto(sockfd, &ID, sizeof ID, 0, p->ai_addr, p->ai_addrlen);
			   fseek(fp, 0, SEEK_END);
			   file_size = ftell(fp);
			   rewind(fp);
		
			//send the size of the file to be transfered to the SERVER
			  bytes_sent = sendto(sockfd, &file_size, sizeof(long int), 0, p->ai_addr, p->ai_addrlen); 

			   if (bytes_sent == 0)
			   {
				printf("Error sending the data\n");
			   }

			   printf("File contains %ld bytes!\n", file_size);
			   printf("Sending file!\n"); 
			

			   long int file_location = 0;
			   while(1)
			   {
				bzero(tempbuffer, MAX_BUF_SIZE);
				bytes_read = fread(tempbuffer, 1, MAX_BUF_SIZE, fp);
				if (bytes_read == 0) //We're done reading from this file
				{
					break;
				}
	
				if (bytes_read < 0)
				{
					printf("Error reading from file\n");
				}

				bytes_sent = sendto(sockfd, tempbuffer, bytes_read , 0, p->ai_addr, p->ai_addrlen); 
				if (bytes_sent == 0)
				{
					printf("Error transfering data\n");
				}
				else if (bytes_sent == -1)
				{
					perror(NULL);
					printf("Failure here!\n");
				}
				file_location += bytes_read; //move forward in the file
				fseek(fp, file_location, SEEK_SET);
			   }
			   rewind(fp);
			   printf("Done sending the file!\n\n");
			   fclose(fp);
			}
		}


	//quit client-handle (client exclusive)
		else if (strcmp(tempbuffer, "quit") == 0)
		{
			printf("Quiting program\n");
			exit(1);
		}
		
	    }
	}
		return 0;
}


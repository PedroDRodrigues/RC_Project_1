//1.º Projeto de Redes de Computadores - PLAYER.C
//Leonor Marques - 99262
//Pedro Rodrigues - 99300
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define GN 32 					//numero do grupo 
#define	_GSport	58000			//GS port
#define BUFFERSIZE 128		 
#define BUFFERSIZE_AUX 31
#define MAX_SIZE_PLID 7  

int fdServerUDP, fdClientUDP;
int fdServerTCP, fdClientTCP;
int errcode;
int trial, correct_trials, position;
int max_errors, word_size;
int started;

char buffer[BUFFERSIZE];
char buffer_aux[BUFFERSIZE];
char PLID[MAX_SIZE_PLID];
char word[BUFFERSIZE_AUX];
char final_word[BUFFERSIZE_AUX];
char guess_word[BUFFERSIZE_AUX];
char letter;

char *status, *command;
char *GSIP, *GSport;
char *Fname, *Fsize;

ssize_t n;
socklen_t addrlen;
struct addrinfo hintsClientUDP, *resClientUDP;
struct addrinfo hintsClientTCP, *resClientTCP;
struct sockaddr_in addr;
struct timeval tv;
extern int errno;

void readInput(int argc, char *argv[]);
void initClientUDP();
void initClientTCP();
void createTCPConnection();

/* UDP */
void SNG();
void PLG();
void PWG();
void QUT();
void REV();

/* TCP */
void GSB();
void GHL();
void STA();

/* ERRO E FREES */
void ERR();
void freeAll();

/* ----------------------------------------------------------- */
/* -----------------------  AUXILIARES ----------------------- */
/* ----------------------------------------------------------- */

int validPLID(char *PLID) //validação do PLID - player ID
{
	int i;
	if(strlen(PLID) != 6) //numero de estudante do IST com 6 digitos
		return 0;
	for(i = 0; i < 6; i++) 
		if(PLID[i] < '0' || PLID[i] > '9')
			return 0;
	return 1;
}

int isLetter(char c) //confirmação se o char recebido é uma letra
{
	if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		return 1;
	else
		return 0;
}

char* malloc_str(char* source) //malloc para strings do IP e da PORT
{
    char* dest;
    dest = malloc((strlen(source) + 1) * sizeof(char));
    if(dest)
	{
        n = snprintf(dest, strlen(source) + 1, "%s", source);
        if (n >= 0)
            return dest;
    }
	else
	{
   		printf("malloc_str(char* source) -> error;\n");
    	exit(1);
	}
}

/* ----------------------------------------------------------- */
/* -------------------- FIM DAS AUXILIARES ------------------- */
/* ----------------------------------------------------------- */

/* Le input -> ./Player [-n GSIP] [-p GSport] */
void readInput(int argc, char *argv[])
{
	if(argc != 1 && argc != 3 && argc != 5)
	{
		printf("readInput(int argc, char *argv[]) -> error;\n");
		exit(1);
	}
	
	for(int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if(argv[i][1] == 'n')
			{
				GSIP = malloc_str(argv[i + 1]);
				i++;
			}
			else if (argv[i][1] == 'p')
			{
				GSport = malloc_str(argv[i + 1]);
				i++;
			}
			else
			{
				exit(1);
			}
		}
	}

    if(GSIP == NULL)
	{
		GSIP = malloc_str("localhost");
	}

    if (!GSport)
	{
		GSport = malloc_str("58032"); //58000 + GN
	}
}


/* Ligacao UDP ao servidor GS */
void initClientUDP()
{
	fdClientUDP = socket(AF_INET,SOCK_DGRAM,0);
	if(fdClientUDP == -1)
		exit(1);

	memset(&hintsClientUDP, 0, sizeof hintsClientUDP);	//UDP SOCKET 
	hintsClientUDP.ai_family = AF_INET;			        //IPv4 
	hintsClientUDP.ai_socktype = SOCK_DGRAM;		    //UDP SOCKET 

	errcode = getaddrinfo(GSIP, GSport, &hintsClientUDP, &resClientUDP);
	if(errcode != 0) //ERROR
		exit(1);

    tv.tv_sec = 5;
    tv.tv_usec = 0;
	if(setsockopt(fdClientUDP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    	exit(1);
}

/* Ligacao TCP ao servidor GS */
void initClientTCP()
{
	fdClientTCP = socket(AF_INET,SOCK_STREAM,0);
	if(fdClientTCP == -1)
	{
		exit(1);
	}

	memset(&hintsClientTCP, 0, sizeof hintsClientTCP);	//TCP SOCKET 
	hintsClientTCP.ai_family = AF_INET;			        //IPv4 
	hintsClientTCP.ai_socktype = SOCK_STREAM;		    //TCP SOCKET 

	errcode = getaddrinfo(GSIP, GSport, &hintsClientTCP, &resClientTCP);
	if(errcode != 0) //ERROR
		exit(1);

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if(setsockopt(fdClientTCP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		exit(1);

	freeaddrinfo(resClientTCP);
}

/* TCP Connection */
void createTCPConnection(int fdClientTCP, char* GSIP, char* GSport)
{
    memset(&hintsClientTCP, 0, sizeof hintsClientTCP);
    hintsClientTCP.ai_family = AF_INET;
    hintsClientTCP.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(GSIP, GSport, &hintsClientTCP, &resClientTCP);
    if((errcode) != 0)
	{
        perror("Getaddrinfo failed\n");
        exit(1);
    }

    n = connect(fdClientTCP, resClientTCP->ai_addr, resClientTCP->ai_addrlen);
    if(n == -1)
	{
        perror("Connection failed do TCP Connection\n");
        exit(1);
    }

    tv.tv_sec = 20;
    tv.tv_usec = 0;
}

/* envia o conteudo do buffer para o GS */
void sendMessageTCP(int fd, char* message, ssize_t size_aux)
{
    int i;
    char *pointer;

    for(i = size_aux, pointer = &message[0]; i > 0; i -= n, pointer += n)
	{
        if((n = write(fd, pointer, i)) <= 0)
		{
            ERR();
            exit(1);
        }
    }   
}

/* Liberta toda a memoria */
void freeAll()
{
	memset(&hintsClientTCP, 0, sizeof(struct addrinfo));
    memset(&hintsClientUDP, 0, sizeof(struct addrinfo));
	
    hintsClientTCP.ai_addr = NULL;
    hintsClientTCP.ai_next = NULL;
	hintsClientUDP.ai_addr = NULL;
    hintsClientUDP.ai_next = NULL;

	freeaddrinfo(resClientUDP);	
	close(fdClientUDP);
	free(GSIP);
	free(GSport);
}

/* ......................... FUNCOES UDP ........................ */

/* comando: SNG PLID */
void SNG()
{
	int nargs;
	
	if(sscanf(buffer, "start %s\n", PLID) == 1)
	{
		nargs = sscanf(buffer, "start %s\n", PLID);
	}
	else
	{
		nargs = sscanf(buffer, "sg %s\n", PLID);
	}
	
	if(nargs != 1) 
	{
		ERR();
		return;
	}

	int size = sprintf(buffer, "SNG %s\n", PLID); //compor a mensagem

	n = sendto(fdClientUDP, buffer, size, 0, resClientUDP->ai_addr, resClientUDP->ai_addrlen); //enviar a mensagem para o GS	
	if(n == -1)
		exit(1);

	memset(buffer, '\0', BUFFERSIZE);

	addrlen = sizeof(addr);

	n = recvfrom(fdClientUDP, buffer, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen); //receber a mensagem do GS
	if(n == -1)
	{
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			printf("Connection Failed do SNG\n");
			return;
		}
		exit(1);
	}

	/*
		comando: RSG status [n_letters max_errors]
	*/

	status = malloc(4*sizeof(char));
	nargs = sscanf(buffer, "RSG %s %d %d\n", status, &word_size, &max_errors); //divisão do buffer

	if(strcmp(status, "OK") == 0) //RSG OK
	{
		started = 1;
		printf("New game started (max %d errors): ", max_errors); 

		for(int i = 0; i < word_size; i++)
		{
			printf(" _");
		}
		printf("\n");
	}
	else if(strcmp(status, "NOK") == 0) //RSG NOK
	{
		printf("Unsuccessful start - RSG NOK\n");
	}
	else //ERR
	{
		ERR();
	}

	memset(buffer, '\0', BUFFERSIZE);
	memset(status, '\0', 4);
	free(status);
}

/* comando: PLG PLID letter trial */
void PLG()
{
	int nargs;	

	if(sscanf(buffer, "play %c\n", &letter) == 1)
	{
		nargs = sscanf(buffer, "play %c\n", &letter);
	}
	else if(sscanf(buffer, "pl %c\n", &letter) == 1)
	{
		nargs = sscanf(buffer, "pl %c\n", &letter);
	}
	else
	{
		printf("Error no PLG - invalid command\n");
		return;
	}

	memset(buffer, '\0', BUFFERSIZE);

	int size = sprintf(buffer, "PLG %s %c %d\n", PLID, letter, trial); //compor a mensagem

	n = sendto(fdClientUDP, buffer, size, 0, resClientUDP->ai_addr, resClientUDP->ai_addrlen); //enviar a mensagem para o GS
	if(n == -1)
	{
		exit(1);
	}

	memset(buffer, '\0', BUFFERSIZE);

	addrlen = sizeof(addr);

	n = recvfrom(fdClientUDP, buffer, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen); //receber a mensagem do GS
	if(n == -1)
	{
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			printf("Connection Failed do PLG\n");
			return;
		}
		exit(1);
	}

	/* 
		comando: RLG status trial [n pos*] 
	*/
	status = malloc(4*sizeof(char));
	nargs = sscanf(buffer, "RLG %s", status);  
	
	if(nargs != 1) //validation
	{
		printf("Error no PLG - invalid status\n");
		memset(status, '\0', 4);
		free(status);
		return;
	}
	
	if(strcmp(status, "OK") == 0) //RLG OK
	{
		nargs = sscanf(buffer, "RLG %s %d %d %[^\n]", status, &trial, &correct_trials, buffer_aux); 
		if(nargs != 4)
		{
			ERR();
			return;
		}

		int mini_pointer = 0; 
		int i = 0;

		while(i < correct_trials)
		{
			position = buffer_aux[mini_pointer] - '0';
			int position_aux = buffer_aux[mini_pointer + 1] - '0';
			
			if(position_aux >= 0 && position_aux < 10)
			{
				position = position * 10 + position_aux;
				word[position-1] = toupper(letter);
				mini_pointer += 3;
				i++;
			}
			else
			{
				word[position-1] = toupper(letter);
				mini_pointer += 2;
				i++;
			}
		}

		printf("Yes, '%c' is part of the word: ", letter); 

		for(int i = 0; i < word_size; i++)
		{
			if (isLetter(word[i]) == 1)
			{
				printf(" %c", word[i]);
			}
			else
			{
				printf(" _");
			}
		}
		printf("\n");
		trial++;
	}
	else if(strcmp(status, "WIN") == 0) //RLG WIN
	{
		nargs = sscanf(buffer, "RLG %s %d\n", status, &trial); 
		if(nargs != 2)
		{
			ERR();
			return;
		}

		for (int i = 0; i < word_size; i++)
		{
			if(word[i] == '\0')
			{
				word[i] = toupper(letter);
			}
		}
		
		printf("WIN! Yes, you guessed the word: %s\n", word);
		trial++;
		started = 0;
		QUT();
	}
	else if(strcmp(status, "DUP") == 0) //RLG DUP
	{
		char letter_aux = toupper(letter);
		printf("DUP! You have already tried '%c': ", letter_aux);
		
		for(int i = 0; i < word_size; i++)
		{
			if (isLetter(word[i]) == 1)
			{
				word[i] = toupper(word[i]);
				printf(" %c", word[i]);
			}
			else
			{
				printf(" _");
			}
		}
		printf("\n");
	}
	else if(strcmp(status, "NOK") == 0) //RLG NOK
	{
		nargs = sscanf(buffer, "RLG %s %d\n", status, &trial); 
		if(nargs != 2)
		{
			ERR();
			return;
		}

		char letter_aux = toupper(letter);
		printf("No, '%c' is not part of the word: ", letter_aux);
		
		for(int i = 0; i < word_size; i++)
		{
			if (isLetter(word[i]) == 1)
			{
				word[i] = toupper(word[i]);
				printf(" %c", word[i]);
			}
			else
			{
				printf(" _");
			}
		}
		printf("\n");
		trial++; 
	}
	else if(strcmp(status, "OVR") == 0) //RLG OVR
	{
		char letter_aux = toupper(letter);
		printf("GAME OVER! No, '%c' is not part of the word!\n", letter_aux);
		started = 0;
		QUT();
	}
	else if(strcmp(status, "INV") == 0) //RLG INV
	{
		nargs = sscanf(buffer, "RLG %s %d\n", status, &trial); 
		if(nargs != 2)
		{
			ERR();
			return;
		}
		printf("INV! Trial '%d' is not valid.\n", trial);
	}
	else if(strcmp(status, "ERR") == 0) //RLG ERR
	{
		printf("ERR! No game ongoing\n");
	}
	else //ERR
	{
		ERR();
	}

	memset(status, '\0', 4);	
	memset(buffer, '\0', BUFFERSIZE);
	free(status);
	return;
}

/* comando: PWG PLID word trial */
void PWG()
{
	int nargs;
	if(sscanf(buffer, "guess %s\n", guess_word) == 1)
	{
		nargs = sscanf(buffer, "guess %s\n", guess_word);
	}
	else
	{
		nargs = sscanf(buffer, "gw %s\n", guess_word);
	}

	if(nargs != 1)
	{
		n = sendto(fdClientUDP, "ERR\n", 4, 0, resClientUDP->ai_addr, resClientUDP->ai_addrlen); //ERR
		if(n == -1)
			exit(1);
		return;
	}

	memset(buffer, '\0', BUFFERSIZE);

	int size = sprintf(buffer, "PWG %s %s %d\n", PLID, guess_word, trial); //compose message

	n = sendto(fdClientUDP, buffer, size, 0, resClientUDP->ai_addr, resClientUDP->ai_addrlen); //send message to GS
	if(n == -1)
		exit(1);

	memset(buffer, '\0', BUFFERSIZE);

	addrlen = sizeof(addr);

	n = recvfrom(fdClientUDP, buffer, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen); //receive message from GS
	if(n == -1)
	{
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			printf("Connection Failed do PWG\n");
			return;
		}
		exit(1);
	}
	
	/*
		comando: RWG status trial
	*/

	if(strcmp(buffer, "RLG ERR\n") == 0)
	{
		printf("ERR! No game ongoing\n");
		return;
	}

	status = malloc(4*sizeof(char));
	nargs = sscanf(buffer, "RWG %s %d", status, &trial); //divisions of the buffer
	
	if(nargs != 2) //validation
	{
		ERR();
		return;
	}

	for(int i = 0; i < word_size; i++)
	{
		guess_word[i] = toupper(guess_word[i]);
	}

	if(strcmp(status, "WIN") == 0) //RWG OK
	{
		printf("WELL DONE! You guessed: %s\n", guess_word);
		started = 0;
		QUT();
	}
	else if(strcmp(status, "NOK") == 0) //RWG NOK
	{
		printf("TRY AGAIN! You did not guess the word\n");
		trial++;
	}
	else if(strcmp(status, "OVR") == 0) //RWG OVR
	{
		printf("GAME OVER! No more chances!\n");
		started = 0;
		QUT();
	}
	else if(strcmp(status, "INV") == 0) //RWG INV
	{
		printf("Incorrect trial number on RWG\n");
	}
	else //ERR
	{
		ERR();
	}
	
	memset(status, '\0', 4);
	free(status);
	return;
}

/* comando: QUT PLID */
void QUT()
{	
	if(strcmp(buffer, "exit\n") != 0 && strcmp(buffer, "quit\n") != 0 && started == 0)
	{
		letter = '\0';
		trial = 1;
		memset(guess_word, '\0', word_size);
		memset(word, '\0', word_size);
		return;
	}

	int exit_flag = 0;
	if(strcmp(buffer, "exit\n") == 0)
	{
		exit_flag = 1;
	}

	if(started == 0 && strcmp(buffer, "quit\n") == 0)
	{
		printf("ERR! No game ongoing\n");
		return;
	}	

	memset(buffer, '\0', BUFFERSIZE);

	int size = sprintf(buffer, "QUT %s\n", PLID); //command of message

	n = sendto(fdClientUDP, buffer, size, 0, resClientUDP->ai_addr, resClientUDP->ai_addrlen); //envia mensagem para o GS
	if(n == -1)
	{
		exit(1);	
	}
	
	memset(buffer, '\0', BUFFERSIZE);

	addrlen = sizeof(addr);
	
	n = recvfrom(fdClientUDP, buffer, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen); //recebe mensagem do GS
	if(n == -1)
	{
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			printf("Connection Failed do QUT\n");
			return;
		}
		exit(1);
	}

	/*
		comando: RQT status
	*/

	if(exit_flag == 1)
	{
		return;
	}
	else if(strcmp(buffer, "RQT OK\n") == 0) //RQT OK - exit and quit
	{
		started = 0;
		letter = '\0';
		trial = 1;
		memset(guess_word, '\0', word_size);
		memset(word, '\0', word_size);
		printf("ENDED GAME\n");
	}
	else //RQT ERR
	{
		ERR();
	}
	return;
}

/* comando: REV PLID */
void REV()
{
	int size = sprintf(buffer, "REV %s\n", PLID); //compose message

	if(started == 0)
	{
		printf("ERR! No game ongoing\n");
		return;
	}
	
	n = sendto(fdClientUDP, buffer, size, 0, resClientUDP->ai_addr, resClientUDP->ai_addrlen); //send message to GS	
	if(n == -1)
		exit(1);

	memset(buffer, '\0', BUFFERSIZE);

	addrlen = sizeof(addr);

	n = recvfrom(fdClientUDP, buffer, BUFFERSIZE, 0, (struct sockaddr*)&addr, &addrlen); //receive message from GS
	if(n == -1)
	{
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			printf("Connection Failed do REV\n");
			return;
		}
		exit(1);
	}

	/*
		comando: RRV word/status 
	*/
	int nargs = sscanf(buffer, "RRV %s\n", final_word); //divisions of the buffer

	if(nargs != 1) //validation if receive one argument (final_word)
	{
		ERR();
		return;
	}

	if(strcmp(final_word, "ERR") == 0) //RRV ERR
	{
		printf("RRV ERR\n");
	}
	else //RRV OK
	{
		printf("RRV OK. CORRECT WORD -> '%s'.\n", final_word);
	}
	return;
}

/* ........................ TCP FUNCTIONS ........................ */

/* comando: GSB */
void GSB()
{
	int n;
    char *result = malloc(2048*sizeof(char));
	char *ptr;

    fdClientTCP = socket(AF_INET, SOCK_STREAM,0);
    if(fdClientTCP==-1)
	{
		printf("Error creating socket\n");
		exit(1);
	}

	createTCPConnection(fdClientTCP, GSIP, GSport);
    if(setsockopt(fdClientTCP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
	{
		exit(1);
	} 

	int size = sprintf(buffer, "GSB\n"); //compose message
    sendMessageTCP(fdClientTCP, buffer, size);

    memset(buffer, '\0', size);
    memset(result, '\0', 2048);

    int total_read = 0;
    ptr = &buffer[0];

    while(total_read < BUFFERSIZE)
    {
		n = read(fdClientTCP, ptr, BUFFERSIZE - total_read);
        if(n == -1)
		{
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                printf("Connection timedout.\n");
            exit(1);
        }

        ptr += n;
        total_read += n;

        if(total_read == BUFFERSIZE)
		{
            result = strcat(result, buffer);
            total_read = 0;
            ptr = &buffer[0];
        }

        if(*(ptr - 1) == '\n')
		{
            result = strcat(result, buffer);
            total_read = 0;
            ptr = &buffer[0];
			break;
		}

    }

    //RSB status [Fname Fsize DATA]
    command = strtok(result, " ");
    if(strcmp(command, "RSB") == 0)
	{
        status = strtok(NULL, " ");
		if(strcmp(status, "OK") == 0)
		{
			int total_read = 0;
			int n_reads = 0;
			int Fsize_aux;
			
			char *header;
			
			FILE *fp;

			Fname = strtok(NULL, " ");
			Fsize = strtok(NULL, " \n");
			Fsize_aux = atoi(Fsize);
			
			header = strtok(NULL, "\n");
			total_read += strlen(header);
			printf("%s\n", header);
			
			header = strtok(NULL, "\n");
			total_read += strlen(header);
			printf("%s\n", header);

			fp = fopen(Fname, "w");
			if(fp == NULL)
			{
				printf("Error opening file\n");
				exit(1);
			}

			while(Fsize_aux > total_read && n_reads < 10)
			{
				header = strtok(NULL, "\n");
				total_read += strlen(header);
				fwrite(header, 1, strlen(header), fp);
				fwrite("\n", 1, 1, fp);
				printf("%s\n", header);
				n_reads++;
			}

			fclose(fp);
    	}	
		else if(strcmp(status, "EMPTY") == 0)
		{
			printf("No files available\n");
		}
		else
		{
			ERR();
		}
	}	

	memset(buffer, '\0', size);
	memset(result, '\0', 2048);
	freeaddrinfo(resClientTCP);
    close(fdClientTCP);
    free(result);
	return;
}

/* comando: GHL PLID */
void GHL()
{
	int n;
	char *ptr;
	int file_read = 0;

    fdClientTCP = socket(AF_INET, SOCK_STREAM,0);
    if(fdClientTCP==-1)
	{
		printf("Error creating socket\n");
		exit(1);
	}

	createTCPConnection(fdClientTCP, GSIP, GSport);
    if(setsockopt(fdClientTCP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
	{
		exit(1);
	} 

	memset(buffer, '\0', 128);	
	
	int size = sprintf(buffer, "GHL %s\n", PLID); //compose message
    sendMessageTCP(fdClientTCP, buffer, size);

    memset(buffer, '\0', 128);

	n = read(fdClientTCP, buffer, BUFFERSIZE);
	if(n == -1)
	{
		if(errno == EAGAIN || errno == EWOULDBLOCK)
			printf("Connection timedout.\n");
		exit(1);
	}

	command = strtok(buffer, " ");
	status = strtok(NULL, " ");

	if(strcmp(command, "RHL") == 0)
	{	
		if(strcmp(status, "OK") == 0)
		{
			int Fsize_aux;
			Fname = strtok(NULL, " ");

			if(Fname == NULL)
			{
				memset(buffer, '\0', 128);
				n = read(fdClientTCP, buffer, BUFFERSIZE);
				if(n == -1)
				{
					if(errno == EAGAIN || errno == EWOULDBLOCK)
						printf("Connection timedout.\n");
					exit(1);
				}
				Fname = strtok(buffer, " ");
				Fsize = strtok(NULL, " ");
				Fsize_aux = atoi(Fsize);
				printf("received hint file: %s ", Fname);
				int size = strlen(Fname) + strlen(Fsize) + 2;
				memccpy(buffer, buffer + size, '\0', n - size);
			}
			else
			{
				Fsize = strtok(NULL, " ");
				Fsize_aux = atoi(Fsize);
				printf("received hint file: %s ", Fname);
				int size = strlen(command) + strlen(status) + strlen(Fname) + strlen(Fsize) + 4;
				memccpy(buffer, buffer + size, '\0', n - size);
			}

			FILE *fp;
			fp = fopen(Fname, "wb");

			fwrite(buffer, 1, sizeof(buffer), fp);
			file_read += n - size;

			while(file_read < Fsize_aux)
			{
				memset(buffer, '\0', BUFFERSIZE);
				n = read(fdClientTCP, buffer, BUFFERSIZE);
				if(n == -1)
				{
					if(errno == EAGAIN || errno == EWOULDBLOCK)
						printf("Connection timedout.\n");
					exit(1);
				}

				if(file_read > Fsize_aux)
				{
					memccpy(buffer, buffer + Fsize_aux - file_read, '\0', n - Fsize_aux + file_read);
					fwrite(buffer, 1, sizeof(buffer), fp);
					file_read += n - Fsize_aux + file_read;
					memset(buffer, '\0', BUFFERSIZE);
					break;
				}
				else
				{
					fwrite(buffer, 1, n, fp);
					file_read += n;
					memset(buffer, '\0', BUFFERSIZE);
				}
			}
			
			printf("(%d bytes)\n", Fsize_aux);
			fclose(fp);
		}	
		else if(strcmp(status, "NOK") == 0)
		{
			printf("No hint file to be sent or something related.\n");
		}
		else
		{
			ERR();
		}
	}
	
	memset(buffer, '\0', sizeof(buffer));
	close(fdClientTCP);
	freeaddrinfo(resClientTCP);
	return;
}

/* comando: STA PLID */
void STA()
{
	int n;
	char *result = malloc(2048*sizeof(char));
	char *ptr;

    fdClientTCP = socket(AF_INET, SOCK_STREAM, 0);
    if(fdClientTCP == -1)
	{
		printf("Error creating socket\n");
		exit(1);
	}

	createTCPConnection(fdClientTCP, GSIP, GSport);
    if(setsockopt(fdClientTCP, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
	{
		exit(1);
	} 

	int size = sprintf(buffer, "STA %s\n", PLID); //compose message
    sendMessageTCP(fdClientTCP, buffer, size);

    memset(buffer, '\0', size);
    memset(result, '\0', 2048);

    int total_read = 0;
    ptr = &buffer[0];

    while(total_read < BUFFERSIZE)
    {
		n = read(fdClientTCP, ptr, BUFFERSIZE - total_read);
        if(n == -1)
		{
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                printf("Connection timedout.\n");
            exit(1);
        }

        ptr += n;
        total_read += n;

        if(total_read == BUFFERSIZE)
		{
            result = strcat(result, buffer);
            total_read = 0;
            ptr = &buffer[0];
        }

        if(*(ptr - 1) == '\n')
		{
            result = strcat(result, buffer);
            total_read = 0;
            ptr = &buffer[0];
			break;
		}

    }

    //STA status [Fname Fsize DATA]
    command = strtok(result, " ");
    if(strcmp(command, "RST") == 0)
	{
        status = strtok(NULL, " ");
		if(strcmp(status, "ACT") == 0)
		{
			int Fsize_aux;
			char *header;
			FILE *fp;

			Fname = strtok(NULL, " ");
			Fsize = strtok(NULL, " \n");
			Fsize_aux = atoi(Fsize);


			total_read += strlen(command) + strlen(status) + strlen(Fname) + strlen(Fsize) + 4; 			
			
			fp = fopen(Fname, "w");
			if(fp == NULL)
			{
				printf("Error opening file\n");
				exit(1);
			}

			while(Fsize_aux > total_read)
			{
				header = strtok(NULL, "\n");
				if(Fsize_aux < (strlen(header) + total_read))
				{
					total_read += strlen(header);
					fwrite(header, 1, strlen(header), fp);
					printf("%s\n", header);
					memset(header, '\0', strlen(header));
					break;
				}
				else
				{
					total_read += strlen(header);
					fwrite(header, 1, strlen(header), fp);
					fwrite("\n", 1, 1, fp);
					printf("%s\n", header);
					memset(header, '\0', strlen(header));
				}
			}

			fclose(fp);
			printf("Displaying information for PLID (%s) ongoing game. File: %s (%d bytes)\n", PLID, Fname, Fsize_aux);
    	}	
		else if(strcmp(status, "FIN") == 0)
		{
			int Fsize_aux;
			char *header;
			FILE *fp;

			Fname = strtok(NULL, " ");
			Fsize = strtok(NULL, " \n");
			Fsize_aux = atoi(Fsize);
			started = 0;

			total_read += strlen(command) + strlen(status) + strlen(Fname) + strlen(Fsize) + 4; 			

			fp = fopen(Fname, "w");
			if(fp == NULL)
			{
				printf("Error opening file\n");
				exit(1);
			}

			while(Fsize_aux > total_read)
			{
				header = strtok(NULL, "\n");
				if(Fsize_aux < (strlen(header) + total_read))
				{
					total_read += strlen(header);
					fwrite(header, 1, strlen(header), fp);
					printf("%s\n", header);
					memset(header, '\0', strlen(header));
					break;
				}
				else
				{
					total_read += strlen(header);
					fwrite(header, 1, strlen(header), fp);
					fwrite("\n", 1, 1, fp);
					printf("%s\n", header);
					memset(header, '\0', strlen(header));
				}
			}

			fclose(fp);
			printf("Displaying information for PLID (%s) last game. File: %s (%d bytes)\n", PLID, Fname, Fsize_aux);
		}
		else if(strcmp(status, "NOK") == 0)
		{
			printf("No information to display (nor active or finished game to PLID (%s)).\n", PLID);
		}
		else
		{
			ERR();
		}
	}
	else
	{
		ERR();
	}

	memset(buffer, '\0', size);
	memset(result, '\0', 2048);
    close(fdClientTCP);
    freeaddrinfo(resClientTCP);
	free(result);
	return;
}

/* ............................ ERROR ............................. */
void ERR()
{
	printf("UNKNOWN ERROR\n");
}

/* ............................. MAIN ............................. */
int main(int argc, char *argv[])
{
	started = 0;
	trial = 1;

	readInput(argc, argv);
	initClientUDP();
	initClientTCP();

	fd_set readfds;

	int counter;
	int final_fd;
	
	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(fdServerUDP, &readfds);
		FD_SET(fdServerTCP, &readfds);
		
		if(fdServerTCP > fdServerUDP)
			final_fd = fdServerTCP;
		else
			final_fd = fdServerUDP;

		counter = select(final_fd + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
		if(counter <= 0)
			exit(1);

		if(FD_ISSET(final_fd, &readfds))
		{
			fgets(buffer, BUFFERSIZE, stdin);
			sscanf(buffer, "%s", buffer_aux);

			if(strcmp(buffer_aux, "start") == 0 || strcmp(buffer_aux, "sg") == 0) //START - UDP
				SNG();
			else if(strcmp(buffer_aux, "play") == 0 || strcmp(buffer_aux, "pl") == 0) //PLAY - UDP
				PLG();
			else if(strcmp(buffer_aux, "guess") == 0 || strcmp(buffer_aux, "gw") == 0) //GUESS - UDP
				PWG();
			else if(strcmp(buffer_aux, "quit") == 0) //QUIT - UDP
				QUT();
			else if(strcmp(buffer_aux, "exit") == 0) //EXIT - UDP
			{
				QUT();
				break;
			} 
			else if(strcmp(buffer_aux, "rev") == 0) //REV - UDP
				REV();
			else if(strcmp(buffer_aux, "scoreboard") == 0 || strcmp(buffer_aux, "sb") == 0) //GSB - TCP
				GSB();
			else if(strcmp(buffer_aux, "hint") == 0 || strcmp(buffer_aux, "h") == 0) //GHL - TCP
				GHL();
			else if(strcmp(buffer_aux, "state") == 0 || strcmp(buffer_aux, "st") == 0) //STA - TCP
				STA();
			else
				ERR();
		}
	}

	freeAll();
	exit(0);
}

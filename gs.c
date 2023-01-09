//1.º Projeto de Redes de Computadores - SERVER.C
//Leonor Marques - 99262
//Pedro Rodrigues - 99300
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define GN 32 					//numero do grupo 
#define	_GSport	58000			//GS port
#define BUFFERSIZE 128			 
#define BUFFERSIZE_AUX 31  
#define MAX_PLID_LEN 7

void readInput(int argc, char* argv[]);
void initServerUDP();
void initServerTCP();

int verbose; //verbose mode
int trial, correct_trials;
int fdServerUDP, fdClientUDP;
int fdServerTCP, fdClientTCP;
int errcode, total_reads;
bool started;

ssize_t n;
socklen_t addrlen;
struct addrinfo hintsServerUDP, *resServerUDP;
struct addrinfo hintsServerTCP, *resServerTCP;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char *status, *command;
char *GSIP, *GSport;
char *Fname, *Fsize; 
char *word_file;
char *class;
int fd;
int palavra_escolhida = 0;
char buffer[BUFFERSIZE];
char buffer_aux[BUFFERSIZE];
char PLID[BUFFERSIZE_AUX];

char word[BUFFERSIZE_AUX];
char final_word[BUFFERSIZE_AUX];
char guess_word[BUFFERSIZE_AUX];

char letter;

struct timeval tv;
extern int errno;

typedef struct player
{
    bool started;
    char word[BUFFERSIZE_AUX];
	char guess_word[BUFFERSIZE_AUX];
    int word_size;
	int errors;
    int max_errors;
    int trials;
	char* class;
} player;

struct player GAMES[1000000]; //array de todos os jogos em curso em que o indice é o PLID

int validPLID(char *PLID);
int isLetter(char c);
int isWord(char *guess_word);
char* malloc_str(char* source);
char* safe_string(char* source);
int max_errors(int word_size);

void checkDirGAMES(int PLID);
void checkDirSCORES();
void createGame(int PLID, char* word_file);
void renameGame(int PLID, char code);
void createGameScore(int PLID);

void readInput(int argc, char* argv[]);
void initServerUDP();
void initServerTCP();
void connectionTCP(int fdServerTCP);
void sendMessageTCP(int fdServerTCP, char* message, ssize_t size);

/* UDP */
void RSG();
void RLG();
void RWG();
void RQT();
void RRV();

/* TCP */
void RSB();
void RHL();
void RST();

/* ---------------------------------------------------------------------- */
/*                              AUXILIARES                                */
/* ---------------------------------------------------------------------- */

int validPLID(char *PLID)
{
	int i;
	if(strlen(PLID) != 6) //numero de estudante do IST com 6 digitos
		return 0;
	for(i = 0; i < 6; i++)
		if(PLID[i] < '0' || PLID[i] > '9')
			return 0;
	return 1;
}

int isLetter(char c)
{
	if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return 1;
	else
		return 0;
}

int isWord(char *guess_word)
{
	int i;
	for (i = 0; i < strlen(guess_word); i++)
	{
		if (guess_word[i] < 'a' || guess_word[i] > 'z' || guess_word[i] < 'A' || guess_word[i] > 'Z')
			return 0;
	}
	return 1;
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
char* safe_string(char* source) //malloc para strings do IP e da PORT
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

int max_errors(int word_size)
{
    int max_errors;

    if ((word_size <= 6) && (word_size >=3)){
        max_errors = 7;
    }
    else if ((word_size >= 7) && (word_size <= 10)){
        max_errors = 8;
    }
    else if ((word_size >= 11)){
        max_errors = 9;
    }
    else
    {
        printf("max_errors(int word_size) -> error;\n");
    }
    return max_errors;
}

void getGSIP()
{
	struct addrinfo hints, *res, *res_aux;
	struct in_addr *addr_aux;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	memset(buffer, '\0' , BUFFERSIZE);
	if(gethostname(buffer, BUFFERSIZE)==-1)
		exit(1);

	errcode = getaddrinfo(buffer, NULL, &hints, &res);
	if(errcode != 0)
		exit(1);

	memset(buffer, '\0' , BUFFERSIZE);
	res_aux = res;
	addr_aux = &((struct sockaddr_in *)res_aux->ai_addr)->sin_addr;
	inet_ntop(res_aux->ai_family, addr_aux, buffer, BUFFERSIZE);

	GSIP = malloc((strlen(buffer) + 1) * sizeof(char));
	strcpy(GSIP, buffer);

	freeaddrinfo(res);
}

/*
int FindLastGame(char *PLID , char *fname)
{
	struct dirent **filelist;
	int n_entries, found;
	char dirname[20];

	sprintf(dirname, "GAMES/%s/", PLID);

	n_entries = scandir(dirname, &filelist, 0, alphasort);
	found = 0;

	if (n_entries <= 0)
		return (0) ;
	else
	{
		while (n_entries--)
		{
			if (filelist[n_entries]->d_name[0] != ' . ')
			{
				sprintf (fname, "GAMES/%s/%s", PLID, filelist[n_entries]->d_name);
				found = 1;
			}
			free (filelist[n_entries]);
			if (found)
				break;
		}
		free(filelist);		
	}
	return (found);
}

int FindTopScores(SCORELIST *list)
{
	struct dirent **filelist;
	int n_entries, i_file;
	char fname[50];
	FILE *fp;

	n_entries = scandir("SCORES/", &filelist, 0, alphasort);
	i_file = 0;

	if (n_entries <= 0)
		return (0) ;
	else
	{
		while (n_entries--)
		{
			if (filelist[n_entries]->d_name[0] != ' . ')
			{
				sprintf(fname, "SCORES/%s", filelist[n_entries]->d_name);
				fp = fopen(fname, ”r”);
				if (fp != NULL)
				{
					fscanf(fp, "%d %s %s %d %d", &list->score[i_file], list->PLID[i_file], list->word[i_file], &list->n_succ[i_file], &list->n_tot[i_file]);
					fclose(fp);
					i_file++;
				}
			}
			free (filelist[n_entries]);
			if (i_file == 10)
				break;
		}
		free(filelist);		
	}
	list->n_scores = i_file;
	return (i_file);
} */

void checkDirGAMES(int PLID)
{
	char dirname[20];
	sprintf(dirname, "GAMES/%d", PLID);
	DIR *dir = opendir(dirname);
	if (dir)
	{
		closedir(dir);
	}
	else if (ENOENT == errno)
	{
		mkdir(dirname, 0777);
	}
}

void createGame(int PLID, char* word)
{
	char dirname[22];
	char *header;
	int ret = 0;
	FILE *fp;

	sprintf(dirname, "GAMES/GAME_%d.txt", PLID);
	fp = fopen(dirname, "w"); 
	if(fp == NULL)
	{
		return;
	}

	int size_header = strlen(word) + strlen(GAMES[PLID].class) + 4;
	char data[size_header];
	memset(data, '\0', size_header);
	strcat(data, word);
	strcat(data, " ");
	strcat(data, GAMES[PLID].class);
	strcat(data, "\n");
	fprintf(fp, "%s", data);
	fclose(fp);
}

void renameGame(int PLID, char code)
{
	char dirname_entry[22];
	char dirname_final[40];
	char time_aux[20];

	sprintf(dirname_entry, "GAMES/GAME_%d.txt", PLID);
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	strftime(time_aux, sizeof(time_aux), "%Y%m%d_%H%M%S", tm);
	sprintf(dirname_final, "GAMES/%d/%s_%c.txt", PLID, time_aux, code);
	checkDirGAMES(PLID);
	rename(dirname_entry, dirname_final);
}

void checkDirSCORES()
{
	char dirname[20];
	sprintf(dirname, "SCORES");
	DIR *dir = opendir(dirname);
	if(dir)
	{
		closedir(dir);
	}
	else if (ENOENT == errno)
	{
		mkdir(dirname, 0777);
	}
}

void createGameScore(int PLID)
{
	checkDirSCORES();
	char data[128];
	char dirname[40];
	char time_aux[20];
	char score[4];
	char bufferPLID[7];
	char bufferTrials[3];
	char bufferN_Succ[3];
	char bufferN_Trials[3];  
	char *header;
	int ret = 0;
	int n_succ_aux;
	int score_aux;
	FILE *fp;
	
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	strftime(time_aux, sizeof(time_aux), "%d%m%Y_%H%M%S", tm);

	n_succ_aux = GAMES[PLID].trials - GAMES[PLID].errors;
	score_aux = ((float)(GAMES[PLID].trials - GAMES[PLID].errors) / (float)(GAMES[PLID].trials)) * 100;

	sprintf(score, "%03d", score_aux);
	memset(dirname, '\0', 40);	

	sprintf(dirname, "SCORES/%s_%d_%s.txt", score, PLID, time_aux);
	fp = fopen(dirname, "w"); 
	if(fp == NULL)
	{
		return;
	}

	memset(data, '\0', sizeof(data));
	strcat(data, score);
	strcat(data, " ");
	sprintf(bufferPLID, "%d", PLID);
	strcat(data, bufferPLID);
	strcat(data, " ");
	strcat(data, GAMES[PLID].word);
	strcat(data, " ");
	sprintf(bufferN_Succ, "%d", n_succ_aux);
	strcat(data, bufferN_Succ);
	strcat(data, " ");
	sprintf(bufferTrials, "%d", GAMES[PLID].trials);
	strcat(data, bufferTrials);
	strcat(data, "\n");
	fprintf(fp, "%s", data);
	fclose(fp);
}

/* ---------------------------------------------------------------------- */
/*                           FIM DAS AUXILIARES                           */
/* ---------------------------------------------------------------------- */

/* Lê input -> ./GS word_file [-p GSport] [-v] */
void readInput(int argc, char* argv[])
{	
	int i = 2;

    if(argc < 2 || argc > 4)
		exit(1);

	verbose = 0;

    word_file = safe_string(argv[1]);

	while(i < argc)
	{
		if(strcmp(argv[i],"-p") == 0) //GSport
		{
			GSport = malloc(6 * sizeof(char));
			strcpy(GSport, argv[i + 1]);
			i += 2;
		}
		else if(strcmp(argv[i],"-v") == 0) //verbose
		{
			verbose = 1;
			i++;
		}
		else
        {
            exit(1);
        }
	}

	getGSIP();

	//GSport
	if(!GSport)
	{
		GSport = malloc(6 * sizeof(char));
		sprintf(GSport,"%d", _GSport + GN);
	}
}

/* Ligacao UDP ao cliente */
void initServerUDP()
{
	fdServerUDP = socket(AF_INET, SOCK_DGRAM, 0);
	if(fdServerUDP == -1)
    {
        printf("Error creating socket!\n");
        exit(1);    
    } 

	memset(&hintsServerUDP, 0, sizeof hintsServerUDP);
	hintsServerUDP.ai_family = AF_INET; 			    // IPv4
	hintsServerUDP.ai_socktype = SOCK_DGRAM; 	        //UDP socket
	hintsServerUDP.ai_flags = AI_PASSIVE;
	
	errcode = getaddrinfo(NULL, GSport, &hintsServerUDP, &resServerUDP);
	if(errcode != 0) //ERROR
	{
        exit(1);
    }

	n = bind(fdServerUDP, resServerUDP->ai_addr, resServerUDP->ai_addrlen);
	if(n == -1) //ERROR
	{
        exit(1);
    }	
}

/* Ligacao TCP ao cliente Player */
void initServerTCP()
{
	fdServerTCP = socket(AF_INET,SOCK_STREAM,0);
	if (fdServerTCP == -1)
		exit(1);

	memset(&hintsServerTCP,0,sizeof hintsServerTCP);
	hintsServerTCP.ai_family = AF_INET; //TCP socket
	hintsServerTCP.ai_socktype = SOCK_STREAM; //IPv4
	hintsServerTCP.ai_flags = AI_PASSIVE; //TCP socket
	
	errcode=getaddrinfo(NULL, GSport, &hintsServerTCP, &resServerTCP);
	if(errcode != 0)
		exit(1);

	n = bind(fdServerTCP, resServerTCP->ai_addr, resServerTCP->ai_addrlen);
	if(n == -1)
		exit(1);

	if(listen(fdServerTCP,5) == -1)
		exit(1);
}

/* quando realizamos um fork temos de estabelecer a ligação */
void connectionTCP(int fd_aux)
{
	freeaddrinfo(resServerTCP);
	close(fdServerTCP);

	//apos fechar a ligacao anterior, estabelecer uma nova
	fdServerTCP = fd_aux;
	
	int total_n;
	char *ptr;
	while(1)
	{
		memset(buffer, '\0', BUFFERSIZE);
		total_n = 0;
		ptr = &buffer[0];

		while((n = read(fdServerTCP, ptr, BUFFERSIZE - total_n)) != 0)
		{
			if(n == -1)
				exit(1);

			ptr += n;
			total_n += n;

			if(*(ptr - 1) == '\n')
				break;
		}

		if(n == 0)
			break;

		sscanf(buffer, "%s", buffer_aux);

		if(strcmp(buffer_aux, "GSB") == 0)
			RSB(fdServerTCP);
		else if(strcmp(buffer_aux, "GHL") == 0)
			RHL(fdServerTCP);
		else if(strcmp(buffer_aux, "STA") == 0)
			RST(fdServerTCP);
		else
			sendMessageTCP(fdServerTCP, "ERR\n", 4);
	}

	free(GSport);
	close(fdServerTCP);
	exit(0);
}

/* envia mensagem ao cliente */
void sendMessageTCP(int fdServerTCP, char* message, ssize_t size)
{
	int i;
	char *ptr;

	for(i = size, ptr = &message[0]; i > 0; i -= n, ptr += n)
	{
		if((n = write(fdServerTCP, ptr, i)) <= 0)
		{
			printf("Error sending message TCP!\n");
			exit(1);
		}	
	}
}

/* comando: RSG status [n_letters max_errors] */
void RSG()
{
	char bufPLID[MAX_PLID_LEN];

	int nargs = sscanf(buffer, "SNG %s\n", bufPLID);

	if(nargs != 1 || validPLID(bufPLID) != 1)
	{
		n = sendto(fdServerUDP, "ERR\n", 4, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	}

    int PLID = atoi(bufPLID);
	
    if(GAMES[PLID].started == false) //verifica se o PLID ja tem um jogo a decorrer
    {
        /* escolher a palavra do ficheiro txt */
        FILE *fp;
        fp = fopen(word_file, "r");
        if (fp == NULL)
        {
            printf("Error opening file to choose one word!\n");
            exit(1);
        }

        int i = 0;
		char line[256];

        while (fgets(line, sizeof(line), fp) != NULL)
        {
            i++;
        }
		fclose(fp);

		//se fosse a palavra aleatoria seria assim
        //int random = rand() % i;

		fp = fopen(word_file, "r");
        if (fp == NULL)
        {
            printf("Error opening file to choose one word!\n");
            exit(1);
        }

		i = 0;
        while (fgets(line, sizeof(line), fp) != NULL)
        {
            if (palavra_escolhida == i)
            {	
				line[strlen(line)-1] = '\0';
				sscanf(line,"%s", GAMES[PLID].word);

				GAMES[PLID].class = malloc(strlen(line) - strlen(word));

				sscanf(line, "%s %s", GAMES[PLID].word, GAMES[PLID].class);
				palavra_escolhida++;
				break;
			}
			i++;
		}

		fclose(fp);
        
		GAMES[PLID].started = true;
		memset(GAMES[PLID].guess_word, ' ', strlen(GAMES[PLID].word));
        GAMES[PLID].word_size = strlen(GAMES[PLID].word);
        GAMES[PLID].errors = 0;
		GAMES[PLID].max_errors = max_errors(GAMES[PLID].word_size);
        GAMES[PLID].trials = 1;
		createGame(PLID, GAMES[PLID].word);

        int size = sprintf(buffer, "RSG OK %d %d\n", GAMES[PLID].word_size, GAMES[PLID].max_errors); //compor a mensagem
        
        n = sendto(fdServerUDP, buffer, size, 0, (struct sockaddr*)&addr, addrlen); 
        if(n == -1)
            exit(1);

		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: new game; word = \"%s\" (%d letters)\n", PLID, GAMES[PLID].word, GAMES[PLID].word_size);
		}
		return;
    }
    else if (GAMES[PLID].started == true) //verify if PLID has any game ongoing
    {
        int size = sprintf(buffer, "RSG NOK\n"); //compor a mensagem

        n = sendto(fdServerUDP, buffer, size, 0, (struct sockaddr*)&addr, addrlen); 
        if(n == -1)
            exit(1);
        return;
    }
    else 
    {
		n = sendto(fdServerUDP, "ERR\n", 4, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	}
}

/* comando: RLG status trial [n pos*] */
void RLG()
{
	char bufPLID[MAX_PLID_LEN];

	int nargs = sscanf(buffer, "PLG %s %c %d\n", bufPLID, &letter, &trial);

	int PLID = atoi(bufPLID);

	if(nargs != 3 || !validPLID(bufPLID) || isLetter(letter) != 1)
	{
		n = sendto(fdServerUDP, "ERR\n", 4, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	}

	int len = strlen(GAMES[PLID].word);
	int correct_positions = 0;
	int duplicate = 0;
	char positions[4];
	int size;

	for (int i = 0; i < len; i++)
	{
		if(GAMES[PLID].guess_word[i] == letter) //caso DUP
		{
			duplicate = 1;
			break;
		}

		if(GAMES[PLID].word[i] == letter) 
		{
			GAMES[PLID].guess_word[i] = letter;
			correct_positions++;
		}	
	}

	if(!validPLID(bufPLID) || GAMES[PLID].started == false) //RLG -> status ERR
	{
		size = sprintf(buffer, "RLG ERR %d", trial); //compor a mensagem
		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("Invalid PLID or no ongoing game for this PLID: %d.\n", PLID);		
		}
	}
	else if(trial != GAMES[PLID].trials || trial == (GAMES[PLID].trials - 1)) //RLG -> status INV
	{
		size = sprintf(buffer, "RLG INV"); //compor a mensagem
		if(verbose ==1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("Invalid trial number... Server -> %d ... Client -> %d\n", GAMES[PLID].trials, trial);	
		}
	}
	else if(correct_positions > 0 && strcmp(GAMES[PLID].word, GAMES[PLID].guess_word) != 0 && duplicate == 0) //RLG -> status OK
	{
		size = sprintf(buffer, "RLG OK %d %d", trial, correct_positions); //compor a mensagem

		for(int i = 0; i < len; i++)
		{	
			if (GAMES[PLID].word[i] == letter)
			{
				size += sprintf(positions, " %d", i+1);
				strcat(buffer, positions);
			}
		}
		strcat(buffer, "\n");
		size += 2;
		GAMES[PLID].trials++;

		char dirname[22];
		sprintf(dirname, "GAMES/GAME_%d.txt", PLID);

		FILE *fp = fopen(dirname, "a+");
		if(fp == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}

		fprintf(fp, "T %c\n", letter);

		fclose(fp);

		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: play letter \"%c\" - %d hits; word not guessed\n", PLID, letter, correct_positions);
		}
	}
	else if(correct_positions > 0 && strcmp(GAMES[PLID].word, GAMES[PLID].guess_word) == 0 && duplicate == 0) //RLG -> status WIN
	{
		size = sprintf(buffer, "RLG WIN %d %d", trial, correct_positions); //compor a mensagem

		for(int i = 0; i < len; i++)
		{
			if (GAMES[PLID].word[i] == letter)
			{
				size += sprintf(positions, " %d", i+1);
				strcat(buffer, positions);
			}
		}
		strcat(buffer, "\n");
		size += 2;
		GAMES[PLID].started = false;
		GAMES[PLID].trials++;

		char dirname[22];
		sprintf(dirname, "GAMES/GAME_%d.txt", PLID);

		FILE *fp = fopen(dirname, "a+");
		if(fp == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}

		fprintf(fp, "T %c\n", letter);

		fclose(fp);

		renameGame(PLID, 'W');
		createGameScore(PLID);

		if(GAMES[PLID].started == true)
		{
			GAMES[PLID].started = false;
			GAMES[PLID].trials = 1;
			GAMES[PLID].errors = 0;
		}

		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: play letter \"%c\" - %d hits; word guessed\n", PLID, letter, correct_positions);
		}
	} 
	else if(correct_positions == 0 && duplicate	== 0 && GAMES[PLID].errors == GAMES[PLID].max_errors) //RLG -> status OVR
	{
		size = sprintf(buffer, "RLG OVR %d\n", trial); //compor a mensagem
		GAMES[PLID].started = false;
		GAMES[PLID].trials++;
		
		char dirname[22];
		sprintf(dirname, "GAMES/GAME_%d.txt", PLID);

		FILE *fp = fopen(dirname, "a+");
		if(fp == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}

		fprintf(fp, "T %c\n", letter);

		fclose(fp);

		renameGame(PLID, 'F');

		if(GAMES[PLID].started == true)
		{
			GAMES[PLID].started = false;
			GAMES[PLID].trials = 1;
			GAMES[PLID].errors = 0;
		}

		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: play letter \"%c\" - 0 hits; 0 trials available\n", PLID, letter);
		}
			
	}
	else if(correct_positions == 0 && duplicate == 0 && GAMES[PLID].errors < GAMES[PLID].max_errors) //RLG -> status NOK 
	{
		size = sprintf(buffer, "RLG NOK %d\n", trial); //compor a mensagem
		GAMES[PLID].trials++;
		GAMES[PLID].errors++;

		char dirname[22];
		sprintf(dirname, "GAMES/GAME_%d.txt", PLID);

		FILE *fp = fopen(dirname, "a+");
		if(fp == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}

		fprintf(fp, "T %c\n", letter);

		fclose(fp);

		int aux_errors = GAMES[PLID].max_errors - GAMES[PLID].errors;
		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: play letter \"%c\" - 0 hits; %d trials available\n", PLID, letter, aux_errors);
		}
	}
	else if(correct_positions == 0 && duplicate == 1 && GAMES[PLID].errors < GAMES[PLID].max_errors) //RLG -> status DUP
	{
		size = sprintf(buffer, "RLG DUP %d\n", trial); //compor a mensagem
		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: play letter \"%c\" - duplicated letter\n", PLID, letter);
		}		
	}
	else
	{
		n = sendto(fdServerUDP, "ERR\n", 5, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	} 

	n = sendto(fdServerUDP, buffer, size, 0, (struct sockaddr*)&addr, addrlen);
	return;
}

/* comando: RWG status trials */
void RWG()
{
	char bufPLID[MAX_PLID_LEN];
	int nargs = sscanf(buffer, "PWG %s %s %d\n", bufPLID, word, &trial);
	int PLID = atoi(bufPLID);
	int size;

	if(nargs != 3 || !validPLID(bufPLID))
	{
		n = sendto(fdServerUDP, "ERR\n", 5, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	}

	if(!validPLID(bufPLID) || GAMES[PLID].started == false) //RWG -> status ERR
	{
		size = sprintf(buffer, "RLG ERR\n"); //compor a mensagem
		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("Invalid PLID or no ongoing game for this PLID: %d.\n", PLID);
		}
			
	}
	else if(strcmp(GAMES[PLID].word, word) == 0) //RWG -> status WIN
	{
		//GAMES[PLID].started = false;
		size = sprintf(buffer, "RWG WIN %d\n", trial); //compor a mensagem

		char dirname[22];
		sprintf(dirname, "GAMES/GAME_%d.txt", PLID);

		FILE *fp = fopen(dirname, "a+");
		if(fp == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}

		fprintf(fp, "G %s\n", word);

		fclose(fp);
		renameGame(PLID, 'W');
		createGameScore(PLID);

		if(GAMES[PLID].started == true)
		{
			GAMES[PLID].started = false;
			GAMES[PLID].trials = 1;
			GAMES[PLID].errors = 0;
		}

		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: guess word \"%s\" - WIN (game ended)\n", PLID, word);
		}
			
	}
	else if(trial > GAMES[PLID].trials || trial == (GAMES[PLID].trials - 1)) //RWG -> status INV
	{
		size = sprintf(buffer, "RWG INV\n"); //compor a mensagem
		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("Invalid trial number... Server -> %d ... Client -> %d\n", GAMES[PLID].trials, trial);	
		}
			
	}
	else if(strcmp(GAMES[PLID].word, word) != 0 && GAMES[PLID].errors < GAMES[PLID].max_errors)
	{
		size = sprintf(buffer, "RWG NOK %d\n", trial); //compor a mensagem
		GAMES[PLID].trials++;
		GAMES[PLID].errors++;

		char dirname[22];
		sprintf(dirname, "GAMES/GAME_%d.txt", PLID);

		FILE *fp = fopen(dirname, "a+");
		if(fp == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}

		fprintf(fp, "G %s\n", word);

		fclose(fp);

		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: guess word \"%s\" - NOK\n", PLID, word);
		}
	}
	else if(strcmp(GAMES[PLID].word, word) != 0 && GAMES[PLID].errors == GAMES[PLID].max_errors)
	{
		size = sprintf(buffer, "RWG OVR %d\n", trial); //compor a mensagem

		char dirname[22];
		sprintf(dirname, "GAMES/GAME_%d.txt", PLID);

		FILE *fp = fopen(dirname, "a+");
		if(fp == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}

		fprintf(fp, "G %s\n", word);

		fclose(fp);
		renameGame(PLID, 'F');

		if(GAMES[PLID].started == true)
		{
			GAMES[PLID].started = false;
			GAMES[PLID].trials = 1;
			GAMES[PLID].errors = 0;
		}
		
		if(verbose == 1)
		{
			printf("Address IP is %s and Port = %d\n", inet_ntoa(addr.sin_addr), _GSport + GN);
			printf("PLID=%d: guess word \"%s\" - OVR (game ended)\n", PLID, word);
		}
	}
	else //RWG -> status ERR
	{
		n = sendto(fdServerUDP, "ERR\n", 5, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	}
	n = sendto(fdServerUDP, buffer, size, 0, (struct sockaddr*)&addr, addrlen);
	return;
}

/* comando: RQT status */
void RQT()
{
	char bufPLID[MAX_PLID_LEN];

	int nargs = sscanf(buffer, "QUT %s\n", bufPLID);
	
	if(nargs != 1 ||validPLID(bufPLID) != 1)
	{
		n = sendto(fdServerUDP, "ERR\n", 5, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	}

    int PLID = atoi(bufPLID);
	
	int size;
	if(GAMES[PLID].started == true)
	{
		size = sprintf(buffer, "RQT OK\n");
		GAMES[PLID].started = false;
		GAMES[PLID].trials = 1;
		GAMES[PLID].errors = 0;
		renameGame(PLID, 'Q');
	}
	else
	{
		size = sprintf(buffer, "RQT ERR\n");
	}
	
	n = sendto(fdServerUDP, buffer, size, 0, (struct sockaddr*)&addr, addrlen);
	return;
}

/* comando: RRV word/status */

void RRV()
{
	char bufPLID[MAX_PLID_LEN];

	int nargs = sscanf(buffer, "REV %s\n", bufPLID);

	if(nargs != 1 || validPLID(bufPLID) != 1)
	{
		n = sendto(fdServerUDP, "ERR\n", 4, 0, (struct sockaddr*)&addr, addrlen); //ERR enviado ao cliente
		if(n == -1)
			exit(1);
		return;
	}

    int PLID = atoi(bufPLID);

	int size;
	
	if(GAMES[PLID].started == true)
	{
		size = sprintf(buffer, "RRV %s\n", GAMES[PLID].word);
	}
	else
	{
		size = sprintf(buffer, "RRV ERR\n");
	}
	
	n = sendto(fdServerUDP, buffer, size, 0, (struct sockaddr*)&addr, addrlen);
	return;
}

/* comando: RSB status [Fname Fsize Fdata] */
void RSB()
{
	int size = sprintf(buffer, "RSB OK\n"); //compor a mensagem
	
	sendMessageTCP(fdServerTCP, buffer, size);
	
	if(verbose == 1)
		printf("GSB received and RSB had been sent!\n");
	return;
}

/* comando: RHL status [Fname Fsize Fdata] */
void RHL()
{
	int size = sprintf(buffer, "RHL OK\n"); //compor a mensagem
	
	sendMessageTCP(fdServerTCP, buffer, size);
	
	if(verbose == 1)
		printf("GHL received and RHL had been sent!\n");
	return;
}

/* comando: RST status [Fname Fsize Fdata] */
void RST()
{
	int size = sprintf(buffer, "RST OK\n"); //compor a mensagem
	
	sendMessageTCP(fdServerTCP, buffer, size);
	
	if(verbose == 1)
		printf("STA received and RST had been sent!\n");
	return;
}


/* MAIN */
int main(int argc, char *argv[])
{
    readInput(argc, argv); 

    initServerUDP();
	initServerTCP();
    fd_set readfds;

	int finalfd, counter;
	int fd, ret;
    
    for (int i = 1; i < 1000000; i++)
    {
        GAMES[i].started = false;
    }

	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(fdServerUDP, &readfds);
		FD_SET(fdServerTCP, &readfds);
		
		if(fdServerTCP > fdServerUDP)
		{
			finalfd = fdServerTCP;
		}
		else
		{
			finalfd = fdServerUDP;
		}

		counter = select(finalfd + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);
		if(counter <= 0)
			exit(1);

		if(FD_ISSET(fdServerUDP, &readfds)) //ligacao UDP com o Player
		{
			memset(buffer, '\0', BUFFERSIZE);

			addrlen=sizeof(addr);
			n = recvfrom(fdServerUDP, buffer, BUFFERSIZE, 0, (struct sockaddr*)&addr,&addrlen);
			if(n == -1)
				exit(1);

			sscanf(buffer, "%s", buffer_aux);

			if(strcmp(buffer_aux, "SNG") == 0) //RSG
				RSG();
			else if(strcmp(buffer_aux, "PLG") == 0) //RLG
				RLG();
			else if(strcmp(buffer_aux, "PWG") == 0) //RWG
				RWG();
			else if(strcmp(buffer_aux, "QUT") == 0) //RQT  
				RQT();
			else if(strcmp(buffer_aux, "REV") == 0) //RRV
				RRV();
			else
			{
				n = sendto(fdServerUDP, "ERR\n", 4, 0, (struct sockaddr*)&addr, addrlen);
				if(n == -1)
					exit(1);
			}
        }
		else if(FD_ISSET(fdServerTCP, &readfds)) //ligacao TCP com o Player
		{
			int fd_aux, pid;
			addrlen = sizeof(addr);
			if((fd_aux = accept(fdServerTCP, (struct sockaddr*)&addr, &addrlen)) == -1)
				exit(1);

			if((pid = fork()) == -1)
			{
				printf("Error in fork();\n");
				exit(1);
			}
			else if(pid == 0)
				connectionTCP(fd_aux);

			ret = close(fd_aux);
			if(ret == -1)
			{
				printf("Error in close();\n");
				exit(1);
			}
		}
    }  
    exit(0);
}


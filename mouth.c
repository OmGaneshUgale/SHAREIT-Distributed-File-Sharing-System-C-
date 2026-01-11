#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define BUFFLEN 1024
#define receivr 3
#define MAX 64

int sockfd;
unsigned char BUFFER[BUFFLEN];
char filename[MAX];
struct sockaddr_in whrtosendAddr;
FILE *fp;

void
makefilename (char port[])
{
  filename[0] = '\0';
  strcat (filename, port);
  strcat (filename, "btm.txt");
}

void
switchfile ()
{
  fp = fopen (filename, "rb");
  if (!fp)
    {
      perror ("Error while Opening file");
      exit (EXIT_FAILURE);
    }
}

void
switchsocket ()
{
  sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      perror ("Socket Failed");
      exit (EXIT_FAILURE);
    }
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      perror ("System not specified");
      exit (EXIT_FAILURE);
    }

  makefilename (argv[1]);
  switchfile ();
  switchsocket ();

  int i;
  while (1)
    {
      i = fread (BUFFER, 1, BUFFLEN, fp);
      if (i == BUFFLEN)
	{
	  short prt;
	  memcpy (&prt, &BUFFER[receivr], sizeof (prt));
	  int port = (int) prt;

	  memset (&whrtosendAddr, 0, sizeof (whrtosendAddr));
	  whrtosendAddr.sin_family = AF_INET;
	  whrtosendAddr.sin_port = htons (port);
	  whrtosendAddr.sin_addr.s_addr = INADDR_ANY;
	  sendto (sockfd, (const char *) BUFFER, BUFFLEN, 0,
		  (const struct sockaddr *) &whrtosendAddr,
		  sizeof (whrtosendAddr));
	  memset (&whrtosendAddr, 0, sizeof (whrtosendAddr));
	}
      else
	{
	  clearerr (fp);
	  usleep (1000);
	}
    }

  close (sockfd);
}

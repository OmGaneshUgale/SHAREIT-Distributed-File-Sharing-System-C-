#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define BUFFLEN 1024
#define MAX 64

int LISTENPORT;
int sockfd, len, n;
unsigned char msg[BUFFLEN];
struct sockaddr_in earAddr, whritcomeAddr;
FILE *fp;
char filename[MAX];

void
makefilename (char port[])
{
  filename[0] = '\0';
  strcat (filename, port);
  strcat (filename, "etb.txt");
}

void
setsocket ()
{
  sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      perror ("Socket System Call Failed");
      exit (EXIT_FAILURE);
    }

  memset (&earAddr, 0, sizeof (earAddr));
  memset (&whritcomeAddr, 0, sizeof (whritcomeAddr));

  earAddr.sin_family = AF_INET;
  earAddr.sin_port = htons (LISTENPORT);
  earAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind (sockfd, (const struct sockaddr *) &earAddr, sizeof (earAddr)) < 0)
    {
      perror ("BIND SYSCALL FAILED");
      exit (EXIT_FAILURE);
    }

  len = sizeof (whritcomeAddr);
}

void
onfile ()
{
  fp = fopen (filename, "ab");
  if (fp == NULL)
    {
      perror ("Error while opening file");
      printf ("%s", filename);
      exit (EXIT_FAILURE);
    }
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      perror ("Receive port not given");
      exit (EXIT_FAILURE);
    }

  LISTENPORT = (int) atoi (argv[1]);
  makefilename (argv[1]);
  setsocket ();
  onfile ();

  while (1)
    {
      n =
	recvfrom (sockfd, (char *) msg, BUFFLEN, MSG_WAITALL,
		  (struct sockaddr *) &whritcomeAddr, &len);
      if (n <= 0)
	continue;
      if (n < BUFFLEN)
	memset (msg + n, 0, BUFFLEN - n);
      fwrite (msg, 1, BUFFLEN, fp);
      fflush (fp);
      memset (&earAddr, 0, sizeof (whritcomeAddr));
    }
  close (sockfd);
  fclose (fp);
}

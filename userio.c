#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#define MAX 64
#define MAXFNAME 256
#define optionsize 0
#define sendr 1
#define receivr 3
#define actmsg 15
#define BUFFLEN 1024

char fnameutb[MAX];
char fnamebtu[MAX];
short sender;

FILE *utb = NULL, *btu = NULL;

int
makefilename (char *port)
{
  fnameutb[0] = '\0';
  fnamebtu[0] = '\0';

  strcat (fnameutb, port);
  strcat (fnamebtu, port);
  strcat (fnameutb, "utb.txt");
  strcat (fnamebtu, "btu.txt");
  return 0;
}

int
onfiles ()
{
  utb = fopen (fnameutb, "ab");
  if (!utb)
    {
      perror ("Error while opening utb file");
      exit (EXIT_FAILURE);
    }

  btu = fopen (fnamebtu, "rb");
  if (!btu)
    {
      btu = fopen (fnamebtu, "wb+");
      if (!btu)
	{
	  perror ("Error while opening btu file");
	  exit (EXIT_FAILURE);
	}
      fclose (btu);
      btu = fopen (fnamebtu, "rb");
    }
  return 0;
}

int
write_frame (unsigned char *buffer)
{
  size_t n = fwrite (buffer, 1, BUFFLEN, utb);
  fflush (utb);
  return (int) n;
}

int
sendoption (unsigned char opt)
{
  unsigned char buffer[BUFFLEN] = { 0 };
  memcpy (&buffer[optionsize], &opt, sizeof (opt));
  write_frame (buffer);
  return 0;
}

int
sendmsg ()
{
  short port;
  char fname[MAXFNAME] = { 0 };

  printf ("Enter the System number where the file to be sent\n");
  scanf ("%hd", &port);

  printf ("Enter the filename to send to %d system\n", port);
  int ch;
  while ((ch = getchar ()) != '\n' && ch != EOF);
  if (!fgets (fname, sizeof (fname), stdin))
    {
      fprintf (stderr, "Failed to read filename\n");
      return 0;
    }
  size_t n = strcspn (fname, "\n");
  fname[n] = '\0';

  unsigned char buffer[BUFFLEN] = { 0 };
  unsigned char option = 1;

  memcpy (&buffer[optionsize], &option, sizeof (option));
  memcpy (&buffer[sendr], &sender, sizeof (sender));
  memcpy (&buffer[receivr], &port, sizeof (port));
  // Put filename (string with '\0') in payload start
  memcpy (&buffer[actmsg], fname, strlen (fname) + 1);

  write_frame (buffer);
  return 0;
}

int
seereceived ()
{
  sendoption (2);
  usleep (10000);
  unsigned char ch[BUFFLEN];
  int i = fread (ch, 1, BUFFLEN, btu);
  if (i > 0)
    {
      printf ("\n\nReceived message:\n\n%.*s\n\n", i, ch);
      fflush (stdout);
    }
  else
    {
      clearerr (btu);
    }
  return 0;
}

int
seemsg ()
{
  FILE *fp;
  char fname[MAXFNAME] = { 0 };
  printf ("Enter the filename:\n");
  scanf ("%255s", fname);
  fp = fopen (fname, "rb");
  if (!fp)
    {
      printf ("File not found. Showing received table:\n");
      seereceived ();
    }
  else
    {
      unsigned char buf[1024];
      size_t r;
      while ((r = fread (buf, 1, sizeof (buf), fp)) > 0)
	{
	  fwrite (buf, 1, r, stdout);
	}
      printf ("\n");
      fclose (fp);
    }
  return 0;
}

void
displaytrm ()
{
  sendoption (2);
  usleep (10000);
  unsigned char buff[BUFFLEN];
  printf
    ("\n\t\tTill Received Messages Table\nSystem\tMessage no.\tNo. of Parts\tFilename\n\n");
  while (fread (buff, 1, BUFFLEN, btu) == BUFFLEN)
    {
      printf ("\n%.*s\n", BUFFLEN, buff);
    }
  clearerr (btu);
}

void
displayspecifictrm ()
{
  short sys;
  printf ("Enter system number whose messages to be seen:\n");
  scanf ("%hd", &sys);

  unsigned char buffer[BUFFLEN] = { 0 };
  unsigned char option = 3;
  memcpy (&buffer[optionsize], &option, sizeof (option));
  memcpy (&buffer[sendr], &sys, sizeof (sys));
  write_frame (buffer);
  usleep (10000);

  unsigned char buff[BUFFLEN];
  printf
    ("\n\t\tTill Received Messages from %d system\nSystem\tMessage no.\tNo. of Parts\tFilename\n\n",
     sys);
  while (fread (buff, 1, BUFFLEN, btu) == BUFFLEN)
    {
      printf ("\n%.*s\n", BUFFLEN, buff);
    }
  clearerr (btu);
}

void
displaytsm ()
{
  sendoption (5);
  usleep (10000);
  unsigned char buff[BUFFLEN];
  printf
    ("\n\t\tTill Send Messages Table\nSystem\tMessage no.\tNo. of Parts\tFilename\n\n");
  while (fread (buff, 1, BUFFLEN, btu) == BUFFLEN)
    {
      printf ("\n%.*s\n", BUFFLEN, buff);
    }
  clearerr (btu);
}

void
displayosmt ()
{
  sendoption (6);
  usleep (10000);
  unsigned char buff[BUFFLEN];
  printf
    ("\n\t\tOngoing Sending Messages Table\nSystem\tMessage no.\tNo. of Parts\tFilename\n\n");
  while (fread (buff, 1, BUFFLEN, btu) == BUFFLEN)
    {
      printf ("\n%.*s\n", BUFFLEN, buff);
    }
  clearerr (btu);
}

int
quit ()
{
  sendoption (7);
  return 0;
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
  onfiles ();

  sender = (short) atoi (argv[1]);

  printf
    ("\n\tWelcome to new Communication system,\n\t\t%s system is ready to communicate with the world...!!!\n",
     argv[1]);

  int choice = 0;
  do
    {
      printf ("\n\t Enter your choice\n"
	      "1. Enter 1 to Send File\n"
	      "2. Enter 2 to See till received messages\n"
	      "3. Enter 3 to see messages received from specific system\n"
	      "4. Enter 4 to view specific received file\n"
	      "5. Enter 5 to see till sent messages\n"
	      "6. Enter 6 to see Ongoing Sending Message table\n"
	      "7. Enter 7 to exit\n");

      scanf ("%d", &choice);

      switch (choice)
	{
	case 1:
	  sendmsg ();
	  choice = 0;
	  break;
	case 2:
	  displaytrm ();
	  choice = 0;
	  break;
	case 3:
	  displayspecifictrm ();
	  choice = 0;
	  break;
	case 4:
	  seemsg ();
	  choice = 0;
	  break;
	case 5:
	  displaytsm ();
	  choice = 0;
	  break;
	case 6:
	  displayosmt ();
	  choice = 0;
	  break;
	case 7:
	  printf
	    ("Thank you for using our communication system\n\t%s system is going offline\n",
	     argv[1]);
	  quit ();
	  break;
	default:
	  printf ("Sorry wrong choice please try again\n");
	  break;
	}
    }
  while (choice != 7);

  if (utb)
    fclose (utb);
  if (btu)
    fclose (btu);
  return 0;
}

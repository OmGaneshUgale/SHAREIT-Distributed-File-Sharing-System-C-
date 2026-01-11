#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
#include<stdint.h>

#define BUFFLEN 1024
#define MAX 256
#define MAXPARTS 10000

// Header layout (binary-safe)
#define optionsize  0		// uint8_t
#define sendr       1		// uint16_t
#define receivr     3		// uint16_t
#define msgnoi      5		// uint16_t
#define noofpart    7		// uint32_t
#define payloadlen  11		// uint32_t
#define actmsg      15		// payload start

int ear_pid = -1;
int mouth_pid = -1;
int userio_pid = -1;

char fnamebtm[MAX];
char fnamebtu[MAX];
char fnameutb[MAX];
char fnameetb[MAX];

FILE *btm = NULL, *btu = NULL, *utb = NULL, *etb = NULL;

unsigned short sender = 0;
static unsigned short msgnocount = 0;

struct table
{
  unsigned char option;
  unsigned short system;
  unsigned short msgno;
  unsigned int totalpart;
  unsigned char vector[MAXPARTS];	// bitmap per part
  unsigned char part[MAXPARTS][BUFFLEN];	// full frames for each part
  uint32_t partlens[MAXPARTS];	// actual payload length per part
  char fname[MAX];		// original filename
  struct table *next;
};
typedef struct table table;

struct table *osmt = NULL, *ormt = NULL, *tsm = NULL, *trm = NULL;

void handle_sigint (int sig)
{
  (void) sig;
  if(etb)
    fclose (etb);
  if(btm)
    fclose (btm);
  if(utb)
    fclose (utb);
  if(btu)
    fclose (btu);

  FILE *f;
  f = fopen (fnameetb, "wb");
  if(f)
    fclose (f);
  f = fopen (fnamebtm, "wb");
  if(f)
    fclose (f);
  f = fopen (fnameutb, "wb");
  if(f)
    fclose (f);
  f = fopen (fnamebtu, "wb");
  if(f)
    fclose (f);

  if(ear_pid >= 0)
  {
      kill (ear_pid, SIGKILL);
      wait (NULL);
  }
  if(mouth_pid >= 0)
  {
      kill (mouth_pid, SIGKILL);
      wait (NULL);
  }
  if(userio_pid >= 0)
  {
      kill (userio_pid, SIGKILL);
      wait (NULL);
  }
  exit (0);
}

int createfilename (char *argv)
{
  //it creates the string by concatenating just like strcat
  snprintf (fnamebtm, sizeof (fnamebtm), "%sbtm.txt", argv);
  snprintf (fnamebtu, sizeof (fnamebtu), "%sbtu.txt", argv);
  snprintf (fnameetb, sizeof (fnameetb), "%setb.txt", argv);
  snprintf (fnameutb, sizeof (fnameutb), "%sutb.txt", argv);
  return 0;
}

int switchear (char *fnameetb, char *argv)
{
  etb = fopen (fnameetb, "rb");
  if(!etb)
  {
      etb = fopen (fnameetb, "wb+");
      if(!etb)
      {
	  perror ("Error while opening etb.txt file");
	  exit (EXIT_FAILURE);
      }
      fclose (etb);
      etb = fopen (fnameetb, "rb");
    }

  ear_pid = fork ();
  char *argtoear[3];
  char *ear = "./recv";
  argtoear[0] = ear;
  argtoear[1] = argv;
  argtoear[2] = NULL;

  if(ear_pid < 0)
  {
      perror ("Error while creating ear child");
      exit (EXIT_FAILURE);
  }
  else if(ear_pid == 0)
  {
      int re = execv (ear, (char **) argtoear);
      if(re < 0)
      {
	  perror ("Error while calling ear child");
	  exit (EXIT_FAILURE);
      }
      return 0;
  }
  return 0;
}

int switchmouth (char *fnamebtm, char *argv)
{
  btm = fopen (fnamebtm, "ab");
  if(!btm)
  {
      perror ("Error while creating file btm.txt");
      exit (EXIT_FAILURE);
  }

  char *argtomouth[3];
  char *mouth = "./send";
  argtomouth[0] = mouth;
  argtomouth[1] = argv;
  argtomouth[2] = NULL;

  mouth_pid = fork ();

  if(mouth_pid < 0)
  {
      perror ("Error while creating mouth");
      exit (EXIT_FAILURE);
  }
  else if(mouth_pid == 0)
  {
      int re = execv (mouth, (char **) argtomouth);
      if(re < 0)
      {
	  perror ("Error while calling mouth");
	  exit (EXIT_FAILURE);
      }
  }
  return 0;
}

int switchuserio (char *fnameutb, char *argv)
{
  utb = fopen (fnameutb, "rb");
  if(!utb)
  {
      utb = fopen (fnameutb, "wb+");
      if(!utb)
      {
	  perror ("Error while opening utb.txt file");
	  exit (EXIT_FAILURE);
      }
      fclose (utb);
      utb = fopen (fnameutb, "rb");
  }

  btu = fopen (fnamebtu, "ab");
  if(!btu)
  {
      perror ("Error while creating file btu.txt");
      exit (EXIT_FAILURE);
  }

  char *argtouio[3];
  char *userio = "./userio";
  argtouio[0] = userio;
  argtouio[1] = argv;
  argtouio[2] = NULL;
  userio_pid = fork ();
  if(userio_pid < 0)
  {
      perror ("Error while creating child");
      exit (EXIT_FAILURE);
  }
  else if(userio_pid == 0)
  {
      int re = execv (userio, (char **) argtouio);
      if(re < 0)
      {
	  perror ("Error while calling userio");
	  exit (EXIT_FAILURE);
      }
      return 0;
  }
  return 0;
}

int switchonall (char *argv)
{
  createfilename (argv);
  switchear (fnameetb, argv);
  switchmouth (fnamebtm, argv);
  switchuserio (fnameutb, argv);
  return 0;
}

struct table *createentry (short system, short msgno, int totalpart, const char fname[])
{
  struct table *t = (struct table *) calloc (1, sizeof (struct table));
  t->system = (unsigned short) system;
  t->msgno = (unsigned short) msgno;
  t->totalpart = (unsigned int) totalpart;
  strncpy (t->fname, fname, MAX - 1);
  for(int i = 0; i < MAXPARTS; i++)
    t->vector[i] = 0;
  for(int i = 0; i < MAXPARTS; i++)
    t->partlens[i] = 0;
  t->next = NULL;
  return t;
}

void addentry (short system, short msgno, int totalpart, char fname[], struct table **tab)
{
  struct table *t = *tab;
  if(t == NULL)
  {
      *tab = createentry (system, msgno, totalpart, fname);
      return;
  }
  while (t->next != NULL)
    t = t->next;
  t->next = createentry (system, msgno, totalpart, fname);
}

struct table * getentry (short system, short msgno, struct table *tab)
{
  struct table *t = tab;
  while(t)
  {
      if(t->system == system && t->msgno == msgno)
	return t;
      t = t->next;
  }
  return NULL;
}

void display (struct table *tab)
{
  struct table *t = tab;
  char line[BUFFLEN], put[64];
  while(t)
  {
      memset (line, 0, sizeof (line));
      sprintf (put, "%d", t->system);
      strcat (line, put);
      strcat (line, "\t");
      sprintf (put, "%d", t->msgno);
      strcat (line, put);
      strcat (line, "\t");
      sprintf (put, "%u", t->totalpart);
      strcat (line, put);
      strcat (line, "\t");
      strcat (line, t->fname);
      strcat (line, "\n");
      fwrite (line, 1, BUFFLEN, btu);
      fflush (btu);
      t = t->next;
  }
}

void displayspecific (short sys, struct table *tab)
{
  struct table *t = tab;
  char line[BUFFLEN], put[64];
  while(t)
  {
      if(t->system == sys)
      {
	  memset (line, 0, sizeof (line));
	  sprintf (put, "%d", t->system);
	  strcat (line, put);
	  strcat (line, "\t");
	  sprintf (put, "%d", t->msgno);
	  strcat (line, put);
	  strcat (line, "\t");
	  sprintf (put, "%u", t->totalpart);
	  strcat (line, put);
	  strcat (line, "\t");
	  strcat (line, t->fname);
	  strcat (line, "\n");
	  fwrite (line, 1, BUFFLEN, btu);
	  fflush (btu);
      }
      t = t->next;
    }
}

// Send: build binary-safe parts and write frames to btm
void sendmsg (unsigned char msg[])
{
  char fn[MAX] = { 0 };
  unsigned short receiver = 0;

  memcpy (&sender, &msg[sendr], sizeof (sender));
  memcpy (&receiver, &msg[receivr], sizeof (receiver));
  memcpy (fn, &msg[actmsg], sizeof (fn));
  fn[MAX - 1] = '\0';

  FILE *fp = fopen (fn, "rb");
  if(!fp)
  {
      perror ("Error opening input file");
      return;
  }

  // compute total parts
  size_t max_payload = BUFFLEN - actmsg;
  fseek (fp, 0, SEEK_END);
  long fsize = ftell (fp);
  rewind (fp);
  unsigned int totalparts = (unsigned int) ((fsize + max_payload - 1) / max_payload);
  if(totalparts > MAXPARTS)
  {
      fprintf (stderr, "File too large for MAXPARTS=%d\n", MAXPARTS);
      fclose (fp);
      return;
  }

  addentry (receiver, msgnocount, totalparts, fn, &osmt);
  struct table *t = getentry (receiver, msgnocount, osmt);
  if(!t)
  {
      perror ("osmt getentry");
      fclose (fp);
      return;
  }

  // create start info (option=1) frame: filename as payload
  
    unsigned char start[BUFFLEN] = { 0 };
    unsigned char opt = 1;
    memcpy (&start[optionsize], &opt, 1);
    memcpy (&start[sendr], &sender, sizeof (sender));
    memcpy (&start[receivr], &receiver, sizeof (receiver));
    memcpy (&start[msgnoi], &t->msgno, sizeof (t->msgno));
    memcpy (&start[noofpart], &t->totalpart, sizeof (t->totalpart));
    uint32_t flen = (uint32_t) strnlen (fn, MAX);
    memcpy (&start[payloadlen], &flen, sizeof (flen));
    memcpy (&start[actmsg], fn, flen);
    fwrite (start, 1, BUFFLEN, btm);
    fflush (btm);
  

  // build part frames
  for(unsigned int partNo = 0; partNo < totalparts; partNo++)
  {
      memset (t->part[partNo], 0, BUFFLEN);
      size_t n = fread (&t->part[partNo][actmsg], 1, max_payload, fp);
      uint32_t plen = (uint32_t) n;

      unsigned char opt = 3;
      memcpy (&t->part[partNo][optionsize], &opt, 1);
      memcpy (&t->part[partNo][sendr], &sender, sizeof (sender));
      memcpy (&t->part[partNo][receivr], &receiver, sizeof (receiver));
      memcpy (&t->part[partNo][msgnoi], &t->msgno, sizeof (t->msgno));
      memcpy (&t->part[partNo][noofpart], &partNo, sizeof (partNo));
      memcpy (&t->part[partNo][payloadlen], &plen, sizeof (plen));
      t->partlens[partNo] = plen;
      t->vector[partNo / 8] |= (1 << (partNo % 8));	// mark prepared

      // sender doesn't write parts directly here; waits for ACK (option=2) to know which parts to send
  }

  fclose (fp);
  msgnocount++;
}

// Receiver: start info ack requesting all parts
void sendstartinfoack (unsigned char buff[])
{
  unsigned short senderaddr, receiveraddr, msgno;
  uint32_t totalparts = 0, flen = 0;
  char filename[MAX] = { 0 };

  memcpy (&senderaddr, &buff[sendr], sizeof (senderaddr));
  memcpy (&receiveraddr, &buff[receivr], sizeof (receiveraddr));
  memcpy (&msgno, &buff[msgnoi], sizeof (msgno));
  memcpy (&totalparts, &buff[noofpart], sizeof (totalparts));
  memcpy (&flen, &buff[payloadlen], sizeof (flen));
  if(flen > MAX)
    flen = MAX;
  memcpy (filename, &buff[actmsg], flen);
  filename[flen < MAX ? flen : MAX - 1] = '\0';

  addentry (senderaddr, msgno, totalparts, filename, &ormt);
  struct table *t = getentry (senderaddr, msgno, ormt);
  if(!t)
  {
      perror ("ormt getentry");
      return;
  }

  unsigned char tosendmsg[BUFFLEN] = { 0 };
  unsigned char opt = 2;
  memcpy (&tosendmsg[optionsize], &opt, 1);
  memcpy (&tosendmsg[sendr], &receiveraddr, sizeof (receiveraddr));	// from us
  memcpy (&tosendmsg[receivr], &senderaddr, sizeof (senderaddr));	// to sender
  memcpy (&tosendmsg[msgnoi], &msgno, sizeof (msgno));
  memcpy (&tosendmsg[noofpart], &totalparts, sizeof (totalparts));
  // request bitmap: all ones for totalparts
  int vecbytes = (int) (totalparts / 8) + 1;
  uint32_t vlen = (uint32_t) vecbytes;
  memcpy (&tosendmsg[payloadlen], &vlen, sizeof (vlen));
  memset (&tosendmsg[actmsg], 0xFF, vlen);

  fwrite (tosendmsg, 1, BUFFLEN, btm);
  fflush (btm);
}

// Sender: upon request (option=2), send requested parts and end info
void sendpart (unsigned char buff[])
{
  unsigned short senderaddr, receiveraddr, msgno;
  uint32_t totalparts = 0, vlen = 0;

  memcpy (&senderaddr, &buff[sendr], sizeof (senderaddr));	// remote receiver
  memcpy (&receiveraddr, &buff[receivr], sizeof (receiveraddr));	// our sender
  memcpy (&msgno, &buff[msgnoi], sizeof (msgno));
  memcpy (&totalparts, &buff[noofpart], sizeof (totalparts));
  memcpy (&vlen, &buff[payloadlen], sizeof (vlen));

  unsigned char req[MAXPARTS] = { 0 };
  if(vlen > sizeof (req))
    vlen = sizeof (req);
  memcpy (req, &buff[actmsg], vlen);

  struct table *t = getentry (senderaddr, msgno, osmt);
  if(!t)
  {
      perror ("sendpart: osmt getentry");
      return;
  }

  for(uint32_t i = 0; i < totalparts; i++)
  {
      if((req[i / 8] & (1 << (i % 8))) != 0)
      {
	  fwrite (t->part[i], 1, BUFFLEN, btm);
	  fflush (btm);
      }
  }

  // end info (option=4)
  unsigned char endmsg[BUFFLEN] = { 0 };
  unsigned char opt = 4;
  uint32_t zero = 0;
  memcpy (&endmsg[optionsize], &opt, 1);
  memcpy (&endmsg[sendr], &receiveraddr, sizeof (receiveraddr));
  memcpy (&endmsg[receivr], &senderaddr, sizeof (senderaddr));
  memcpy (&endmsg[msgnoi], &msgno, sizeof (msgno));
  memcpy (&endmsg[noofpart], &totalparts, sizeof (totalparts));
  memcpy (&endmsg[payloadlen], &zero, sizeof (zero));
  fwrite (endmsg, 1, BUFFLEN, btm);
  fflush (btm);
}

// Receiver: add incoming part
void addpart (unsigned char buff[])
{
  uint32_t partno = 0, plen = 0;
  unsigned short senderaddr, receiveraddr, msgno;

  memcpy (&senderaddr, &buff[sendr], sizeof (senderaddr));
  memcpy (&receiveraddr, &buff[receivr], sizeof (receiveraddr));
  memcpy (&msgno, &buff[msgnoi], sizeof (msgno));
  memcpy (&partno, &buff[noofpart], sizeof (partno));
  memcpy (&plen, &buff[payloadlen], sizeof (plen));

  struct table *t = getentry (senderaddr, msgno, ormt);
  if(!t)
  {
      perror ("addpart: ormt getentry");
      return;
  }
  if(partno >= MAXPARTS)
    return;

  size_t max_payload = BUFFLEN - actmsg;
  if(plen > max_payload)
    plen = (uint32_t) max_payload;

  // copy only payload
  memcpy (&t->part[partno][actmsg], &buff[actmsg], plen);
  t->partlens[partno] = plen;

  t->vector[partno / 8] |= (1 << (partno % 8));	// mark received
}

// Receiver: end info ack — compute missing; if none, assemble file
void sendendinfoack (unsigned char buff[])
{
  unsigned short senderaddr, receiveraddr, msgno;
  uint32_t totalparts = 0;

  memcpy (&senderaddr, &buff[sendr], sizeof (senderaddr));
  memcpy (&receiveraddr, &buff[receivr], sizeof (receiveraddr));
  memcpy (&msgno, &buff[msgnoi], sizeof (msgno));
  memcpy (&totalparts, &buff[noofpart], sizeof (totalparts));

  struct table *t = getentry (senderaddr, msgno, ormt);
  if(!t)
  {
      perror ("endinfoack: ormt getentry");
      return;
  }

  int missed = 0;
  for(uint32_t i = 0; i < totalparts; i++)
  {
      if((t->vector[i / 8] & (1 << (i % 8))) == 0)
	missed++;
  }

  unsigned char tosendmsg[BUFFLEN] = { 0 };
  unsigned char opt = 5;
  memcpy (&tosendmsg[optionsize], &opt, 1);
  memcpy (&tosendmsg[sendr], &receiveraddr, sizeof (receiveraddr));
  memcpy (&tosendmsg[receivr], &senderaddr, sizeof (senderaddr));
  memcpy (&tosendmsg[msgnoi], &msgno, sizeof (msgno));
  memcpy (&tosendmsg[noofpart], &missed, sizeof (missed));

  int vecbytes = (int) (totalparts / 8) + 1;
  uint32_t vlen = (uint32_t) vecbytes;
  memcpy (&tosendmsg[payloadlen], &vlen, sizeof (vlen));

  if(missed == 0)
  {
      // assemble file (binary-safe)
      char outname[(3 * MAX)] = { 0 };
      char put[64];
      strcat (outname, "recv");
      snprintf (put, sizeof (put), "%d", t->system);
      strcat (outname, put);
      strcat (outname, t->fname);

      FILE *fp = fopen (outname, "wb");
      if(!fp)
      {
	  perror ("Error while creating the received file");
	  return;
      }
      for(uint32_t i = 0; i < t->totalpart; i++)
      {
	  uint32_t len = t->partlens[i];
	  if (len > 0)
	    fwrite (&t->part[i][actmsg], 1, len, fp);
      }
      fclose (fp);

      addentry (t->system, t->msgno, t->totalpart, outname, &trm);
      // cleanup receive entry
      // (optional: implement rementry that updates global head)
      // For now skip full removal to keep simple.

      memset (&tosendmsg[actmsg], 0, vlen);	// no missing
    }
  else
  {
      unsigned char missing[MAXPARTS] = { 0 };
      for(uint32_t i = 0; i < totalparts; i++)
      {
	  if((t->vector[i / 8] & (1 << (i % 8))) == 0)
	    missing[i / 8] |= (1 << (i % 8));
      }
      memcpy (&tosendmsg[actmsg], missing, vlen);
  }

  fwrite (tosendmsg, 1, BUFFLEN, btm);
  fflush (btm);
}

// Sender: handle missing parts request
void sendmissingpart (unsigned char buff[])
{
  uint32_t totalparts = 0, vlen = 0;
  unsigned short senderaddr, receiveraddr, msgno;

  memcpy (&senderaddr, &buff[sendr], sizeof (senderaddr));
  memcpy (&receiveraddr, &buff[receivr], sizeof (receiveraddr));
  memcpy (&msgno, &buff[msgnoi], sizeof (msgno));
  memcpy (&totalparts, &buff[noofpart], sizeof (totalparts));
  memcpy (&vlen, &buff[payloadlen], sizeof (vlen));

  if(totalparts == 0)
  {
      struct table *sys = getentry (senderaddr, msgno, osmt);
      if(sys)
	addentry (sys->system, sys->msgno, sys->totalpart, sys->fname, &tsm);
      return;
  }

  unsigned char req[MAXPARTS] = { 0 };
  if(vlen > sizeof (req))
    vlen = sizeof (req);
  memcpy (req, &buff[actmsg], vlen);

  struct table *t = getentry (senderaddr, msgno, osmt);
  if(!t)
  {
      perror ("sendmissingpart: osmt getentry");
      return;
  }

  for(uint32_t i = 0; i < totalparts; i++)
  {
      if((req[i / 8] & (1 << (i % 8))) != 0)
      {
	  fwrite (t->part[i], 1, BUFFLEN, btm);
	  fflush (btm);
      }
  }

  // end info again
  unsigned char endmsg[BUFFLEN] = { 0 };
  unsigned char opt = 4;
  uint32_t zero = 0;
  memcpy (&endmsg[optionsize], &opt, 1);
  memcpy (&endmsg[sendr], &receiveraddr, sizeof (receiveraddr));
  memcpy (&endmsg[receivr], &senderaddr, sizeof (senderaddr));
  memcpy (&endmsg[msgnoi], &msgno, sizeof (msgno));
  memcpy (&endmsg[noofpart], &t->totalpart, sizeof (t->totalpart));
  memcpy (&endmsg[payloadlen], &zero, sizeof (zero));
  fwrite (endmsg, 1, BUFFLEN, btm);
  fflush (btm);
}

int main (int argc, char *argv[])
{
  signal (SIGINT, handle_sigint);
  if(argc < 2)
  {
      perror ("Receiver Port not given");
      exit (EXIT_FAILURE);
  }

  switchonall (argv[1]);
  sender = (short) atoi (argv[1]);

  unsigned char BUFFERU[BUFFLEN], BUFFERE[BUFFLEN];
  unsigned char choice1, choice2;

  while(1)
  {
      size_t n1 = fread (BUFFERU, 1, BUFFLEN, utb);
      if(n1 == BUFFLEN)
      {
	  memcpy (&choice1, &BUFFERU[optionsize], sizeof (choice1));
	  switch ((int) choice1)
	  {
	    case 1:
	      sendmsg (BUFFERU);
	      break;
	    case 2:
	      display (trm);
	      break;
	    case 3:
	      short sys;
      	      memcpy (&sys, &BUFFERU[sendr], sizeof (sys));
	      displayspecific (sys, trm);
	      break;
	    case 5:
	      display (tsm);
	      break;
	    case 6:
	      display (osmt);
	      break;
	    case 7:
	      handle_sigint (SIGINT);
	      return 0;
	    default:
	      break;
      	  }
      }
      else
      {
	  clearerr (utb);
      }

      size_t n2 = fread (BUFFERE, 1, BUFFLEN, etb);
      if(n2 == BUFFLEN)
      {
	  memcpy (&choice2, &BUFFERE[optionsize], sizeof (choice2));
	  switch ((int) choice2)
	  {
	    case 1:
	      sendstartinfoack (BUFFERE);
	      break;
	    case 2:
	      sendpart (BUFFERE);
	      break;
	    case 3:
	      addpart (BUFFERE);
	      break;
	    case 4:
	      sendendinfoack (BUFFERE);
	      break;
	    case 5:
	      sendmissingpart (BUFFERE);
	      break;
	    default:
	      break;
	  }
      }
      else
      {
	  clearerr (etb);
      }

      usleep (1000);
  }
}

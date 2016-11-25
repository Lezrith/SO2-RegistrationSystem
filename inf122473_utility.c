#include "inf122473_utility.h"

ssize_t readline(char **lineptr, FILE *stream)
{
  size_t len = 0;  // Size of the buffer, ignored.
  
  ssize_t chars = getline(lineptr, &len, stream);
  
  if ((*lineptr)[chars - 1] == '\n') 
  {
    (*lineptr)[chars - 1] = '\0';
    --chars;
  }
  
  return chars;
}

int clean_stdin()
{
  while (getchar()!='\n');
  return 1;
}


void SendSimpleMessage(int qid, long type, int subtype, int id)
{
  msgbuf msg;
  msg.name[0]='\0';
  msg.pesel[0]='\0';
  msg.type=type;
  msg.subtype=subtype;
  msg.id=id;
  msgsnd(qid, &msg, MSGBUFSIZE, 0);
}

void SendDateMessage(int qid, long type, int subtype, int id, Tdate *d)
{
  msgbuf msg;
  msg.name[0]='\0';
  msg.pesel[0]='\0';
  msg.type=type;
  msg.subtype=subtype;
  msg.id=id;
  msg.date=*d;
  msgsnd(qid, &msg, MSGBUFSIZE, 0);
}

void AddDaySchedule(daySchedule* ds)
{
  ds->next=(daySchedule*) malloc(sizeof(daySchedule));
  
  for(int z=0; z<3;z++)
  {
    ds->next->hours[z]=10+z*2;
    ds->next->patientsAppointed[z]=NULL;
  }
  ds->next->next=NULL;
}

void UpdateDate(Tdate *d, int i)
{
  d->day+=i;
  d->weekDay=(d->weekDay+i)%7;
  if(d->weekDay==0) d->weekDay=7;
  while(1)
  {
    if(d->month==1 || d->month==3 || d->month==5 || d->month==7 || d->month==8 || d->month==10 || d->month==12)
    {
      if(d->day<=31) break;
      if(d->day/31>1)
      {
	d->month++;
	d->day-=31;
	continue;
      }
      else
      {
	d->month++;
	d->day-=31;
	break;
      }
    }
    else
    {
      if(d->month==2)
      {
	if ((d->year & 3) == 0 && ((d->year % 25) != 0 || (d->year & 15) == 0))
	{
	  if(d->day<=29) break;
	  if(d->day/29>1)
	  {
	    d->month++;
	    d->day-=29;
	    continue;
	  }
	  else
	  {
	    d->month++;
	    d->day-=29;
	    break;
	  }
	}
	else
	{
	  if(d->day<=28) break;
	  if(d->day/28>1)
	  {
	    d->month++;
	    d->day-=28;
	    continue;
	  }
	  else
	  {
	    d->month++;
	    d->day-=28;
	    break;
	  }
	}
      }
      else
      {
	if(d->day<=30) break;
	if(d->day/30>1)
	{
	  d->month++;
	  d->day-=30;
	  continue;
	}
	else
	{
	  d->month++;
	  d->day-=30;
	  break;
	}
      }
    }
  }
  if(d->month>12)
  {
    d->year+=d->month/12;
    d->month%=12;
  }
  
}

inline void PrintDate(Tdate *d)
{
  switch(d->weekDay)
  {
    case 1:
      printf("poniedzialek, ");
      break;
    case 2:
      printf("wtorek, ");
      break;
    case 3:
      printf("sroda, ");
      break;
    case 4:
      printf("czwartek, ");
      break;
    case 5:
      printf("piatek, ");
      break;
    case 6:
      printf("sobota, ");
      break;
    case 7:
      printf("niedziela, ");
      break;
  }
  printf("%i.%i.%i\n",d->day,d->month,d->year);
}

void GetDayOfWeek(Tdate *d)
{
  struct tm timeIn = { 0, 0, 0, d->day, d->month, d->year - 1900 };
  time_t timeTemp = mktime( & timeIn );
  struct tm const *timeOut = gmtime( & timeTemp );
  d->weekDay=timeOut->tm_wday;
  if(d->weekDay==0) d->weekDay=7;
}

int DaysBetweenDates(Tdate *d1, Tdate *d2)
{
  struct tm a = {0,0,0,d1->day,d1->month,d1->year-1900};
  struct tm b = {0,0,0,d2->day,d2->month,d2->year-1900};
  time_t x = mktime(&a);
  time_t y = mktime(&b);
  double difference=0;
  if ( x != (time_t)(-1) && y != (time_t)(-1) )
  {
    difference = difftime(y, x) / (60 * 60 * 24);
  }
  return (int)floor(difference);
}

void SemP(int semid, int semnum, int value)
{
  struct sembuf buf;
  buf.sem_num = semnum;
  buf.sem_op = value;
  buf.sem_flg = 0;
  if(semop(semid, &buf, 1)==-1)
  {
    perror("Opuszczenie semafora");
    exit(1);
  }
}

void SemV(int semid, int semnum, int value)
{
  struct sembuf buf;
  buf.sem_num = semnum;
  buf.sem_op = -value;
  buf.sem_flg = 0;
  if(semop(semid, &buf, 1)==-1)
  {
    perror("Podnoszenie semafora");
    exit(1);
  }
}

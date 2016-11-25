#include "inf122473_p.h"
#include "../inf122473_utility.h"
int qid;

void Login()
{
  char *name, *pesel;
  size_t nameSize=60, peselSize=11;
  name=(char *)malloc(nameSize * sizeof(char));
  pesel=(char *)malloc(peselSize * sizeof(char));
  printf("Podaj imie i nazwisko (max 60 znakow).\n");
  readline(&name, stdin);
  printf("Podaj pesel (11 cyfr).\n");
  readline(&pesel, stdin);
  
  msgbuf msg;
  msg.type=1;
  msg.subtype=1;
  msg.id=getpid();
  strcpy(msg.name, name);
  strcpy(msg.pesel, pesel);
  msgsnd(qid, &msg, MSGBUFSIZE, 0);
  //printf("wiadomosc wyslana\n");
  free(name);
  free(pesel);
  
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  //printf("%i, %li, %i, %i\n", getpid(), msg.type, msg.subtype, msg.id);
  if(msg.subtype==1 && msg.id!=-1)
  {
    if(msg.id==0) printf("Logowanie do rejestracji pomyślne.\n");
    else printf("Logowanie do rejestracji niepomyślne. Pacjent jest już zalogowany!\n");
  }else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 1\n", msg.subtype);
    exit(1);
  }
}

void Logout()
{
  SendSimpleMessage(qid, 1, 1, -getpid());
  msgbuf msg;
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  if(msg.subtype==1)
  {
    if(msg.id==0) printf("Wylogowanie pomyślne.\n");
    else printf("Wylogowanie niepomyślne. Pacjent nie był zalogowany!\n");
  }else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 1.\n", msg.subtype);
    exit(1);
  }
}

void GetDoctorsOnGivenDay()
{
  Tdate date;
  msgbuf msg;
  while(1)
  {
    printf("Podaj termin w formacie [dzien] [miesiac] [rok]\n");
    scanf("%i%i%i", &date.day, &date.month, &date.year);
    GetDayOfWeek(&date);
    SendDateMessage(qid, 1, 2, getpid(), &date);
    msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
    if(msg.subtype==2)
    {
      if(msg.id==0)
      {
	printf("Klinika jest zamknięta w tym dniu\n");
	break;
      }
      else if(msg.id==-1)
      { 
	printf("Wprowadź datę po dniu dzisiejszym: %i.%i.%i\n", msg.date.day, msg.date.month, msg.date.year);
	continue;
      }
      else if(msg.id==-2)
      {
	printf("Grafik na te dni nie jest jeszcze ustalony. Wprowadź datę przed: %i.%i.%i\n", msg.date.day, msg.date.month, msg.date.year);
	continue;
      }
      else
      {
	printf("W tym dniu przyjmują:\n doktor %s,\n", msg.name);
	for(int i=1; i<msg.id; i++)
	{
	  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
	  printf(" doktor %s,\n", msg.name);
	}
	break;
      }
    }
    else
    {
      printf("Błąd komunikacji. msg.subtype==%i oczekiwano 2\n", msg.subtype);
      exit(1);
    }
  }
}

void GetAvailableHoursOnGivenDay()
{
  Tdate date;
  msgbuf msg;
  while(1)
  {
    printf("Podaj termin w formacie [dzien] [miesiac] [rok]\n");
    scanf("%i%i%i", &date.day, &date.month, &date.year);
    GetDayOfWeek(&date);
    SendDateMessage(qid, 1, 3, getpid(), &date);
    msgrcv(qid, &msg, MSGBUFSIZE, getpid(),0);
    if(msg.subtype==3)
    {
      if(msg.id==0)
      {
	printf("Klinika jest zamknięta w tym dniu\n");
	break;
      }
      else if(msg.id==-1)
      { 
	printf("Wprowadź datę po dniu dzisiejszym: %i.%i.%i\n", msg.date.day, msg.date.month, msg.date.year);
	continue;
      }
      else if(msg.id==-2)
      {
	printf("Grafik na te dni nie jest jeszcze ustalony. Wprowadź datę przed: %i.%i.%i\n", msg.date.day, msg.date.month, msg.date.year);
	continue;
      }
      else
      {
	printf("Wolne terminy w tym dniu:\n");
	for(int i=msg.id; i>0;i--)
	{
	  msgrcv(qid, &msg, MSGBUFSIZE, getpid(),0);
	  printf("dr %s ", msg.name);
	  if(msg.id==0) printf("Brak wolnych terminów.\n");
	  else
	  {
	    if(msg.id & 1) printf(" %i:00", msg.date.day);
	    msg.id=msg.id >> 1;
	    if(msg.id & 1) printf(" %i:00", msg.date.month);
	    msg.id=msg.id >> 1;
	    if(msg.id & 1) printf(" %i:00", msg.date.year);
	    printf("\n");
	  }
	}
	break;
      }
    }
    else
    {
      printf("Błąd komunikacji. msg.subtype==%i oczekiwano 3\n", msg.subtype);
      exit(1);
    }
  }
}

void GetAvailableHoursOfDoctor()
{
  msgbuf msg;
  SendSimpleMessage(qid, 1, 4, getpid());
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  int numOfDoctors=msg.id;
  if(msg.subtype==4)
  {
    printf("Wybierz lekarza.\n");
    for(int i=numOfDoctors; i>0; i--)
    {
      msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
      printf("%i. dr %s\n", 6-i, msg.name);
    }
    int a;
    short done;
    do
    {
      done=0;
      scanf("%i", &a);
      a-=1;
      for(int i=numOfDoctors; i>0; i--)
      {
	if(a==i)
	{
	  Tdate d={i,0,0};
	  SendDateMessage(qid, 1, 5, getpid(),&d);
	  printf("Wysyłam wiadmość: 1, 5, %i, %i\n", getpid(), a);
	  done=1;
	  break;
	}
	if(i==1)
	{
	  printf("Wprowadziłeś niepoprawny numer. Spróbuj ponownie.\n");
	  done=0;
	}
      }
    }while(!done);
    msgbuf2 msg2;
    msgrcv(qid, &msg, MSGBUFSIZE, getpid(),0);
    Tdate d=msg.date;
    for(int i=0; i< msg.id; i++)
    { 
      msgrcv(qid, &msg2, MSGBUF2SIZE, getpid(),0);
      for(int j=0; j<30; j++)
      {
	PrintDate(&d);
	UpdateDate(&d, 1);
	if(msg2.appointments[j]==0) printf("Brak wolnych terminów\n");
	else
	{
	  if(msg2.appointments[j] & 1) printf(" %i:00", msg2.hours[3*j]);
	  msg2.appointments[j]=msg2.appointments[j] >> 1;
	  if(msg2.appointments[j] & 1) printf(" %i:00", msg2.hours[3*j+1]);
	  msg2.appointments[j]=msg2.appointments[j] >> 1;
	  if(msg2.appointments[j] & 1) printf(" %i:00", msg2.hours[3*j+2]);
	  printf("\n");
	}
      }
    }
  }
  else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 4\n", msg.subtype);
    exit(1);
  }
}

void GetAppointmentStatus()
{
  SendSimpleMessage(qid, 1, 6, getpid());
  msgbuf msg;
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  if(msg.subtype==6)
  {
    if(msg.id==-1) printf("Nie masz umówionej wizyty.\n");
    else
    {
      printf("Masz wizytę u dr %s o godzinie %i:00 w ", msg.name, msg.id);
      PrintDate(&msg.date);
    }
  }
  else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 6\n", msg.subtype);
    exit(1);
  }
}

void MakeAppointmentIn2Months()
{
  msgbuf msg;
  SendSimpleMessage(qid,1,9,getpid());
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  if(msg.subtype==9)
  {
    if(msg.id<0)
    {
      printf("Masz już wizytę u dr %s o %i:00 w ", msg.name, -msg.id);
      PrintDate(&msg.date);
    }
    else
    {
      printf("Masz wizytę u dr %s o %i:00 w ", msg.name, msg.id);
      PrintDate(&msg.date);
    }
  }
  else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 9\n", msg.subtype);
    exit(1);
  }
}

void MakeAppointmentAfter2Months()
{
  msgbuf msg;
  SendSimpleMessage(qid,1,10,getpid());
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  if(msg.subtype==10)
  {
    if(msg.id<0)
    {
      printf("Masz już wizytę u dr %s o %i:00 w ", msg.name, -msg.id);
      PrintDate(&msg.date);
    }
    else
    {
      printf("Masz wizytę u dr %s o %i:00 w ", msg.name, msg.id);
      PrintDate(&msg.date);
    }
  }
  else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 10\n", msg.subtype);
    exit(1);
  }
}

void ChangeAppointmentDate()
{
  msgbuf msg;
  SendSimpleMessage(qid, 1, 7, getpid());
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  if(msg.subtype==7)
  {
    if(msg.id==-1) printf("Nie masz umówionej wizyty.\n");
    else
    {
      printf("Poprzedni termin wizyty u dr %s o %i:00 w ", msg.name, msg.id);
      PrintDate(&msg.date);
      msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
      if(msg.id==-2) printf("Brak wolnych terminów. Obowiązuje poprzedni termin wizyty\n");
      else
      {
	printf("Obecny termin wizyty u dr %s o %i:00 w ", msg.name, msg.id);
	PrintDate(&msg.date);
      }
    }
  }
  else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 7\n", msg.subtype);
    exit(1);
  }
}

void CancelAppointment()
{
  msgbuf msg;
  SendSimpleMessage(qid, 1, 8, getpid());
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  if(msg.subtype==8)
  {
    if(msg.id==-1) printf("Nie masz umówionej wizyty.\n");
    else printf("Wizyta odwołana.\n");
  }
  else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 8\n", msg.subtype);
    exit(1);
  }
}

void AskForConfirmation(msgbuf *msg)
{
  printf("UWAGA! Twoja wizyta wymaga potierdzenia.\n");
  printf("Obecny termin wizyty u dr %s o %i:00 w ", msg->name, msg->id);
  PrintDate(&msg->date);
  while(1)
  {
    printf("Czy nadal chcesz zostać przyjęty w tym terminie? [T/N]\n");
    char c;
    scanf("%c", &c);
    if(c == 'T' || c=='t') break;
    else if(c == 'N' || c=='n')
    {
      CancelAppointment();
      break;
    }
    else
    {
      printf("Wpisałeś nieprawidłowy znak. Spróbuj ponownie.\n");
      clean_stdin();
    }
  }
  clean_stdin();
}

int main()
{
  char c;
  qid=msgget(1403, 0644);
  if(fork()==0)
  {
    msgbuf msg;
    prctl(PR_SET_PDEATHSIG, SIGQUIT);
    msgrcv(qid, &msg, MSGBUFSIZE, 2, 0);
    AskForConfirmation(&msg);
    usleep(200);
  }
  else
  {
    printf("Witaj w rejestracji lekarza specjalisty!\n");
    Login();
    signal(SIGINT, Logout);
    int n=0;
    do
    {
      n=0;
      printf("Wybierz czynność i nacisnij ENTER.\n");
      printf("1. Wyświetl listę lekarzy przyjmujacych w danym teminie.\n");
      printf("2. Wyświetl wolne terminy w danym dniu.\n");
      printf("3. Wyświetl wolne terminy danego lekarza.\n");
      printf("4. Sprawdz termin swojej wizyty.\n");
      printf("5. Zmień termin wizyty.\n");
      printf("6. Odwołaj wizytę.\n");
      printf("7. Zapisz się na wizytę w ciągu 2 miesięcy.\n");
      printf("8. Zapisz się na wizytę po 2 miesiącach.\n");
      printf("9. Zakończ.\n");
      fflush(stdout);
      while (((scanf("%d%c", &n, &c)!=2 || c!='\n') && clean_stdin()) || n<1 || n>9)
      {  
	printf("Wprowadziles blędny numer! Spróbuj ponownie.\n");
      }
      switch(n)
      {
	case 1: 
	  GetDoctorsOnGivenDay();
	  break;
	case 2:
	  GetAvailableHoursOnGivenDay();
	  break;
	case 3:
	  GetAvailableHoursOfDoctor();
	  break;
	case 4:
	  GetAppointmentStatus();
	  break;
	case 5:
	  ChangeAppointmentDate();
	  break;
	case 6:
	  CancelAppointment();
	  break;
	case 7:
	  MakeAppointmentIn2Months();
	  break;
	case 8:
	  MakeAppointmentAfter2Months();
	  break;
	case 9:
	  break;
	default:
	  printf("Wprowadziles blędny numer! Spróbuj ponownie.\n");
	  n=0;
	  break;
      }
    }while(n!=9);
    Logout();
  }
  return 0;
}

#include "inf122473_l.h"
#include "../inf122473_utility.h"
int qid, number;
char *name;

void Login()
{
  char *line;
  size_t nameSize=60;
  name=(char *)malloc(nameSize * sizeof(char));
  line=(char *)malloc(512 *sizeof(char));
  while(1)
  {
    name[0]='\0';
    line[0]='\0';
    printf("Podaj imie i nazwisko (max 60 znakow).\n");
    readline(&name, stdin);
    printf("Podaj hasło.\n");
    readline(&line, stdin);
    number=atoi(line);
    
    msgbuf msg;
    msg.type=1;
    msg.subtype=11;
    msg.id=getpid();
    strcpy(msg.name, name);
    msg.date.day=number;
    msgsnd(qid, &msg, MSGBUFSIZE, 0);
    free(line);
    msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
    //printf("%i, %li, %i, %i\n", getpid(), msg.type, msg.subtype, msg.id);
    if(msg.subtype==11)
    {
      if(msg.id==0)
      {
	printf("Logowanie do rejestracji pomyślne.\n");
	break;
      }
      else if(msg.id==-1) printf("Logowanie do rejestracji niepomyślne. Lekarz jest już zalogowany!\n");
      else if(msg.id==-2) printf("Logowanie do rejestracji niepomyślne. Wprowadzono błędne dane!\n");
    }else
    {
      printf("Błąd komunikacji. msg.subtype==%i oczekiwano 11\n", msg.subtype);
      exit(1);
    }
  }
}

void Logout()
{
  msgbuf msg;
  msg.type=1;
  msg.subtype=11;
  msg.id=-getpid();
  msg.date.day=number;
  msgsnd(qid, &msg, MSGBUFSIZE, 0);
  msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
  if(msg.subtype==11)
  {
    if(msg.id==0) printf("Wylogowanie pomyślne.\n");
    else printf("Wylogowanie niepomyślne. Doctor nie był zalogowany!\n");
  }else
  {
    printf("Błąd komunikacji. msg.subtype==%i oczekiwano 11.\n", msg.subtype);
    exit(1);
  }
}

void TakeLeave()
{
  msgbuf msg;
  Tdate d1, d2;
  int b;
  while(1)
  {
    printf("Podaj początek urlopu w formacie [dzien] [miesiac] [rok]\n");
    scanf("%i%i%i", &d1.day, &d1.month, &d1.year);
    GetDayOfWeek(&d1);
    printf("Podaj koniec urlopu w formacie [dzien] [miesiac] [rok]\n");
    scanf("%i%i%i", &d2.day, &d2.month, &d2.year);
    GetDayOfWeek(&d2);
    b=DaysBetweenDates(&d1, &d2);
    PrintDate(&d1);
    PrintDate(&d2);
    printf("%i\n", b);
    if(b<=0) printf("Koniec powinien nastąpić co najmniej jeden dzień po początku.\n");
    else
    {
      msg.type=1;
      msg.subtype=12;
      msg.id=getpid();
      strcpy(msg.name, name);
      msg.date=d1;
      msgsnd(qid, &msg, MSGBUFSIZE, 0);
      msg.date=d2;
      msgsnd(qid, &msg, MSGBUFSIZE, 0);
      msgrcv(qid, &msg, MSGBUFSIZE, getpid(), 0);
      if(msg.id==-1)
      { 
	printf("Wprowadź datę po dniu dzisiejszym: %i.%i.%i\n", msg.date.day, msg.date.month, msg.date.year);
	continue;
      }
      if(msg.id==-2)
      {
	printf("Grafik na te dni nie jest jeszcze ustalony. Wprowadź datę przed: %i.%i.%i\n", msg.date.day, msg.date.month, msg.date.year);
	continue;
      }
      if(msg.id==0)
      {
	printf("Operacja zakończona sukcesem. Miłego urlopu.\n");
	break;
      }
    }
  }
}

int main()
{
  qid=msgget(1403, 0644);
  printf("Witaj doktorze!\n");
  Login();
  signal(SIGINT, Logout);
  char c;
  while(1)
  {
    printf("Aby wziąć urlop naciśnij u, aby się wylogować q.\n");
    scanf("%c", &c);
    if(c=='u') TakeLeave(); 
    if(c=='q') break;
    scanf("%c", &c);
  }
  Logout();
  return 0;
}

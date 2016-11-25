#include "inf122473_r.h"
#include "../inf122473_vector.h"
#include "../inf122473_utility.h"
#define NUMOFDOCTORS 5
int qid=-1, semid;
vector patients;
doctor *doctors;
Tdate *date;
int numOfPatients=0;
patient *ghost;

patient* FindPatientById(int id)
{
  patient *p;
  for(int i=0; i<numOfPatients; i++)
  {
    p=VECTOR_GET(patients, patient*, i);
    if(id==p->id)
    {
      break;
    }
  }
  return p;
}

void Close()
{
  printf("Zamykanie programu\n");
  if(qid!=-1)
  {
    if(msgctl(qid, IPC_RMID, NULL)==-1)
    {
      perror("Usuwanie kolejki");
      exit(1);
    }
  }
  if(semid!=-1)
  {
    if(semctl(semid, 0, IPC_RMID, NULL)==-1)
    {
      perror("Usuwanie kolejki");
      exit(1);
    }
  }
  VECTOR_FREE(patients);
  daySchedule *ds;
  
  for(int i=0; i<NUMOFDOCTORS;i++)
  {
    while(doctors[i].begin!=NULL)
    {
      ds=doctors[i].begin;
      doctors[i].begin=doctors[i].begin->next;
      free(ds);
    }
  }
  munmap(date, sizeof(Tdate));
  munmap(doctors, NUMOFDOCTORS* sizeof(doctor));
  free(ghost);
  exit(0);
}

void Init()
{
  doctors=(doctor*) mmap(NULL, NUMOFDOCTORS*sizeof(doctors), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  printf("Witaj w panelu rejestracji.\n");
  date=(Tdate*)mmap(NULL, sizeof(Tdate), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  time_t t2 = time(NULL);
  struct tm *t= gmtime(&t2);
  date->day=t->tm_mday;
  date->month=t->tm_mon+1;
  date->year=t->tm_year+1900;
  date->weekDay=t->tm_wday;
  if(date->weekDay==0)date->weekDay=7;
  
  printf("Jest ");
  PrintDate(date);
  
  if((qid=msgget(1403, 0644|IPC_CREAT))==-1)
  {
    perror("Otwarcie kolejki");
    exit(1);
  }
  if((semid=semget(2610, NUMOFDOCTORS, 0644|IPC_CREAT))==-1)
  {
    perror("Uzyskanie semafora");
    exit(1);
  }
  int tab[]={1,1,1,1,1};
  semctl(semid, 0, SETALL, tab);
  
  
  
  VECTOR_INIT(patients);
  strcpy(doctors[0].name, "Ryszard Lipton");
  strcpy(doctors[1].name, "Andrzej Kmicic");
  strcpy(doctors[2].name, "Cezary Baryka");
  strcpy(doctors[3].name, "Elżbieta Biecka");
  strcpy(doctors[4].name, "Tomasz Judym");
  
  for(int i=0; i<NUMOFDOCTORS;i++)
  {
    doctors[i].loggedIn=0;
    doctors[i].numOfAppointments=0;
    doctors[i].begin=(daySchedule*) malloc(sizeof(daySchedule));
    for(int z=0; z<3;z++)
    {
      doctors[i].begin->hours[z]=10+z*2;
      doctors[i].begin->patientsAppointed[z]=NULL;
    }
    doctors[i].end=doctors[i].begin;
    for(int j=0; j<180; j++)
    {
      
      AddDaySchedule(doctors[i].end);
      doctors[i].end=doctors[i].end->next;
    }
  }
  
  ghost=(patient*) malloc(sizeof(patient));
  ghost->id=-1403;
  ghost->name[0]='\0';
  ghost->pesel[0]='\0';
  ghost->loggedIn=1;
  ghost->docNum=-2;
}

void LoginPatient(msgbuf *msg)
{
  patient *p;
  short duplicate=0;
  for(int i=0; i<numOfPatients; i++)
  {
    p=VECTOR_GET(patients, patient*, i);
    if(strcmp(msg->pesel, p->pesel)==0)
    {
      if(p->loggedIn==1) SendSimpleMessage(qid, msg->id, 1, -1); //send error 'user already logged in'
      else
      {
	p->id=msg->id;
	p->loggedIn=1;
	SendSimpleMessage(qid, p->id, 1, 0);
	printf("Pacjent %s, %s zalogowal sie z pid %i\n", p->name, p->pesel, p->id);
      }
      duplicate=1;
      break;
    }
  }
  if(duplicate==0)
  {
    p=(struct patient*)malloc(sizeof(patient));
    p->id=msg->id;
    p->docNum=-1;
    Tdate d={.day=-1,.month=-1,.year=-1};
    p->date=d;
    p->hour=-1;
    p->loggedIn=1;
    strcpy(p->name, msg->name);
    strcpy(p->pesel, msg->pesel);
    VECTOR_ADD(patients, p);
    numOfPatients++;
    SendSimpleMessage(qid, p->id, 1, 0);
    printf("Pacjent %s, %s zalogowal sie z pid %i\n", p->name, p->pesel, p->id);
  }
}

void LogoutPatient(int id)
{
  short found=0;
  patient *p;
  for(int i=0; i<numOfPatients; i++)
  {
    p=VECTOR_GET(patients, patient*, i);
    if(p->id==id && p->loggedIn==1)
    {
      found=1;
      printf("Pacjent %s, %s wylogowal sie.\n", p->name, p->pesel);
      p->loggedIn=0;
      SendSimpleMessage(qid, id, 1, 0);
      //numOfPatients--;
      break;
    }
  }
  if(found==0) SendSimpleMessage(qid, id, 1, -1);
}

void PrintPatients()
{
  patient *p;
  printf("Lista pacjentów w przychodni:\n");
  for(int i=0; i<numOfPatients; i++)
  {
    p=VECTOR_GET(patients, patient*, i);
    printf("%s : %i ", p->name, p->id);
    if(p->loggedIn!=1) printf("niezalogowany\n");
    else printf("zalogowany\n");
  }
}
void PrintDoctors()
{
  daySchedule *ds;
  Tdate tempDate;
  for(int i=0; i<NUMOFDOCTORS; i++)
  {
    SemP(semid, i, 1);
    tempDate=*date;
    printf("Doktor %s. Liczba wizyt:%i\n", doctors[i].name, doctors[i].numOfAppointments);
    ds=doctors[i].begin;
    while(ds!=NULL)
    {
      PrintDate(&tempDate);
      if(tempDate.weekDay==6 || tempDate.weekDay==7)
      {
	printf("  Klinika zamknieta\n");
      }
      else
      {
	for(int j=0; j<3;j++)
	{
	  printf("  %i:00 ", ds->hours[j]);
	  if(ds->patientsAppointed[j]==NULL) printf("Wolny\n");
	  else printf("Zajety %s, %s\n", ds->patientsAppointed[j]->name, ds->patientsAppointed[j]->pesel);
	}
      }
      UpdateDate(&tempDate, 1);
      ds=ds->next;
    }
    SemV(semid, i, 1);
  }
}

void SendDoctorsOnGivenDay(msgbuf *msg)
{
  int i=DaysBetweenDates(date, &msg->date);
  printf("Pacjent o pid %i prosi o listę lekarzy z dnia ", msg->id);
  PrintDate(&msg->date);
  Tdate d=*date;
  if(i<0) SendDateMessage(qid, msg->id, 2, -1, &d);
  else if(i>180)
  { 
    UpdateDate(&d, 180);
    SendDateMessage(qid, msg->id, 2, -2, &d);
  }
  else if(msg->date.weekDay==6 || msg->date.weekDay==7) SendSimpleMessage(qid, msg->id, 2, 0);
  else
  {
    msgbuf msgOut;
    msgOut.type=msg->id;
    msgOut.subtype=2;
    msgOut.id=NUMOFDOCTORS;
    for(int i=0; i<NUMOFDOCTORS; i++)
    {
      SemP(semid, i, 1);
      strcpy(msgOut.name, doctors[i].name);
      //printf("Wysylam %s na %li %i %i\n", msgOut.name, msgOut.type, msgOut.subtype, msgOut.id);
      msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
      SemV(semid, i, 1);
    }
  }
}

void SendAvailableHoursOnGivenDay(msgbuf *msg)
{
  int b=DaysBetweenDates(date, &msg->date);
  //printf("Rożnica %i\n", b);
  printf("Pacjent o pid %i prosi o listę terminów w dniu ", msg->id);
  PrintDate(&msg->date);
  Tdate d=*date;
  if(b<0) SendDateMessage(qid, msg->id, 3, -1, &d);
  else if(b>180)
  { 
    UpdateDate(&d, 180);
    SendDateMessage(qid, msg->id, 3, -2, &d);
  }
  else if(msg->date.weekDay==6 || msg->date.weekDay==7) SendSimpleMessage(qid, msg->id, 3, 0);
  else
  {
    daySchedule *ds;
    msgbuf msgOut;
    msgOut.type=msg->id;
    msgOut.subtype=3;
    SendSimpleMessage(qid, msg->id, 3, NUMOFDOCTORS);
    for(int i=0; i<NUMOFDOCTORS; i++)
    {
      SemP(semid, i, 1);
      ds=doctors[i].begin;
      strcpy(msgOut.name, doctors[i].name);
      for(int j=0;j<b;j++)
      {
	ds=ds->next;
      }
      msgOut.date.day=ds->hours[0];
      msgOut.date.month=ds->hours[1];
      msgOut.date.year=ds->hours[2];
      msgOut.id=0;
      if(ds->patientsAppointed[0]==NULL)msgOut.id+=1;
      if(ds->patientsAppointed[1]==NULL)msgOut.id+=2;
      if(ds->patientsAppointed[2]==NULL)msgOut.id+=4;
      //printf("Wysylam %s na %li %i %i\n", msgOut.name, msgOut.type, msgOut.subtype, msgOut.id);
      msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
      SemV(semid, i, 1);
    }
  }
}

void SendDoctorsToChoose(msgbuf *msg)
{
  msgbuf msgOut;
  msgOut.type=msg->id;
  msgOut.subtype=4;
  SendSimpleMessage(qid, msg->id, 4, NUMOFDOCTORS);
  for(int i=0; i<NUMOFDOCTORS; i++)
  {
    strcpy(msgOut.name, doctors[i].name);
    msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
  }
  
}

void SendAvailableHoursOfDoctor(msgbuf *msg)
{
  
  int doctorNumber=msg->date.day;
  SemP(semid, doctorNumber,1);
  printf("Pacjent o pid %i prosi o listę terminów dr %s\n ", msg->id, doctors[doctorNumber].name);
  msgbuf2 msgOut;
  msgOut.type=msg->id;
  SendDateMessage(qid, msg->id, 5, 6, date);
  daySchedule *ds=doctors[doctorNumber].begin;
  for(int i=0; i<6; i++)
  {
    for(int j=0; j<30; j++)
    {
      for(int k=0; k<3; k++)
      {
	msgOut.hours[j*3+k]=ds->hours[k];
      }
      msgOut.appointments[j]=0;
      if(ds->patientsAppointed[0]==NULL)
      {
	msgOut.appointments[j]+=1;
	//printf("1\n");
      }
      if(ds->patientsAppointed[1]==NULL)msgOut.appointments[j]+=2;
      if(ds->patientsAppointed[2]==NULL)msgOut.appointments[j]+=4;
      ds=ds->next;
    }
    msgsnd(qid, &msgOut, MSGBUF2SIZE, 0);
  }
  SemV(semid, doctorNumber,1);
}

void SendAppointmentStatus(msgbuf *msg)
{
  printf("Pacjent o pid %i prosi o termin swojej wizyty ", msg->id);
  patient *p=FindPatientById(msg->id);
  
  if(p->docNum==-1) SendSimpleMessage(qid, msg->id, 6, -1);
  else
  {
    SemP(semid, p->docNum, 1);
    msgbuf msgOut;
    msgOut.type=msg->id;
    msgOut.subtype=6;
    msgOut.date=p->date;
    strcpy(msgOut.name, doctors[p->docNum].name);
    msgOut.id=p->hour;
    msgsnd(qid, &msgOut, MSGBUFSIZE, 0); 
    SemV(semid, p->docNum, 1);
  }
}

void MakeAppointmentIn2Months(msgbuf *msg)
{
  printf("Pacjent o pid %i prosi o rejestrację w ciągu dwóch miesięcy ", msg->id);
  int done=0;
  patient *p=FindPatientById(msg->id);
  if(p->docNum==-1)
  {
    int docMin=0;
    SemP(semid, 0, 1);
    for(int i=1; i<NUMOFDOCTORS; i++)
    {
      SemP(semid, i, 1);
      if(doctors[docMin].numOfAppointments>doctors[i].numOfAppointments) docMin=i;
    }
    for(int i=0; i<NUMOFDOCTORS; i++) SemV(semid, i, 1);
    SemP(semid, docMin, 1);
    daySchedule *ds=doctors[docMin].begin->next;
    Tdate d=*date;
    while(ds!=NULL && done==0)
    {
      UpdateDate(&d, 1);
      if(d.weekDay!=6 && d.weekDay!=7)
      {
	for(int i=0; i<3; i++)
	{
	  if(ds->patientsAppointed[i]==NULL)
	  {
	    ds->patientsAppointed[i]=p;
	    //printf("!!! %s %i\n", ds->patientsAppointed[i]->name, ds->patientsAppointed[i]->id);
	    msgbuf msgOut;
	    msgOut.type=msg->id;
	    msgOut.subtype=9;
	    p->docNum=docMin;
	    p->date=d;
	    p->hour=ds->hours[i];
	    msgOut.date=p->date;
	    msgOut.id=p->hour;
	    strcpy(msgOut.name, doctors[docMin].name);
	    msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
	    done=1;
	    break;
	  }
	}
      }
      ds=ds->next;
    }
    SemV(semid, docMin, 1);
  }
  else
  {
    SemP(semid, p->docNum,1);
    msgbuf msgOut;
    msgOut.type=msg->id;
    msgOut.subtype=9;
    msgOut.date=p->date;
    msgOut.id=-p->hour;
    strcpy(msgOut.name, doctors[p->docNum].name);
    msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
    SemV(semid, p->docNum,1);
  }
}

void MakeAppointmentAfter2Months(msgbuf *msg)
{
  printf("Pacjent o pid %i prosi o rejestrację po 2 tygodniach ", msg->id);
  int done=0;
  patient *p=FindPatientById(msg->id);
  if(p->docNum==-1)
  {
    int docMin=0;
    SemP(semid, 0, 1);
    for(int i=1; i<NUMOFDOCTORS; i++)
    {
      SemP(semid, i, 1);
      if(doctors[docMin].numOfAppointments>doctors[i].numOfAppointments) docMin=i;
    }
    for(int i=0; i<NUMOFDOCTORS; i++) SemV(semid, i, 1);
    SemP(semid, docMin, 1);
    daySchedule *ds=doctors[docMin].begin->next;
    Tdate d=*date;
    for(int i=0; i<60;i++)
    {
      ds=ds->next;
    }
    UpdateDate(&d, 59);
    while(ds!=NULL && done==0)
    {
      UpdateDate(&d, 1);
      if(d.weekDay!=6 && d.weekDay!=7)
      {
	for(int i=0; i<3; i++)
	{
	  if(ds->patientsAppointed[i]==NULL)
	  {
	    ds->patientsAppointed[i]=p;
	    msgbuf msgOut;
	    msgOut.type=msg->id;
	    msgOut.subtype=10;
	    p->docNum=docMin;
	    p->date=d;
	    p->hour=ds->hours[i];
	    msgOut.date=p->date;
	    msgOut.id=p->hour;
	    strcpy(msgOut.name, doctors[docMin].name);
	    msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
	    done=1;
	    break;
	  }
	}
      }
      ds=ds->next;
    }
    SemV(semid, docMin, 1);
  }
  else
  {
    SemP(semid, p->docNum,1);
    msgbuf msgOut;
    msgOut.type=msg->id;
    msgOut.subtype=10;
    msgOut.date=p->date;
    msgOut.id=-p->hour;
    strcpy(msgOut.name, doctors[p->docNum].name);
    msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
    SemV(semid, p->docNum,1);
  }
}

void ChangeAppointmentDate(msgbuf *msg)
{
  printf("Pacjent o pid %i prosi o zmianę terminu wizyty ", msg->id);
  int done=0;
  patient *p=FindPatientById(msg->id);
  if(p->docNum!=-1)
  {
    SemP(semid, p->docNum, 1);
    daySchedule *ds=doctors[p->docNum].begin->next;
    Tdate d=*date;
    while(ds!=NULL && done==0)
    {
      //printf("czo");
      UpdateDate(&d, 1);
      if(d.weekDay!=6 && d.weekDay!=7)
      {
	PrintDate(&d);
	for(int i=0; i<3; i++)
	{
	  //printf("%i\n", ds->patientsAppointed[i]!=NULL && ds->patientsAppointed[i]==p);
	  if(ds->patientsAppointed[i]!=NULL && ds->patientsAppointed[i]==p)
	  {
	    //printf("Znaleziono ");
	    //PrintDate(&d);
	    msgbuf msgOut;
	    msgOut.type=msg->id;
	    msgOut.subtype=7;
	    msgOut.id=0;
	    msgOut.date=p->date;
	    strcpy(msgOut.name, doctors[p->docNum].name);
	    msgOut.id=p->hour;
	    msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
	    
	    daySchedule *ds2=ds->next;
	    while(ds2!=NULL && done==0)
	    {
	      UpdateDate(&d, 1);
	      if(d.weekDay!=6 && d.weekDay!=7)
	      {
		for(int i=0; i<3; i++)
		{
		  if(ds2->patientsAppointed[i]==NULL)
		  {
		    ds2->patientsAppointed[i]=p;
		    msgbuf msgOut;
		    msgOut.type=msg->id;
		    msgOut.subtype=7;
		    p->docNum=p->docNum;
		    p->date=d;
		    p->hour=ds2->hours[i];
		    msgOut.date=p->date;
		    msgOut.id=p->hour;
		    strcpy(msgOut.name, doctors[p->docNum].name);
		    msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
		    done=1;
		    break;
		  }
		}
	      }
	      ds2=ds2->next;
	    }
	    if(done!=0)
	    {
	      ds->patientsAppointed[i]=NULL;
	      break;
	    }
	    else
	    {
	      SendSimpleMessage(qid, p->id, 7, -2);
	      done=1;
	    }
	  }
	}
      }
      ds=ds->next;
    }
    SemV(semid, p->docNum, 1);
  }
  else
  {
    SendSimpleMessage(qid, msg->id, 7, -1);
  }
  if(done==0)SendSimpleMessage(qid, msg->id, 7, -1);
}

void CancelAppointment(msgbuf *msg)
{
  printf("Pacjent o pid %i prosi o anulowanie wizyty ", msg->id);
  int done=0;
  patient *p=FindPatientById(msg->id);
  int docSem=p->docNum;
  if(p->docNum!=-1)
  {
    SemP(semid, p->docNum, 1);
    daySchedule *ds=doctors[p->docNum].begin->next;
    Tdate d=*date;
    while(ds!=NULL && done==0)
    {
      //printf("czo");
      UpdateDate(&d, 1);
      if(d.weekDay!=6 && d.weekDay!=7)
      {
	PrintDate(&d);
	for(int i=0; i<3; i++)
	{
	  //printf("%i\n", ds->patientsAppointed[i]!=NULL && ds->patientsAppointed[i]==p);
	  if(ds->patientsAppointed[i]!=NULL && ds->patientsAppointed[i]==p)
	  {
	    //printf("Znaleziono ");
	    //PrintDate(&d);
	    ds->patientsAppointed[i]=NULL;
	    p->docNum=-1;
	    p->date.day=-1;
	    p->date.month=-1;
	    p->date.year=-1;
	    p->hour=-1;
	    done=1;
	    SendSimpleMessage(qid, msg->id, 8,0);
	    break;
	  }
	}
      }
      ds=ds->next;
    }
    SemV(semid, docSem, 1);
  }
  else
  {
    SendSimpleMessage(qid, msg->id, 8, -1);
  }
  if(done==0)SendSimpleMessage(qid, msg->id, 8, -1);
}

void LoginDoctor(msgbuf *msg)
{
  for(int i=0; i<NUMOFDOCTORS; i++)
  {
    if(msg->date.day==i && strcmp(msg->name, doctors[i].name)==0)
    {
      if(doctors[i].loggedIn!=0)SendSimpleMessage(qid, msg->id, 11, -1);
      else
      {
	doctors[i].loggedIn=1;
	SendSimpleMessage(qid, msg->id, 11, 0);
	printf("Dr %s zalogował się z pid %i\n", doctors[i].name, msg->id);
      }
      break;
    }
    if(i==(NUMOFDOCTORS-1))
    {
      SendSimpleMessage(qid, msg->id, 11, -2);
    }
  }
}

void LogoutDoctor(msgbuf *msg)
{
  for(int i=0; i<NUMOFDOCTORS; i++)
  {
    if(msg->date.day==i)
    {
      if(doctors[i].loggedIn==0) SendSimpleMessage(qid, msg->id, 11, -1);
      else
      {
	doctors[i].loggedIn=0;
	SendSimpleMessage(qid, msg->id, 11, 0);
	printf("Dr %s wylogował się z pid %i\n", doctors[i].name, msg->id);
      }
    }
  }
}

void GrantLeave(msgbuf *msg)
{
  int docNum=-1;
  
  for(int i=0; i<NUMOFDOCTORS; i++)
  {
    if(strcmp(doctors[i].name, msg->name)==0)
    {
      docNum=i;
      break;
    }
  }
  SemP(semid, docNum, 1);
  msgbuf msg2;
  msgrcv(qid, &msg2, MSGBUFSIZE, 1, 0);
  Tdate d=*date;
  //PrintDate(&msg->date);
  //PrintDate(&d);
  int b=DaysBetweenDates(&d, &msg->date);
  printf("%i\n", b);
  if(b<=0) SendDateMessage(qid, msg->id, 12, -1, &d);
  else if(b>180)
  { 
    UpdateDate(&d, 180);
    SendDateMessage(qid, msg->id, 12, -2, &d);
  }
  else
  {
    daySchedule *ds1=doctors[docNum].begin, *ds2;
    printf("Dr %s prosi o urlop między datami: ", doctors[docNum].name);
    Tdate d1=msg->date, d2=msg2.date;
    PrintDate(&d1);
    PrintDate(&d2);
    for(int i=0; i<b; i++) ds1=ds1->next;
    ds2=ds1;
    int b=DaysBetweenDates(&d1, &d2);
    for(int i=0; i<b; i++) ds2=ds2->next;
    while(ds1!=ds2->next)
    {
      for(int i=0; i<3; i++)
      {
	if(ds1->patientsAppointed[i]==NULL) ds1->patientsAppointed[i]=ghost;
	else
	{
	  doctors[i].numOfAppointments--;
	  patient *p=ds1->patientsAppointed[i];
	  int docMin;
	  for(int i=0; i<NUMOFDOCTORS; i++)
	  {
	    if(i!=docNum)
	    {
	      docMin=i;
	      break;
	    }
	  }
	  for(int i=0; i<NUMOFDOCTORS; i++)
	  {
	    if(i!=docNum)
	    {
	      SemP(semid, i, 1);
	      if(doctors[docMin].numOfAppointments>doctors[i].numOfAppointments) docMin=i;
	    }
	  }
	  for(int i=0; i<NUMOFDOCTORS; i++) if(i!=docNum) SemV(semid, i, 1);
	  SemP(semid, docMin, 1);
	  daySchedule *ds=doctors[docMin].begin->next;
	  Tdate d3=*date;
	  int done=0;
	  while(ds!=NULL && done==0)
	  {
	    UpdateDate(&d3, 1);
	    if(d3.weekDay!=6 && d3.weekDay!=7)
	    {
	      for(int i=0; i<3; i++)
	      {
		if(ds->patientsAppointed[i]==NULL)
		{
		  ds->patientsAppointed[i]=p;
		  //printf("!!! %s %i\n", ds->patientsAppointed[i]->name, ds->patientsAppointed[i]->id);
		  //msgbuf msgOut;
		  //msgOut.type=p->id;
		  //msgOut.subtype=9;
		  p->docNum=docMin;
		  p->date=d3;
		  p->hour=ds->hours[i];
		  //msgOut.date=p->date;
		  //msgOut.id=p->hour;
		  //strcpy(msgOut.name, doctors[docMin].name);
		  //msgsnd(qid, &msgOut, MSGBUFSIZE, 0);
		  done=1;
		  break;
		}
	      }
	    }
	    ds=ds->next;
	  }
	  SemV(semid, docMin, 1);
	  ds1->patientsAppointed[i]=ghost;
	}  
      }
      ds1=ds1->next;
    }
    SendSimpleMessage(qid, msg->id, 12, 0);
  }
  
  SemV(semid, docNum, 1);
}

void AskForConfirmation(patient *p)
{
  msgbuf msg;
  msg.type=2;
  msg.subtype=14;
  msg.date=p->date;
  msg.id=p->hour;
  strcpy(msg.name, doctors[p->docNum].name);
  msgsnd(qid, &msg, MSGBUFSIZE, 0);
}

void AdvanceTime()
{
  daySchedule *ds;
  UpdateDate(date, 1);
  printf("Zmieniła się data: ");
  PrintDate(date);
  for(int i=0; i<NUMOFDOCTORS; i++)
  {
    ds=doctors[i].begin;
    doctors[i].begin=doctors[i].begin->next;
    free(ds);
    ds=doctors[i].begin;
    AddDaySchedule(doctors[i].end);
    for(int j=0; j<3; j++)
    {
      //printf("sprawdzam %i, %i, %i\n", i, j, ds->patientsAppointed[j]!=NULL);
      if(ds->patientsAppointed[j]!=NULL) printf("%s zostaje przyjęty o %i:00 przez %s\n", ds->patientsAppointed[j]->name, ds->patientsAppointed[j]->hour, doctors[i].name);
    }
    for(int j=0; j<14; j++) ds=ds->next;
    for(int j=0; j<3; j++)
    {
      //printf("sprawdzam %i, %i, %i\n", i, j, ds->patientsAppointed[j]!=NULL);
      if(ds->patientsAppointed[j]!=NULL) AskForConfirmation(ds->patientsAppointed[j]);
    }
  }
}

int main()
{
  Init();
  struct msgbuf msg;
  //PrintDoctors();
  if(fork()==0)
  {
    prctl(PR_SET_PDEATHSIG, SIGQUIT);
    while(1)
    {
      sleep(5);
      for(int i=0; i<NUMOFDOCTORS; i++) SemP(semid, i, 1);
      SendSimpleMessage(qid, 1, 13, 0);
      for(int i=0; i<NUMOFDOCTORS; i++) SemV(semid, i, 1);
    }
  }
  else
  {
    signal(SIGINT, Close);
    while(1)
    {
      msgrcv(qid, &msg, MSGBUFSIZE, 1, 0);
      printf("Nowa wiadomosc - %s : %i, %i\n", msg.name, msg.id, msg.subtype);
      switch(msg.subtype)
      {
	case 1:
	  if(msg.id<0) LogoutPatient(-msg.id); 
	  else LoginPatient(&msg);
	  PrintPatients();
	  break;
	case 2:
	  SendDoctorsOnGivenDay(&msg);
	  break;
	case 3:
	  SendAvailableHoursOnGivenDay(&msg);
	  break;
	case 4:
	  SendDoctorsToChoose(&msg);
	  break;
	case 5:
	  SendAvailableHoursOfDoctor(&msg);
	  break;
	case 6:
	  SendAppointmentStatus(&msg);
	  break;
	case 7:
	  ChangeAppointmentDate(&msg);
	  break;
	case 8:
	  CancelAppointment(&msg);
	  break;
	case 9:
	  MakeAppointmentIn2Months(&msg);
	  break;
	case 10:
	  MakeAppointmentAfter2Months(&msg);
	  break;
	case 11:
	  if(msg.id>0) LoginDoctor(&msg);
	  else
	  {
	    msg.id=-msg.id;
	    LogoutDoctor(&msg);
	  }
	  break;
	case 12:
	  GrantLeave(&msg);
	  break;
	case 13:
	  AdvanceTime();
	  break;
      }
    }
  }
  return 0;
}

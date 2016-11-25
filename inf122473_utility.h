#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED
#define MSGBUFSIZE sizeof(msgbuf)-sizeof(long)
#define MSGBUF2SIZE sizeof(msgbuf2)-sizeof(long)

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>

typedef struct Tdate
{
  int day;
  int month;
  int year;
  int weekDay;
}Tdate;

typedef struct msgbuf
{
  long type;
  int subtype;
  int id;
  char name[60];
  char pesel[11];
  Tdate date;
}msgbuf;

typedef struct msgbuf2
{
  long type;
  int hours[90];
  short appointments[30];
}msgbuf2;

typedef struct patient
{
  int id;
  char name[60];
  char pesel[11];
  short loggedIn;
  int docNum;
  Tdate date;
  int hour;
}patient;

typedef struct daySchedule daySchedule;

struct daySchedule
{
  int hours[3];
  patient* patientsAppointed[3];
  daySchedule* next;
};

typedef struct doctor
{
  char name[60];
  int numOfAppointments;
  int loggedIn;
  daySchedule *begin, *end;
}doctor;



ssize_t readline(char **lineptr, FILE *stream);
int clean_stdin();
void SendSimpleMessage(int qid, long type, int subtype, int id);
void SendDateMessage(int qid, long type, int subtype, int id, Tdate *d);
void AddDaySchedule(daySchedule* ds);
void UpdateDate(Tdate *d, int i);
void PrintDate(Tdate *d);
void GetDayOfWeek(Tdate *d);
int DaysBetweenDates(Tdate *d1, Tdate *d2);
void SemP(int semid, int semnum, int value);
void SemV(int semid, int semnum, int value);
#endif // UTILITY_H_INCLUDED

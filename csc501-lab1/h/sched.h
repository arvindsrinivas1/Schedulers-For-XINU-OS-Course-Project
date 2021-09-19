#ifndef SCHEDULING_DOT_H
#define SCHEDULING_DOT_H


#define EXPDISTSCHED 1
#define LINUXSCHED 2

extern int goodness_value[]; // calc at beg of resched - 
extern int counter[];  //decreement for every tick. Shows current remaining quantum slice for every proc.
extern int quantum[];  // calc at the begining of an epoch and is constant until the end of that epoch
extern int eligible[]; // dont schedule a newly created process in the current epoch. eligible[i]=0;
extern int priorty_in_current_epoch[];//set at the begining of an epoch
extern int schedule_class;


int calc_goodness_value();
int calc_quantum();



//struct goodness_struct{
//	int gnext;
//	int gv;
//	int gprev;
//}
//extern struct goodness_struct goodness_value[];

int getschedclass();
void setschedclass (int sched_class);

#endif

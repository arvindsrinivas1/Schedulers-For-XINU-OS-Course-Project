#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <sched.h>

int schedule_class = 322;

void setschedclass(int sched_class){
	schedule_class = sched_class;
}

int getschedclass(){
	return(schedule_class);
}



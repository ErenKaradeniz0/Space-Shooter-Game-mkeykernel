#ifndef _CLOCK_DRIVER_
#define _CLOCK_DRIVER_
int _CLOCK_HOUR;
int _CLOCK_MINUTES;
int _CLOCK_SEC;
int _CLOCK_MSEC;
int _CLOCK_COUNTER;

extern int get_counter_asm(); 

void get_clock(){
	_CLOCK_COUNTER  = get_counter_asm();
	_CLOCK_HOUR = _CLOCK_COUNTER / 65543;
    int Remainder = _CLOCK_COUNTER % 65543;
  	_CLOCK_MINUTES   = Remainder / 1092;
    Remainder = Remainder % 1092;
 	_CLOCK_SEC = (Remainder*100) / 1821;
  	_CLOCK_MSEC = ((Remainder*100) % 1821)*10;
}

void timer(int msec){
	get_clock();
	int acumulado = 0;
	int last_counter = _CLOCK_COUNTER;
	do{
		get_clock();
		if(_CLOCK_COUNTER != last_counter){
			acumulado += 5400;
			last_counter = _CLOCK_COUNTER;
		}
	}while(acumulado <= (msec*1000));
}
#endif

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <vector>

double Time_lim = 0;
std::vector <double> gist = {0};

using namespace std;
int MAX_LENGHT = 5;
int EMPTY = -1;
FILE* f_log;
const char file_name[] = "log.txt";
double Time_global = 0.0, Time_end = 3600.0;

void start_log()
{ 
	f_log = fopen(file_name, "w");// "w" means that we are going to write on this file
}

struct Transact {
	int id;							// номер транзакци
	double time_start_service;		// момент времени появления
	double time_end_service;		// момент времени конца обслуживания
};

class Queue {
	private:
		int id;
		int max_lenght;		// макс. длина очереди (расширяемо)
		int lenght;		// длина очереди (EMPTY = касса свободна)
		Transact** trans_queue;	// двумерный массив указателей на очереди из транзакций
	public:
		Queue (int i) : id(i) {
			trans_queue = new Transact* [MAX_LENGHT]; // создаем массив из указателей
			lenght = EMPTY;
			max_lenght = MAX_LENGHT;
      }
		int get_lenght() {return lenght;}; // получить длину очереди
		void extend ();			// расширение территории
		Transact* trans_from_queue_to_device ();		// выпихиваем из очереди в устройство
		void add (Transact*);	// добавить в очередь
		~Queue();
};

void Queue::extend () { // расширение очереди на 1
	Transact** q_transactions = new Transact* [++max_lenght]; // создаем новую очередь на 1 длиннее прошлой
	for (int i = 0; i < lenght; i++) // цикл по всему массиву
		q_transactions[i] = trans_queue[i]; // переписываем прошлый список в новый
	delete[] trans_queue; // удалем прошлую очередь
	trans_queue = q_transactions; // сохраняем в класс новую
}

Transact* Queue::trans_from_queue_to_device () { // добавить в очередь новый элемент
	if (lenght > 0) { // если очередь не пустая
		Transact* trans = trans_queue[0]; // запоминаем первую очередь транзакций
		if (lenght > 1) { 
			for (int i = 0; i<lenght-1; i++) 
				trans_queue[i] = trans_queue[i+1]; // переписываем 
		}
    lenght--;
    gist[lenght] += Time_global - Time_lim;
    Time_lim = Time_global;
    // кол-во заявок всегда больше длины очереди на 1, поэтому запись идет в gist[lenght], а не gist[lenght+1]
		return trans; // возвращаем первый список транзакций
	} else {
		lenght--;
		return NULL;
	}
}

void Queue::add (Transact* trans) {
	if (trans != NULL) { // проверяем, что транзакция не пустая
		if (trans_queue[0] == NULL) { // если нет элементов - добавляем в первый
			trans_queue[0] = trans;
      gist[0] += Time_global - Time_lim;
      Time_lim = Time_global;
		} else {
			trans_queue[lenght] = trans; // добавляем в конец
		}
    lenght++;
    if(gist.size()<lenght) gist.push_back(0);
    gist[lenght-1] += Time_global - Time_lim;
    Time_lim = Time_global;
		if (lenght+1 == max_lenght)  // если очередь заполнена - расширяем кол-во элементов
			extend(); // расширяем размер списка на 1
	} else 
	  lenght++; //выполняется при очереди == EMPTY
}

Queue::~Queue (void) { // удаление очереди
	if (lenght > 0) { // если очередь не пустая
		for (int i = 0; i < lenght; i++) { // цикл по всей длине очереди
			string text;
			fprintf(f_log, "%.3f %s %d %d\n", Time_end, "не был обслужен", trans_queue[i]->id, id);
			delete trans_queue[i]; // удаляем все
		}
	}
	delete [] trans_queue; 
}


class Device {
	private:
		int id;			// номер кассы
		double down_limit_time;		// нижняя граница диапазона обслуживания
		double up_limit_time;		// верхняя граница диапазона обслуживания
		Queue trans_queue;			// список транзакций
		Transact* current_trans;    // текущая транзакция
	public:
		Device (int i, double tl, double th) : down_limit_time(tl), up_limit_time(th), id(i), trans_queue(i) {};
    void Output();
		void puch_to_queue (Transact*);	// запихиваем в очередь
		void release ();			// выпускаем с устройства
		void set_time_end ();		// даём время на обслуживание
		double get_time_end_service();  // получить время
		int get_lenght () {return trans_queue.get_lenght();};
		~Device();
};

void Device::puch_to_queue (Transact* trans) {
	fprintf(f_log, "В момент времени %.3f транзакт с идентификатором %d встал в очередь %d\n", Time_global, trans->id, id);
	if (trans_queue.get_lenght() != EMPTY) { // если очередь пустая
		trans_queue.add(trans);// добавить транзакцию
	} else {
		trans_queue.add(NULL); // добавить NULL
		current_trans = trans; 
	}
}

void Device::release () {
	fprintf(f_log, "В момент времени %.3f транзакт с идентификатором %d освободил устройство %d\n", Time_global, current_trans->id, id);
  fprintf(f_log, "В момент времени %.3f транзакт с идентификатором %d вышел из модели\n", Time_global, current_trans->id);
	delete current_trans; // удаляем текущую транзакцию
	current_trans = trans_queue.trans_from_queue_to_device(); // текущая очередь транзакций
}

void Device::set_time_end () {  // 
	srand(time(0)+current_trans->id); // задаем стартовое значение для рандома
	int delta = (up_limit_time - down_limit_time) * 1000;
	double rand_number = (rand() % delta) / 1000.0;
	current_trans->time_end_service = Time_global + rand_number + down_limit_time;
	fprintf(f_log, "В момент времени %.3f транзакт с идентификатором %d занял устройство %d\n", Time_global, current_trans->id, id);
}

double Device::get_time_end_service() { 
	if (current_trans != NULL)
		return current_trans->time_end_service;
	else
		return 0.0;
}

Device::~Device (void) { // деструктор
	if (current_trans != NULL) { 
		fprintf(f_log, "%.3f %s %d %d\n", Time_end, "обслуживание не закончено", current_trans->id, id);
		delete current_trans;
	}
}

void Device::Output()
  {
  for(int i=0;i<gist.size();i++)
    cout<<"Очередь из "<<i<<" заявок была "<<gist[i]<<" секунд"<<endl;
  }

int main () {
	setlocale(LC_CTYPE, "rus");
	srand(time(0));
	Device device_1(1, 8.0 , 27.0); // создаем два новых устройства
	Device device_2(2, 9.0, 27.0);
	double tl = 0.0, th = 17.0; // время начало и конца
	int id_count = 1; // счетчик транзакций
	start_log();

	Transact* new_trans = new Transact { id_count,
  Time_global + ((rand()%(int)((th-tl)*1000)) / 1000.0 + tl), 0 }; // создаем транзакцию, присваиваем номер
  fprintf(f_log, "В момент времени %.3f транзакт с идентификатором %d вошел в модель\n", Time_global, new_trans->id);
	device_1.puch_to_queue(new_trans); // добавляем к первому устройству в очередь новую транзакцию
	device_1.set_time_end();
	new_trans = new Transact { // создаем новую транзакцию
		++id_count,  // увеличиваем счетчик транзакций
		((rand()%(int)((th-tl)*1000)) / 1000.0 + tl), // время начало = ранд(0, 18000) / 1000 + tl
		0
	};
	
	while ((Time_global <= Time_end) && // пока время не закончилось
		!((new_trans->time_start_service > Time_end) && // И время начала новой транзакции меньше времени конца
		(device_1.get_time_end_service() > Time_end || device_1.get_time_end_service() < 0.0001) && // И время обслуживание первого устройства больше времени конца или меньше 0.0001
		(device_2.get_time_end_service() > Time_end || device_2.get_time_end_service() < 0.0001))) { // И время обслуживание второго устройства больше времени конца или меньше 0.0001
	
		if ((new_trans->time_start_service < device_1.get_time_end_service() || 
			device_1.get_time_end_service() < 0.0001) 
		 && (new_trans->time_start_service < device_2.get_time_end_service() || 
			 device_2.get_time_end_service() < 0.0001)) {
		 	/*
			 // (время начала транзакции меньше времени окончания обслуживания первого устройства ИЛИ 
			время конца обслуживания первого устройстваменьше 0.0001.) И
			// (время начала транзакции меньше времени окончания обслуживания второго устройства ИЛИ 
			время конца обслуживания второго устройства меньше 0.0001.) И
			*/

			Time_global = new_trans->time_start_service; // глобальное время равно времени создания транзакции
			fprintf(f_log, "В момент времени %.3f транзакт с идентификатором %d вошел в модель\n", Time_global, new_trans->id);
			if (device_1.get_lenght() <= device_2.get_lenght()) { // если у первого устройства очередь меньше, то
				device_1.puch_to_queue(new_trans); // добавляем новую транзакцию к нему
				if (device_1.get_lenght() == 0) 
					device_1.set_time_end();    // установить время окончания, если очередь пустая
			} else {
				device_2.puch_to_queue(new_trans); // иначе ко второму
				if (device_2.get_lenght() == 0)
					device_2.set_time_end();	// установить время окончания, если очередь пустая
			}
			
			new_trans = new Transact { // создаем новую транзакцию
				++id_count, 
				(Time_global + ((rand()%(int)((th-tl)*1000)) / 1000.0 + tl)), // время начала новой транзакции - это глобальное, плюс рандом
				0
			};
		} else {
			if ((device_1.get_time_end_service() < device_2.get_time_end_service() && 
				 device_1.get_time_end_service() > 0.0) || 
				(device_2.get_time_end_service() < 0.0001 && 
				 device_1.get_time_end_service() > 0.0)) {
				/*
				время окончания обслуживания первого устройства: [0, времени окончания обслуживания второго]
				ИЛИ 
				время окончания обслуживания второго устройства: [0, времени окончания обслуживания первого]
				*/
				Time_global = device_1.get_time_end_service(); // устанавливаем глобальное время как время окончания обслуживания первого устройства
				
				device_1.release(); // печатаем данные и удаляем транзакцию
				if (device_1.get_lenght() > EMPTY) 
					device_1.set_time_end(); // устанавливаем время окончания обслуживания первого устройства если очередь не пустая
			} else 
				// если времена обслуживания не попали в интервалы между 0 и временем обслуживания другого устройства
				if ((device_2.get_time_end_service() > 0.0 && 
					device_2.get_time_end_service() < device_1.get_time_end_service())
					|| (device_1.get_time_end_service() < 0.0001 && 
						device_2.get_time_end_service() > 0.0)) {
				/*
				время окончания обслуживания второго устройства: [0, времени окончания обслуживания первого]
				ИЛИ 
					время окончания обслуживания первого устройства меньше 0.0001 
					И
					время окончания обслуживания второго больше 0
				*/
				Time_global = device_2.get_time_end_service(); // глобальное время = время окончания обслуживания второго
				
				device_2.release(); // печатаем данные и удаляем транзакцию
				if (device_2.get_lenght() > EMPTY) 
					device_2.set_time_end(); /// если есть элементы в очереди, то устанавливаем время окончания обслуживания
			}
		}
	}
	fclose(f_log);
  device_1.Output();
  return 0;
}
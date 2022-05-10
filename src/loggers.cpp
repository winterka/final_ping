#include "loggers.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include<time.h>
#include <errno.h>
#include <cstring>

//Глобальные переменные 
int MAX_SIZE = 30000;
std::ofstream str;//переменная для создание и открытия файла
char  fileName[2048];//Адрес текщего файла лога
char Dirname[1024]; //Адрес директории лога
int errorCode;
////////////////////////////////////////////////////////
void addressLog();// Определение адреса директории логов
char * getCurrentDateTime();// Определение даты и времени создания файла лога
char * getTime();// Определение даты и времени
int createLog(); // Создание файла лога
int createdirlog();// Создание директории лога
int deleteLogFile();// Удаление старого файла лога при переполнении
int getFileSize();// Определение размера директории
int AddMessageToLog(const char * message);// Ввод сообщение в лог
void DiagLog();// Ошибки лога
void Diag();//Ошибки программы
////////////////////////////////////////////////////////

int main_func()
{
	setlocale(LC_ALL, "Rus");
	addressLog();
	switch(createLog())
	{
		case -1: DiagLog();break;
	}
	//AddMessageToLog("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII");
	return 0;
}
///////////////////////////////////////////////////////
void addressLog()// Определение адреса директории логов
{
	sprintf(Dirname, "/home/%s/Desktop/PING", getenv("USER"));
}
char * getTime()
{
	char * tim;
	time_t tt=time(nullptr);
	tim=ctime(&tt);
	tim[strlen(tim)-1] = '\0';
	return tim;

}
char * getCurrentDateTime()// Определение даты и времени создания файла лога
{
	tm *t;
	char *datetime = new char[512];
	time_t tt=time(nullptr);
	t=localtime(&tt);
	sprintf(datetime, "%d", 1900+t->tm_year);
	if((t->tm_mon)/10<=0)
		sprintf(datetime, "%s_0%d",datetime, t->tm_mon);
	else 
		sprintf(datetime, "%s_%d",datetime, t->tm_mon);

	if((t->tm_mday)/10<=0)
		sprintf(datetime, "%s_0%d",datetime, t->tm_mday);
	else 
		sprintf(datetime, "%s_%d",datetime, t->tm_mday);

	if((t->tm_hour)/10<=0)
		sprintf(datetime, "%s_0%d",datetime, t->tm_hour);
	else 
		sprintf(datetime, "%s_%d",datetime, t->tm_hour);

	if((t->tm_min)/10<=0)
		sprintf(datetime, "%s_0%d",datetime, t->tm_min);
	else 
		sprintf(datetime, "%s_%d",datetime, t->tm_min);

	return datetime;
}

int createLog()// Создание файла лога
{
	sprintf(fileName,"%s/log%s.txt", Dirname, getCurrentDateTime());
	if(deleteLogFile()!=0)
		return -1;
	str.open(fileName);
	if (!str.is_open())// если не удалось открыть файл
	{
        printf("Errr");
		return -1;
	}
	str<<"["<<getTime()<<"]"<<" "<<"Log file is created."<<"\n";
	str.close();
	return 0;
}

int createdirlog()//Создание директории для лога
{
	char path_buff[2048];
	sprintf(path_buff, "mkdir -p %s", Dirname);
	if(system(path_buff) == 0)//Создание директории для лога 
		return 0;
	return -1;//если не удалось создать директорию лога
}

int deleteLogFile()// Удаление старого файла лога при переполнении
{
	int filesize=getFileSize();
	if (filesize==-1)
	{
		return -1;
	}
	if (getFileSize()>=MAX_SIZE)
	{
		char comant[2048];
		sprintf(comant,"find %s -type f -printf '%%T+ %%p\\n' | sort | head -n1 | awk '{print $2}' | xargs rm -v", Dirname);
		if(system(comant)!=0)//Удаление самого старого лога
			return -1;
		return 0;
	}
	return 0;
}

int getFileSize()// Определение размера директории
{
	DIR * directory;
	struct dirent *dir;
	struct stat buf;
	int exists;
	int total_size=0;
	char * address = new char[512];
	directory = opendir(Dirname);
	if (directory == NULL) // если дериктории не сущесвует
	{
		DiagLog();
		if(createdirlog()==0)
			return 0;
		else
			return -1;
	}
	for (dir = readdir(directory); dir != NULL; dir = readdir(directory))
	{
		sprintf(address,"%s/%s", Dirname, dir->d_name);
	    exists = stat(address, &buf);

		if (exists < 0) 
		{
			DiagLog();
		} 
		else 
		{
			total_size += buf.st_size;
		}
	}
	closedir(directory);
	return total_size;
}

int AddMessageToLog(const char * message)// Ввод сообщение в лог
{
	if(deleteLogFile()!=0)
		return -1;
	FILE * file;
	str.open(fileName, std::ios_base::app);
	if (!str.good())// если не удалось открыть файл
	{
		return -1;
	}
	str<<"["<<getTime()<<"] "<<message<<"\n";
	str.close();
	return 0;
}

void DiagLog()
{
	char * errorText = new char [128];
	switch(errno)
	{
		case E2BIG:
		{
			std::cout<<"Error: Input conversion stopped due to lack of space in the output buffer"<<std::endl;
			break;
		}
		case EROFS:
		{
			std::cout<<"Error: The file system is read-only"<<std::endl;
			break;
		}
		case EBADF:
		{
			std::cout<<"Error: Bad file descriptor."<<std::endl;
			break;	
		}
		case EBUSY:
		{
			std::cout<<"Error: The device or resource is busy"<<std::endl;
			break;
		}
		case EEXIST:
		{
			std::cout<<"Error: The file exists"<<std::endl;
			break;
		}
		case EXDEV:
		{
			std::cout<<"Error: Invalid communication between devices"<<std::endl;
			break;
		}
		case EMFILE:
		{
			std::cout<<" There are too many open files"<<std::endl;
			break;	
		}
		case ENFILE:
		{
			std::cout<<"Error: There are too many open files in the system"<<std::endl;
			break;
		}
		case ETXTBSY:
		{
			std::cout<<"Error: The text file is busy"<<std::endl;
			break;			
		}
		case EFBIG:
		{
			std::cout<<"Error: The file is too large"<<std::endl;
			break;
		}
		case ENOSPC:
		{
			std::cout<<"Error: There is no free space left on the device"<<std::endl;
			break;
		}
		case EILSEQ:
		{
			std::cout<<"Error: Input conversion stopped due to an input byte that does not belong to the input codeset"<<std::endl;
			break;
		}
		case EINVAL:
		{
			std::cout<<"Error: Input conversion stopped due to an incomplete character or shift sequence at the end of the input buffer"<<std::endl;
			break;
		}
		case EPERM:
		{
			std::cout<<"Error: Operation not permitted"<<std::endl;
			break;
		}
		case ENOENT:
		{
			std::cout<<"Error: this file or directory does not exist"<<std::endl;
			break;
		}
		case EACCES:
		{
			std::cout<<"Error: Permission denied"<<std::endl;
			break;
		}
		case ENOMEM:
		{
			std::cout<<"Error: Out of memory"<<std::endl;
			break;
		}
		case EOVERFLOW:
		{
			std::cout<<"Error: Value too large for defined data type"<<std::endl;
			break;
		}
		default:
		{
			std::cout<<" Unknown Error"<<std::endl;
			break;
		}
	}
}

void Diag()
{
	char logText[1024];
	switch(errorCode)
	{
		case 1:
			sprintf(logText,"Error: Wrong number of input parameters"); break;
		case 2:
			sprintf(logText,"Error: Domain doesn't match to any IP address"); break;
		case 3:
			sprintf(logText,"Error: Wrong initialization socket"); break;
		case 4:
			sprintf(logText,"Error: Wrong sending packet"); break;
		case 5:
			sprintf(logText,"Error: The number of sent parts of the package does not match the number of received"); break;
		case 6:
			sprintf(logText,"Error: The amount of the received packet does not match the sent");break;
		default:
			sprintf(logText, "Unknown Error"); break;
	}
	std::cout<<logText<<std::endl;
	switch(AddMessageToLog(logText))
	{
		case 0:
			break;
		case -1:
			DiagLog();break;
	}
}

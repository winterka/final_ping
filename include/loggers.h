#ifdef LOGGERS_H
#define LOGGERS_H

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

int logFunc();
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
int errorCode;

#endif


/*
  Koi - a GIMP image authentication plugin
	Copyright (C) 2010  ben howard

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LOG
#define LOG

FILE *log_file;

#include "Koi.h"

void print_log(const char *format, ...)
{
	va_list var_args;

//	printf("doing va start\n");
	va_start(var_args, format);
//		printf("doing vprintf on %d file\n",log_file);
	vfprintf(log_file, format, var_args);
//		printf("doing va end\n");
	va_end(var_args);
}

int open_log(char *file_name)
{
	//first need to append .txt or something to the end of the file name
	//then open the file and write the text, close the file
	char log_name[256];

	sprintf(log_name,"%s.log",file_name);

//	printf("log_name %s\n",log_name);

	log_file = fopen(log_name, "a");

	if(log_file == NULL)
	{
		printf("failed to open %s\n",log_name);
		return 0;
	}
	else
	{
		fprintf(log_file,"test\n");
		return 1;
	}

}
void close_log()
{
	fclose(log_file);
	log_file = 0;
}

#endif // LOG

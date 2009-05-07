#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

int main(void)
{	
	time_t now;
	struct tm* utc_info;
	struct tm* tz_info;
	int utc_day;
	int utc_hour;
	int utc_minute;
	int tz_day;
	int tz_hour;
	int tz_minute;
	int minuteswest;

	struct timeval tv;
	struct timezone old_tz;
	struct timezone new_tz;

	time(&now);
	utc_info = gmtime(&now);
	utc_day = utc_info->tm_mday;
	utc_hour = utc_info->tm_hour;
	utc_minute = utc_info->tm_min;
	tz_info = localtime(&now);
	tz_day = tz_info->tm_mday;
	tz_hour = tz_info->tm_hour;
	tz_minute = tz_info->tm_min;

	utc_day = utc_day < tz_day  - 1 ? tz_day  + 1 : utc_day;
	tz_day =  tz_day  < utc_day - 1 ? utc_day + 1 : tz_day;
	
	minuteswest = (24*60*utc_day + 60*utc_hour + utc_minute) - (24*60*tz_day + 60*tz_hour + tz_minute) ;
	new_tz.tz_minuteswest = minuteswest;
	new_tz.tz_dsttime = 0;

	/* Get tv to pass to settimeofday(2) to be sure we avoid hour-sized warp */
	/* (see gettimeofday(2) man page, or /usr/src/linux/kernel/time.c) */
	if (gettimeofday(&tv, &old_tz))
	{
		perror("gettimeofday");
		exit(EXIT_FAILURE);
	}
	if (settimeofday(&tv, &new_tz))
	{
		perror("settimeofday");
		exit(EXIT_FAILURE);
	}
	
	/*
	printf("utc hours = %d\n", utc_hour);
	printf("tz hours = %d\n", tz_hour);
	printf("observed minuteswest: %d\n", minuteswest);
	printf("Old minuteswest: %d\n", old_tz.tz_minuteswest);
	printf("Old dsttime: %d\n", old_tz.tz_dsttime);
	printf("New minuteswest: %d\n", new_tz.tz_minuteswest);
	printf("New dsttime: %d\n", new_tz.tz_dsttime);
	*/

	return EXIT_SUCCESS;
}

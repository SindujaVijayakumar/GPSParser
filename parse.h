#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define pi 3.14159265358979323846

typedef struct tm
{
	int tm_sec;   // seconds after the minute - [0, 60] including leap second
	int tm_min;   // minutes after the hour - [0, 59]
	int tm_hour;  // hours since midnight - [0, 23]
	int tm_mday;  // day of the month - [1, 31]
	int tm_mon;   // months since January - [0, 11]
	int tm_year;  // years since 1900
	int tm_wday;  // days since Sunday - [0, 6]
	int tm_yday;  // days since January 1 - [0, 365]
	int tm_isdst; // daylight savings time flag
}tm_t;

typedef struct coordinates {
	double min;
	int deg;
	char dir;
} coord_t;

typedef struct RMC_fields{
	int i;

}RMC_t;

typedef struct GGA_fields{
	tm_t timestamp;
	coord_t lat;
	coord_t lon;
	int fix;
	int numSat;
	float hod;
	float alt;
	char altRef;
	float geoHt;
	char geoRef;
	int lastUpdateTime_s;
	int ID;

}GGA_t;

typedef struct VTG_fields{
	int i;
}VTG_t;

typedef enum msg_type{
  RMC,
  VTG,
  GGA,
  UNSUPPORTED = -1
  
}msg_type_t;

typedef enum log_parse_state {
LOGP_NOINIT,
LOGP_OPENED,
LOGP_CLOSE,
LOGP_FAILED
}log_parse_state_t;

typedef enum msg_parser_state{
MSG_VALID,
MSG_SPLIT,
MSG_TYPE,
MSG_DATA,
MSG_CHSUM,
MSG_UNSUPPORTED,
MSG_FAILED,
MSG_DISCARD
}msg_parser_t;

typedef struct pvt_buffer
{
	tm_t timestamp;
	coord_t lat;
	coord_t lon;

}pvt_buffer_t;

typedef enum buffer_status {
	BUFF_EMPTY = 0,
	BUFF_INUSE = 1,
	BUFF_FULL = 2
}buffer_status_t;


uint8_t process_log(const FILE* log_file);
uint8_t process_msg(const char* msg);
bool is_valid_msg(const char* msg, size_t length);
int get_msg_type(const char* msg);
GGA_t parse_GGA(const char** msg_parts);
buffer_status_t update_speed_timestamp(const GGA_t dataGGA);
double get_distance_km(const coord_t latA, const coord_t latB, const coord_t lonA, const coord_t lonB);
double get_time_sec(const tm_t timestampA, const tm_t timestampB);
double get_speed_kmph(const float distance_km, const float time_sec);
void write_data(const double speed_kmph, const tm_t timestamp);
char** get_msg_parts(const char* msg, size_t length);
double deg2rad(const double);
double rad2deg(const double);
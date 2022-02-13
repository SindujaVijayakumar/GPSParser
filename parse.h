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

typedef struct fieldsRMC_s{
	int i;

}RMC_t;

typedef struct fieldsGGA_s{
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

typedef struct fieldsVTG_s{
	int i;
}VTG_t;

typedef enum msgType{
  RMC,
  VTG,
  GGA,
  UNSUPPORTED = -1
  
}msgType_t;

typedef enum logParserState {
LOGP_NOINIT,
LOGP_OPENED,
LOGP_CLOSE,
LOGP_FAILED
}logParse;

typedef enum msgParserState{
MSG_VALID,
MSG_SPLIT,
MSG_TYPE,
MSG_DATA,
MSG_CHSUM,
MSG_UNSUPPORTED,
MSG_FAILED,
MSG_DISCARD
}msgParse;

typedef struct pvtBuffer
{
	tm_t timestamp;
	coord_t lat;
	coord_t lon;

}pvtBuf_t;

typedef enum buffStatus {
	BUFF_EMPTY = 0,
	BUFF_INUSE = 1,
	BUFF_FULL = 2
}buffStatus_t;


int processLog(FILE *logFile);
int processMsg(const char* msg);
bool isValidMsg(const char* msg, int length);
int getMsgType(const char* msg, int length);
RMC_t parseMsgRMC(const char* msg);
GGA_t parseMsgGGA(const char** msgParts);
VTG_t parseMsgVTG(const char*msg);
int updatePVTBuffer(GGA_t dataGGA);
double getDistanceKM(coord_t latA, coord_t latB, coord_t lonA, coord_t lonB);
double getTimeSec(tm_t timestampA, tm_t timestampB);
double getSpeedKMPH(float distanceKm, float tSec);
int updateSpeedTime(float speed, float tStamp);
int writeSpeedTimetoFile(float speed, float tStamp);
bool isValidStartChar(const char* msg);
char** getMsgParts(const char* msg, int length);
double deg2rad(double);
double rad2deg(double);
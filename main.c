#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "parse.h"

#define ENABLE 1
#define DISABLE 0
#define DEBUG ENABLE
#define MAX_MSGS INT_MAX
#define NMEA_MSG_LEN 82

const char debugFile[] = "./nmea_sample";





bool isValidMsg(const char* msg, int length)
{
  return true;
    
}


char** splitMsgByComma(const char* msg, int length)
{
  int pos = 0;
  char **tokens = NULL;
  int lenToken = 0;
  int numToken = 0;
  int prevCommaPos = 0;
  char* temp = (char*)(malloc(sizeof(char) * (length)));
  strcpy(temp, msg);

  tokens = (char**)malloc(sizeof(char) * 90);
  for(pos=0; pos<length; pos++)
  {
    if(temp[pos] == ',' || temp[pos] == '*' || temp[pos] == '\n')
    {
        temp[pos] = '\0';
        if(pos > 5 && temp[pos-1] == ',')
        {
            tokens[numToken] = (char*)malloc(sizeof(char) * 2);
            strncpy(tokens[numToken], " \0", 2);
            
        
        }
        else
        {
            lenToken = pos - prevCommaPos;
            tokens[numToken] = (char*)malloc(sizeof(char) * (lenToken));
            strncpy(tokens[numToken], temp + prevCommaPos + 1, lenToken);
            //strcat(tokens[numToken], '\0');
      
        }
        printf("\ntokens[%d]=%s", numToken, tokens[numToken]);
        // free(tokens[numToken]);
        numToken++;
        lenToken = 0;
        prevCommaPos = pos;
      
    } 
  }
  return tokens;
    
}

msgType_t getMsgType(const char* token)
{
    msgType_t type = (msgType_t)UNSUPPORTED;
    // Skip first 2 characters 
    token++;
    token++;
    if (token)
    {
        if (strcmp("VTG", token) == 0)
            type = (msgType_t)VTG;
        else if (strcmp("GGA", token) == 0)
            type = (msgType_t)GGA;
        else if (strcmp("RMC", token) == 0)
            type = (msgType_t)RMC;

    }
    return type;

}

char** getMsgParts(const char* msg, int length)
{
  
  int numData = 0;
  char** tokens = splitMsgByComma(msg, length);
  return tokens;
}

int time_parse(char* s, struct tm* time)
{
    char* rv;
    unsigned int x;

    if (s == NULL || *s == '\0') {
        return -1;
    }

    x = strtoul(s, &rv, 10);
    time->tm_hour = x / 10000;
    time->tm_min = (x % 10000) / 100;
    time->tm_sec = x % 100;
    if (time->tm_hour > 23 || time->tm_min > 59 || time->tm_sec > 59 || (int)(rv - s) < 6) {
        return -1;
    }
    if (*rv == '.') {
        /* TODO There is a sub-second field. */
    }

    return 0;
}

int position_parse(char* s, coord_t* pos)
{
    char* cursor;

    pos->deg = 0;
    pos->min = 0;

    if (s == NULL || *s == '\0') {
        return -1;
    }

    /* decimal minutes */
    if (NULL == (cursor = strchr(s, '.'))) {
        return -1;
    }

    /* minutes starts 2 digits before dot */
    cursor -= 2;
    pos->min = atof(cursor);
    *cursor = '\0';

    /* integer degrees */
    cursor = s;
    pos->deg = atoi(cursor);

    return 0;
}

GGA_t parseMsgGGA(char** msgParts) 
{
    GGA_t rawGGA; 
    

    time_parse(msgParts[1], &rawGGA.timestamp);
    position_parse(msgParts[2], &rawGGA.lat);
    rawGGA.lat.dir = msgParts[3][0];
    position_parse(msgParts[4], &rawGGA.lon);
    rawGGA.lon.dir = msgParts[5][0];
    rawGGA.fix = atoi(msgParts[6]);
    

    return rawGGA;

}

bool isValidGGAMsg(GGA_t msg)
{
    return (msg.fix > 0 && msg.fix < 9);
}

float getDistanceKM(coord_t latA, coord_t latB, coord_t lonA, coord_t lonB)
{
    return 0;
}
float getTimeSec(tm_t timestampA, tm_t timestampB)
{
    return 0;
}

buffStatus_t updatePVTBuffer(GGA_t msg)
{
    static pvtBuf_t pvtBuffer[2];
    static int bufferCnt = BUFF_EMPTY;

    if (bufferCnt < BUFF_FULL)
    {        
        pvtBuffer[bufferCnt].lat = msg.lat;
        pvtBuffer[bufferCnt].lon = msg.lon;
        pvtBuffer[bufferCnt].timestamp = msg.timestamp;
        bufferCnt++;
    }
    if (bufferCnt == BUFF_FULL)
    {
        getDistanceKM(pvtBuffer[0].lat, pvtBuffer[1].lat,
            pvtBuffer[0].lon, pvtBuffer[1].lon);
        getTimeSec(pvtBuffer[0].timestamp, pvtBuffer[1].timestamp);
        memset(&pvtBuffer[0], 0, sizeof(pvtBuf_t));
        memset(&pvtBuffer[1], 0, sizeof(pvtBuf_t));
        bufferCnt = BUFF_EMPTY;

    }

    return (buffStatus_t)bufferCnt;


}


int processLog(FILE *logFile)
{
  enum msgParserState msgParser = MSG_VALID;
  char msg[NMEA_MSG_LEN + 1]; //One extra for null char
  int msg_len = 0;
  char** msgParts = NULL;
  msgType_t msgtype = (msgType_t)UNSUPPORTED;
  GGA_t msgGGA;
  
  while(fscanf(logFile, " %[^\n]", msg) != EOF)
  {
    msgParser = MSG_VALID;
    msg_len = strlen(msg);
    switch(msgParser)
    {

      case MSG_VALID:
        if(isValidMsg(msg, msg_len)){
          msgParser = MSG_SPLIT;
        }
        else{
          msgParser = MSG_VALID;
          break;
        }

      case MSG_SPLIT:
      {
          msgParts = getMsgParts(msg, msg_len);
          if (msgParts)
              msgParser = MSG_TYPE;
          else
              msgParser = MSG_FAILED;
      }

      case MSG_TYPE:
      {
          msgtype = getMsgType(*msgParts);
          printf("\n MSG tyPE %d", msgtype);
          if (msgtype == UNSUPPORTED)
              msgParser = MSG_VALID;
          else
              msgParser = MSG_DATA;

      }
      case MSG_DATA:
          
          if (msgtype == GGA)
          {
              msgGGA = parseMsgGGA(msgParts);
              if (isValidGGAMsg(msgGGA))
              {
                  updatePVTBuffer(msgGGA);
              }
          }
                

      case MSG_CHSUM:
      case  MSG_FAILED:
          ;
              
        
    }
  }
  return 0;

}

int main(void) {

  FILE *logFile = NULL; 
  enum logParserState logParser = LOGP_NOINIT;
  

  switch(logParser){
    case LOGP_NOINIT:
    {
      logFile = fopen(debugFile, "r");
      logParser = LOGP_FAILED;
      if(logFile != NULL){
          logParser = LOGP_OPENED;
      }
      
    }

    case LOGP_OPENED:
    {
      
      processLog(logFile);
      logParser = LOGP_CLOSE;
      
      
    }

    case LOGP_CLOSE:
    {
      fclose(logFile);
      break;
    }

    case LOGP_FAILED:
    {
      printf("Cannot find log");
      break;
    }




  }


  
}
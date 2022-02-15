#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "parse.h"

#define ENABLE 1
#define DISABLE 0
#define DEBUG ENABLE
#define MAX_MSGS INT_MAX
#define NMEA_MSG_LEN 82

const char defaultDebugFile[] = "./ridetest.txt";

/// <summary>
/// Convert degrees to radian
/// </summary>
/// <param name="deg">degrees</param>
/// <returns>radian</returns>
double deg2rad(const double deg) {
    return (deg * pi / 180);
}

/// <summary>
/// Convert radians to degrees
/// </summary>
/// <param name="rad">radians</param>
/// <returns>degrees</returns>
double rad2deg(const double rad) {
    return (rad * 180 / pi);
}

/// <summary>
/// Clears speed and timestamp data file
/// </summary>
void clear_data() {
    FILE* file;
    if ((file = fopen(".\speedLog.csv", "r"))) {
        fclose(file);
        file = fopen(".\speedLog.csv", "w");
        fclose(file);
    }
}

/// <summary>
/// Writes speed and timestamp to 'speedlog.csv' file
/// </summary>
/// <param name="speed_kmph">speed in kmph</param>
/// <param name="timestamp">timestamp as a string hh:mm:ss</param>
void write_data(const double speed_kmph, const tm_t timestamp) {

    static FILE* speed_log = NULL;
    char timestamp_str[9];
    char hour[3];
    char min[3];
    char sec[3];

    speed_log = fopen("./speedlog.csv", "a");

    if (speed_log == NULL) {
        printf("\nUnable to write to speedlog.csv");
        return;
    }

    sprintf(hour, "%d", timestamp.tm_hour);
    sprintf(min, "%d", timestamp.tm_min);
    sprintf(sec, "%d", timestamp.tm_sec);
    sprintf(timestamp_str, "%s:%s:%s", hour, min, sec);
    fprintf(speed_log, "%s,%f\n", timestamp_str, speed_kmph);
    fclose(speed_log);

}

/// <summary>
/// Calculate speed with location coordinates from the last 2 GGA
/// messages
/// </summary>
/// <param name="msg">GGA message in GGA_t format</param>
/// <returns>Status of the internal buffer- in use or full</returns>
buffer_status_t update_speed_timestamp(const GGA_t msg) {
    static pvt_buffer_t pvt_buffer[2];
    static uint8_t buffer_count = BUFF_EMPTY;
    double distance_km, time_sec, speed_kmph;

    distance_km = -1;
    time_sec = -1;
    speed_kmph = -1;

    if (buffer_count < BUFF_FULL) {
        pvt_buffer[buffer_count].lat = msg.lat;
        pvt_buffer[buffer_count].lon = msg.lon;
        pvt_buffer[buffer_count].timestamp = msg.timestamp;
        buffer_count++;
    }
    if (buffer_count == BUFF_FULL) {
        distance_km = get_distance_km(pvt_buffer[0].lat, pvt_buffer[1].lat,
            pvt_buffer[0].lon, pvt_buffer[1].lon);
        time_sec = get_time_sec(pvt_buffer[0].timestamp, pvt_buffer[1].timestamp);
        speed_kmph = get_speed_kmph(distance_km, time_sec);

        write_data(speed_kmph, pvt_buffer[1].timestamp);

        memcpy(&pvt_buffer[0], &pvt_buffer[1], sizeof(pvt_buffer));
        memset(&pvt_buffer[1], 0, sizeof(pvt_buffer_t));
        buffer_count = BUFF_INUSE;

    }

    return (buffer_status_t)buffer_count;


}

/// <summary>
/// Get speed 
/// </summary>
/// <param name="distanceKm">Distance covered between last two GGA msgs</param>
/// <param name="tSec">Time elapsed between last two GGA msgs</param>
/// <returns>Speed in kmph</returns>
double get_speed_kmph(const float distanceKm, const float tSec) {
    double speed_kmph;

    speed_kmph = distanceKm / (tSec / 3600);
    return speed_kmph;
}

/// <summary>
/// Get distance covered between last two GGA msgs
/// </summary>
/// <param name="latA">Latitude in the second to last GGA message </param>
/// <param name="latB">Latitude in the last GGA message</param>
/// <param name="lonA">Longitude in the second to last GGA message</param>
/// <param name="lonB">Longitude in the last GGA message</param>
/// <returns>Distance in km</returns>
double get_distance_km(const coord_t latA, const coord_t latB, const coord_t lonA, const coord_t lonB) {
    double theta, dist, lat1, lat2, lon1, lon2;

    dist = 0;
    lat1 = (latA.deg + (latA.min / 100)) * ((latA.dir == 'N') ? 1 : -1);
    lat2 = (latB.deg + (latB.min / 100)) * ((latB.dir == 'N') ? 1 : -1);
    lon1 = (lonA.deg + (lonA.min / 100)) * ((lonA.dir == 'E') ? 1 : -1);
    lon2 = (lonB.deg + (lonB.min / 100)) * ((lonB.dir == 'E') ? 1 : -1);

    if ((lat1 != lat2) && (lon1 != lon2)) {
        theta = lon1 - lon2;
        dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
        dist = acos(dist);
        dist = rad2deg(dist);
        dist = dist * 60 * 1.1515;
        dist = dist * 1.609344;
    }
    return dist;
}

/// <summary>
/// Get time elapsed between the last two GGA messages
/// </summary>
/// <param name="timestampA">Timestamp of the last but one GGA message</param>
/// <param name="timestampB">Timestamp of the last GGA message</param>
/// <returns>Time in seconds</returns>
double get_time_sec(const tm_t timestampA, const tm_t timestampB) {
    double time_sec = -1;

    time_sec = fabs(((timestampA.tm_hour * 60 * 60) + (timestampA.tm_min * 60) + timestampA.tm_sec) -
        ((timestampB.tm_hour * 60 * 60) + (timestampB.tm_min * 60) + timestampB.tm_sec));
    return time_sec;

}

/// <summary>
/// Parse part of GGA message containing timestamp
/// </summary>
/// <param name="s">Part of the GGA message containing timestamp</param>
/// <param name="time">Timestamp as a tm_t object</param>
/// <returns>Status of parsing</returns>
uint8_t time_parse(const char* s, tm_t* time) {
    char* rv;
    uint32_t x;

    time->tm_hour = -1;
    time->tm_min = -1;
    time->tm_sec = -1;

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
        /* Do nothing. Sub-second left unprocessed */
    }

    return 0;
}

/// <summary>
/// Parse part of GGA message containing location coordinates
/// </summary>
/// <param name="s">Part of the GGA message containing location information</param>
/// <param name="pos">Location as a coord_t object</param>
/// <returns>Status of parsing</returns>
uint8_t position_parse(const char* s, coord_t* pos) {
    char* cursor;

    pos->deg = -1;
    pos->min = -1;

    if (s == NULL || *s == '\0') {
        return -1;
    }

    /* get decimal point */
    if (NULL == (cursor = strchr(s, '.'))) {
        return -1;
    }

    /* minutes starts 2 digits before decimal point */
    cursor -= 2;
    pos->min = atof(cursor);

    /* isolate degrees */
    *cursor = '\0';

    /* get degrees */
    cursor = s;
    pos->deg = atoi(cursor);

    return 0;
}

/// <summary>
/// Evaluate validity of the GGA message. 
/// Check whether fix and location coordinates are valid.
/// </summary>
/// <param name="msg">GGA message as a GGA_t object</param>
/// <returns>True if GGA message contains valid information</returns>
bool is_valid_GGA(const GGA_t msg) {
    if (msg.lat.deg == -1 || msg.lat.min == -1) {
        return false;
    }
    if (msg.lon.deg == -1 || msg.lon.min == -1) {
        return false;
    }
    if (msg.timestamp.tm_hour == -1 || msg.timestamp.tm_min == -1 || msg.timestamp.tm_sec == -1) {
        return false;
    }
    if (!(msg.fix > 0 && msg.fix < 9)) {
        return false;
    }

    return true;

}

/// <summary>
/// Parses all parts of a GGA message and maps them to GGA_t object
/// </summary>
/// <param name="msg_parts">Parts of a message of GGA type</param>
/// <returns>GGA message mapped to a GGA_t object</returns>
GGA_t parse_GGA(char** msg_parts) {
    GGA_t raw_msg;

    time_parse(msg_parts[1], &raw_msg.timestamp);
    position_parse(msg_parts[2], &raw_msg.lat);
    raw_msg.lat.dir = msg_parts[3][0];
    position_parse(msg_parts[4], &raw_msg.lon);
    raw_msg.lon.dir = msg_parts[5][0];
    raw_msg.fix = atoi(msg_parts[6]);

    return raw_msg;
}

/// <summary>
/// Parses part of the message containing type to evaluate type
/// </summary>
/// <param name="token">Part of the message potentially containing type</param>
/// <returns>Message type</returns>
msg_type_t get_msg_type(const char* token) {
    msg_type_t type = (msg_type_t)UNSUPPORTED;

    /* Skip first 2 characters */
    token++;
    token++;
    if (token) {
        if (strcmp("VTG", token) == 0) {
            type = (msg_type_t)VTG;
        }
        else if (strcmp("GGA", token) == 0) {
            type = (msg_type_t)GGA;
        }
        else if (strcmp("RMC", token) == 0) {
            type = (msg_type_t)RMC;
        }
    }
    return type;
}

/// <summary>
/// Splits a string containing a GPS sentence/message into 
/// chunks or tokens
/// </summary>
/// <param name="msg">GPS sentence or message as a string</param>
/// <param name="length">Length of the string including null char</param>
/// <returns>Chunks/tokens of the message after being split by commas</returns>
char** split_msg(const char* msg, size_t length) {
    uint32_t pos, len_token, num_token, previous_delim_pos;
    char* temp = NULL;
    char** tokens = NULL;

    num_token = 0;
    previous_delim_pos = 0;
    len_token = 0;
    temp = _strdup(msg);
    tokens = (char**)malloc(sizeof(char) * 90);

    for (pos = 0; pos < length; pos++) {
        if (temp[pos] == ',' || temp[pos] == '*' || temp[pos] == '\n') {
            temp[pos] = '\0';
            if (pos > 5 && temp[pos - 1] == ',') {
                tokens[num_token] = (char*)malloc(sizeof(char) * 2);
                strncpy(tokens[num_token], " \0", 2);
            }
            else {
                len_token = pos - previous_delim_pos;
                tokens[num_token] = (char*)malloc(sizeof(char) * (len_token));
                strncpy(tokens[num_token], temp + previous_delim_pos + 1, len_token);
            }
            num_token++;
            len_token = 0;
            previous_delim_pos = pos;
        }
    }
    free(temp);
    return tokens;

}

/// <summary>
/// Gets parts of a GPS message split by delimiters
/// </summary>
/// <param name="msg">GPS message as a string</param>
/// <param name="length">Length of the string including null char</param>
/// <returns>Message split into parts</returns>
char** get_msg_parts(const char* msg, size_t length) {

    char** tokens;

    tokens = split_msg(msg, length);
    return tokens;
}

/// <summary>
/// Process each message from the log file, 
/// Look for GGA messages, get location information from GGA messages,
/// Calculate speed and write speed and timestamp to a csv file
/// </summary>
/// <param name="msg">GPS message to parse</param>
/// <returns>0</returns>
uint8_t process_msg(const char* msg) {
    msg_parser_t msg_parser = MSG_VALID;
    msg_type_t msg_type = (msg_type_t)UNSUPPORTED;    
    buffer_status_t buffer_status = (buffer_status_t)BUFF_EMPTY;
    GGA_t msg_GGA;

    size_t msg_len = strlen(msg);
    char** msg_parts = NULL;
    
    bool is_msg_processed = false;

    while (!is_msg_processed) {
        switch (msg_parser) {

            case MSG_VALID: {
                if (is_valid_msg(msg, msg_len)) {
                    msg_parser = MSG_SPLIT;
                }
                else {
                    msg_parser = MSG_FAILED;
                }
                break;
            }

            case MSG_SPLIT: {
                msg_parts = get_msg_parts(msg, msg_len);
                if (msg_parts) {
                    msg_parser = MSG_TYPE;
                }
                else {
                    msg_parser = MSG_DISCARD;
                }
                break;
            }

            case MSG_TYPE: {
                msg_type = get_msg_type(*msg_parts);
                if (msg_type == UNSUPPORTED) {
                    msg_parser = MSG_DISCARD;
                }
                else {
                    msg_parser = MSG_DATA;
                }
                break;

            }
            case MSG_DATA: {
                if (msg_type == GGA) {
                    msg_GGA = parse_GGA(msg_parts);
                    if (is_valid_GGA(msg_GGA)) {
                        buffer_status = update_speed_timestamp(msg_GGA);
                    }
                }
                msg_parser = MSG_DISCARD;
            }

            case MSG_CHSUM: {
                ;
            }
            case  MSG_FAILED: {
                is_msg_processed = true;
                break;
            }

            case MSG_DISCARD: {
                is_msg_processed = true;
                free(msg_parts);
                break;
            }
        }
    }
    return 0;
}

/// <summary>
/// Check if the message is valid by looking for a valid talker ID
/// This implementation does not evaluate and verify checksum.
/// </summary>
/// <param name="msg">GPS message as a string</param>
/// <param name="length">length of the string</param>
/// <returns>True is message is valid</returns>
bool is_valid_msg(const char* msg, size_t length) {
    char talker_id[3] = { 0 };

    /* Skip $ sign*/
    msg++;

    /* Get first 2 characters as talker ID */
    strncpy(talker_id, msg, 2);
    if (strcmp(talker_id, "GN") == 0 || strcmp(talker_id, "GP") == 0 || strcmp(talker_id, "GL") == 0)
        return true;
    else
        return false;

}

/// <summary>
/// Process the log file to look for GPS messages. 
/// Advance potential GPS messages to process_msg
/// </summary>
/// <param name="log_file"></param>
/// <returns></returns>
uint8_t process_log(const FILE *log_file)
{
  
  char msg[100] = { 0 };
  bool got_msg;
  int symbol;

  got_msg = false; 
 
  clear_data();

  while((symbol=fgetc(log_file))!=EOF) {
      if (symbol == '$') {
          if (got_msg){
              got_msg = false;
              memset(msg, 0, sizeof(msg));
          }
          else {
              got_msg = true;              
          }                          
      }
          
      if (symbol == '*' && got_msg == true) {          
          process_msg(msg);          
          memset(msg, 0, sizeof(msg));
          got_msg = false;         
      }
      if (got_msg) {
          if (strlen(msg) < (NMEA_MSG_LEN + 1)) {
              strncat(msg, &symbol, 1);
          }            
          else {
              memset(msg, 0, sizeof(msg));
          }
      }            
  }

  return 0;
}

/// <summary>
/// Driver function. 
/// Accepts log file name as argument. 
/// </summary>
/// <param name="argc"></param>
/// <param name="argv">Enter log file name within quotes</param>
/// <returns>0</returns>
int32_t main(int argc, char* argv[]){
  

  if (argc != 2) {
      printf("Provide log file to parse within double quotes\n\
               For example: ./parse.out \"ridetest.txt\"");
      return 0;
    }

  FILE* log_file = NULL;
  log_parse_state_t log_parser = LOGP_NOINIT;
  const char* file_to_load = argv[1];
  
  

  switch (log_parser){
    case LOGP_NOINIT:{
      log_file = fopen(file_to_load, "rb");
      log_parser = LOGP_FAILED;
      if(log_file != NULL){
          log_parser = LOGP_OPENED;
      }
      
    }

    case LOGP_OPENED:{      
      process_log(log_file);
      log_parser = LOGP_CLOSE;    
    }

    case LOGP_CLOSE:{
        if(log_file)
            fclose(log_file);
        break;
    }

    case LOGP_FAILED:{
      printf("Cannot find log");
      break;
    }
  }  

  return 0;
}
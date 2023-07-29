#ifndef SIP_TIME_H
#define SIP_TIME_H

#include <cstdint>
#include <string>
#include <ctime>

/*second*/
std::time_t get_system_timestamp(void);
/*utc format*/
std::string get_utc_time(void);
std::string get_utc_time_by_ts(std::time_t);
/*ios format*/
std::string get_iso_time(void);
std::string get_iso_time_by_ts(std::time_t);
/*convert str time to timestamp*/
std::time_t string_to_timestamp(std::string);
/*ntp*/

#endif
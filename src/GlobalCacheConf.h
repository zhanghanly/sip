#ifndef GLOBAL_CACHE_CONF_H
#define GLOBAL_CACHE_CONF_H

#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <set>
#include <memory>
#include "MessageQueue.h"
#include "redis/hiredis.h"
#include "redis/async.h"
#include "redis/adapters/libevent.h"
#include "DBConnectionPool.h"


struct ChannelStreamInfo {
	ChannelStreamInfo();
	ChannelStreamInfo(const std::string&,
			  const std::string&,
			  const std::string&,
			  const std::string&,
			  const std::string&,
			  const std::string&,
			  bool,
			  bool,
			  int,
			  bool,
			  bool,
			  bool);

	ChannelStreamInfo& operator=(const ChannelStreamInfo&) = default;

	/*enterprise id*/
	std::string enterpriseid;
	/*device ID*/
	std::string camera_deviceid;
	/*for mysql*/
	std::string camera_id;
	/*video channel ID*/
	std::string camera_video_channel_code;
	/*audio output id*/
	std::string camera_speech_output_channel_code;
	/*alarm input channel id*/
	std::string camera_alarm_input_channel_code;
	/*media transport way(TCP or UDP)*/
	std::string media_transport;
	/*sip transport(TCP or UDP)*/
	std::string command_transport;
	/*enable snap or not(parsing json error when type is bool)*/
	uint8_t open_snap;
	/*enable save video or not(parsing json error when type is bool)*/
	uint8_t open_save_video;
	/*the way to save snap(append or override)*/
	uint8_t snap_storage_way;
	/*enable audio or not*/
	uint8_t open_audio;
	/*the interval of snap*/
	int snap_interval;
	/*enable denoise or not*/
	uint8_t open_denoise;
	/*camera state(0->disable, 1->enable)*/
	uint8_t camera_state;
};

struct EnterpriseSaveInfo {
	EnterpriseSaveInfo() = default;
	EnterpriseSaveInfo(int, int, int, int, int);
	EnterpriseSaveInfo& operator=(const EnterpriseSaveInfo&) = default;

	/*enterprise id*/
	std::string enterpriseid;
	/*video save spave(GB)*/
	int64_t video_space;
	/*live video save days*/
	int live_video_save_days;
	/*monitor video save days*/
	int monitor_video_save_days;
	/*picture save space(GB)*/
	int64_t snap_space;
	/*picture save days*/
	int snap_save_days;
	/*enterprise state(0:disable, 1:enable)*/
	uint8_t enterprise_state;
	/*enterprise contract expire time*/
	int64_t enterprise_contract_expiration;
};

struct CameraTaskInfo {
	CameraTaskInfo();
	uint8_t task_type;
	std::string task_cycle;
	int64_t task_date;
	int64_t live_start_time;
	int64_t live_end_time;
	uint8_t live_task_status;
	int64_t live_task_invalid_date;
	uint8_t live_status;
	int64_t live_disable_duration;
	int64_t live_disable_time;
	int64_t live_recovery_time;
}; 


class GlobalCacheConf : public MessageRecvHandle {
public:
	~GlobalCacheConf();
	static GlobalCacheConf* get_instance();
	std::shared_ptr<ChannelStreamInfo> fetch_channel_by_deviceid(const std::string&);
	std::shared_ptr<EnterpriseSaveInfo> fetch_enterprise_by_deviceid(const std::string&);
	std::shared_ptr<ChannelStreamInfo> fetch_channel_by_cameraid(const std::string&);
	std::shared_ptr<EnterpriseSaveInfo> fetch_enterprise_by_cameraid(const std::string&);
	std::list<std::shared_ptr<CameraTaskInfo>> fetch_task_lst_by_cameraid(const std::string&);
	/*implement subscribe interface*/	
	virtual void recv_message(const std::string&) override;
	virtual std::string topic() override;
	//static void cycle(GlobalCacheConf*);

private:
	GlobalCacheConf();
	
	static std::shared_ptr<ChannelStreamInfo> parse_json_for_channelStreamInfo(const std::string&);
	static std::shared_ptr<EnterpriseSaveInfo> parse_json_for_enterpriseSaveInfo(const std::string&);
	static std::list<std::shared_ptr<CameraTaskInfo>> parse_json_for_task_info(const std::string&);

	static std::string query_camera_by_id(const std::string&);
	static std::string query_channel_by_id(const std::string&);	
	static std::string query_enterprise_by_id(const std::string&);	
	static std::string query_task_by_id(const std::string&);	
	
	/*deviceid->cameraid*/
	std::map<std::string, std::string> deviceid_cameraid_;
	/*cameraid->deviceid*/
	std::map<std::string, std::string> cameraid_deviceid_;
	/*deviceid->enterpriseid*/
	std::map<std::string, std::string> deviceid_enterpriseid_;
	/*enterpriseid->deviceid*/
	std::map<std::string, std::set<std::string>> enterpriseid_deviceid_;
	/*deviceid->ChannelStreamInfo*/
	std::map<std::string, std::shared_ptr<ChannelStreamInfo>> deviceid_stream_info_;
	/*enterpriseid->EnterpriseSaveInfo*/
	std::map<std::string, std::shared_ptr<EnterpriseSaveInfo>> enterpriseid_save_;
	/*cameraid->camera_task_info*/
	std::map<std::string, std::list<std::shared_ptr<CameraTaskInfo>>> cameraid_task_info_;
};

#endif
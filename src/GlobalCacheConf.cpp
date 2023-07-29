#include <cstring>
#include <stdexcept>
#include <set>
#include <sstream>
#include <chrono>
#include <algorithm>
#include "GlobalCacheConf.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "ServerConfig.h"
#include "SipManager.h"
#include "DataBaseHandler.h"
#include "DBConnectionPool.h"


using json = nlohmann::json;

ChannelStreamInfo::ChannelStreamInfo() :
					open_snap(0),
					open_save_video(0),
					snap_interval(500),
					snap_storage_way(0),
					open_audio(1),
					open_denoise(0),
					camera_state(1) {
}

ChannelStreamInfo::ChannelStreamInfo(const std::string& deviceid,
			  const std::string& video_channel_code,
			  const std::string& speech_channel_code,
			  const std::string& alarm_channel_code,
			  const std::string& media_transprot,
			  const std::string& command_transprot,
			  bool open_snap,
			  bool open_save_video,
			  int snap_interval,
			  bool snap_storage_way,
			  bool open_audio,
			  bool open_denoise)
		: camera_deviceid(deviceid),
		  camera_video_channel_code(video_channel_code),
		  camera_speech_output_channel_code(speech_channel_code),
		  camera_alarm_input_channel_code(alarm_channel_code),
		  media_transport(media_transprot),
		  command_transport(command_transprot),
		  open_snap(open_snap),
		  open_save_video(open_save_video),
		  snap_interval(snap_interval),
		  snap_storage_way(snap_storage_way),
		  open_audio(open_audio),
		  open_denoise(open_denoise),
		  camera_state(1) {
} 


EnterpriseSaveInfo::EnterpriseSaveInfo(int video_space,
				       int live_video_save_days,
				       int monitor_video_save_days,
				       int snap_space,
				       int snap_save_days)
	          : video_space(video_space),
		    live_video_save_days(live_video_save_days),
		    monitor_video_save_days(monitor_video_save_days),
		    snap_space(snap_space),
		    snap_save_days(snap_save_days),
			enterprise_state(1) {
}

CameraTaskInfo::CameraTaskInfo() 
			: live_task_status(0),
			live_status(0),
			live_disable_duration(0) {

}


GlobalCacheConf::GlobalCacheConf() {
}

GlobalCacheConf* GlobalCacheConf::get_instance() {
	static GlobalCacheConf globalCacheConf;
	return &globalCacheConf;
}


GlobalCacheConf::~GlobalCacheConf() {
}


std::shared_ptr<ChannelStreamInfo> GlobalCacheConf::fetch_channel_by_deviceid(const std::string& deviceid) {
	if (deviceid_stream_info_.find(deviceid) != deviceid_stream_info_.end()) {
		return deviceid_stream_info_[deviceid];
	} else {
		if (deviceid_cameraid_.find(deviceid) == deviceid_cameraid_.end()) {
			std::string answer = query_camera_by_id(deviceid);
			//std::string answer = DBConnectionPool::get_instance()->fetch_answer_sync(deviceid);  
			if (!answer.empty()) {
				deviceid_cameraid_[deviceid] = answer;
				cameraid_deviceid_[answer] = deviceid;
			} else {
				return nullptr;
			}
		}

		std::string cameraid = deviceid_cameraid_[deviceid];
		std::string answer = query_channel_by_id(cameraid);
		//std::string answer = DBConnectionPool::get_instance()->fetch_answer_sync(query_channel_command);  
		std::shared_ptr<ChannelStreamInfo> channel_sp = parse_json_for_channelStreamInfo(answer);
		if (channel_sp && channel_sp->camera_deviceid == deviceid) {
			deviceid_stream_info_[deviceid] = channel_sp;
			deviceid_enterpriseid_[deviceid] = channel_sp->enterpriseid;
		
			return channel_sp;
		}

		return nullptr;
	}
}

std::shared_ptr<EnterpriseSaveInfo> GlobalCacheConf::fetch_enterprise_by_deviceid(const std::string& deviceid) {
	if (enterpriseid_save_.find(deviceid) != enterpriseid_save_.end()) {
		return enterpriseid_save_[deviceid];
	} else {
		if (deviceid_cameraid_.find(deviceid) == deviceid_cameraid_.end()) {
			return nullptr;
		}
		if (deviceid_enterpriseid_.find(deviceid) == deviceid_enterpriseid_.end()) {
			/*query channel info first*/
			fetch_channel_by_deviceid(deviceid);
			if (deviceid_enterpriseid_.find(deviceid) == deviceid_enterpriseid_.end()) {
				return nullptr;
			}
		}
		std::string enterpriseid = deviceid_enterpriseid_[deviceid];
		std::string answer = query_enterprise_by_id(enterpriseid);
		//std::string answer = DBConnectionPool::get_instance()->fetch_answer_sync(query_enterprise_command); 
		std::shared_ptr<EnterpriseSaveInfo> enterprise_sp = parse_json_for_enterpriseSaveInfo(answer);
		if (enterprise_sp) {
			enterpriseid_save_[deviceid] = enterprise_sp;
		}

		return enterprise_sp;
	}
}

std::shared_ptr<ChannelStreamInfo> GlobalCacheConf::fetch_channel_by_cameraid(const std::string& cameraid) {
	if (cameraid_deviceid_.find(cameraid) == cameraid_deviceid_.end()) {
		return nullptr;
	} else {
		return fetch_channel_by_deviceid(cameraid_deviceid_[cameraid]);
	}
}

std::shared_ptr<EnterpriseSaveInfo> GlobalCacheConf::fetch_enterprise_by_cameraid(const std::string& cameraid) {
	if (cameraid_deviceid_.find(cameraid) == cameraid_deviceid_.end()) {
		return nullptr;
	} else {
		return fetch_enterprise_by_deviceid(cameraid_deviceid_[cameraid]);
	}

}

std::list<std::shared_ptr<CameraTaskInfo>> GlobalCacheConf::fetch_task_lst_by_cameraid(const std::string& cameraid) {
	if (cameraid_task_info_.find(cameraid) != cameraid_task_info_.end()) {
		return cameraid_task_info_[cameraid];
	} else {
		std::list<std::shared_ptr<CameraTaskInfo>> task_lst;
		bool cameraid_exist = false;
		for (auto& elem: deviceid_cameraid_) {
			if (elem.second == cameraid)
				cameraid_exist = true;
		}
		if (cameraid_exist) {
			std::string answer = query_task_by_id(cameraid);
			//std::string answer = DBConnectionPool::get_instance()->fetch_answer_sync(query_task_command);
			task_lst = parse_json_for_task_info(answer);
			cameraid_task_info_[cameraid] = task_lst;
		}

		return task_lst;
	}
}


std::shared_ptr<ChannelStreamInfo> GlobalCacheConf::parse_json_for_channelStreamInfo(const std::string& json_str) {
	std::shared_ptr<ChannelStreamInfo> sp = std::make_shared<ChannelStreamInfo>();

	spdlog::info("json={}", json_str);

	try {
		json j = json::parse(json_str);

		for (auto& iter : j.items()) {
			if (iter.key() == "enterpriseId") {
				sp->enterpriseid = iter.value();
			} else if (iter.key() == "cameraDeviceId") {
				sp->camera_deviceid = iter.value();
			} else if (iter.key() == "id") {
				sp->camera_id = iter.value();
			} else if (iter.key() == "cameraVideoChannelCode") {
				sp->camera_video_channel_code = iter.value();
			} else if (iter.key() == "cameraSpeechOutputChannelCode") {
				sp->camera_speech_output_channel_code = iter.value();
			} else if (iter.key() == "cameraAlarmInputCode") {
				sp->camera_alarm_input_channel_code = iter.value();
			} else if (iter.key() == "mediaTransport") {
				sp->media_transport = iter.value();
			} else if (iter.key() == "commandTransport") {
				sp->command_transport = iter.value();
			} else if (iter.key() == "snapshotState") {
				sp->open_snap = iter.value();
			} else if (iter.key() == "openSaveVideo") {
				sp->open_save_video = iter.value();
			} else if (iter.key() == "snapshotStorageWay") {
				sp->snap_storage_way = iter.value();
			} else if (iter.key() == "snapshotInterval") {
				sp->snap_interval = iter.value();
			} else if (iter.key() == "openAudio") {
				sp->open_audio = iter.value();
			} else if (iter.key() == "openDenoise") {
				sp->open_denoise = iter.value();
			} else if (iter.key() == "state") {
				sp->camera_state = iter.value();
			}
		}

		spdlog::info(sp->enterpriseid); 
		spdlog::info(sp->camera_deviceid);
		spdlog::info(sp->camera_video_channel_code); 
		spdlog::info(sp->camera_speech_output_channel_code); 
		spdlog::info(sp->camera_alarm_input_channel_code); 
		spdlog::info(sp->media_transport); 
		spdlog::info(sp->command_transport); 
		spdlog::info((int)sp->open_snap); 
		spdlog::info((int)sp->open_save_video); 
		spdlog::info((int)sp->snap_storage_way); 
		spdlog::info((int)sp->snap_interval); 
		spdlog::info((int)sp->open_audio); 
		spdlog::info((int)sp->open_denoise); 
		spdlog::info((int)sp->camera_state); 
	
	} catch(std::runtime_error& e) {
		sp.reset();
	} catch(std::exception& e) {
		sp.reset();
	}

	return sp;
}
	
std::shared_ptr<EnterpriseSaveInfo> GlobalCacheConf::parse_json_for_enterpriseSaveInfo(const std::string& json_str) {
	std::shared_ptr<EnterpriseSaveInfo> sp = std::make_shared<EnterpriseSaveInfo>();
	
	spdlog::info("json={}", json_str);
	try {
		json j = json::parse(json_str);

		for (auto& iter : j.items()) {
			if (iter.key() == "id") {
				sp->enterpriseid = iter.value();
			} else if (iter.key() == "liveVideoStorageSpace") {
				sp->video_space = iter.value();
			} else if (iter.key() == "liveVideoStorageDays") {
				sp->live_video_save_days = iter.value();
			} else if (iter.key() == "monitorVideoStorageDays") {
				sp->monitor_video_save_days = iter.value();
			} else if (iter.key() == "livePicStorageSpace") {
				sp->snap_space = iter.value();
			} else if (iter.key() == "livePicStorageDays") {
				sp->snap_save_days = iter.value();
			} else if (iter.key() == "state") {
				sp->enterprise_state = iter.value();
			} else if (iter.key() == "contractExpiration") {
				sp->enterprise_contract_expiration = iter.value();
			}
		}

		spdlog::info(sp->video_space);
		spdlog::info(sp->live_video_save_days);
		spdlog::info(sp->monitor_video_save_days);
		spdlog::info(sp->snap_space);
		spdlog::info(sp->snap_save_days);
	
	} catch(std::runtime_error& e) {
		sp.reset();
	} catch(std::exception& e) {
		sp.reset();
	}

	return sp;
}

std::list<std::shared_ptr<CameraTaskInfo>> GlobalCacheConf::parse_json_for_task_info(const std::string& json_str) {
	spdlog::info("json={}", json_str);
	std::list<std::shared_ptr<CameraTaskInfo>> task_lst;
	
	try {
		json j = json::parse(json_str);
		for (auto iter = j.begin(); iter != j.end(); ++iter) {
			std::shared_ptr<CameraTaskInfo> task_sp = std::make_shared<CameraTaskInfo>();
			
			for (auto& item : iter->items()) {
				if (item.key() == "taskType") {
					task_sp->task_type = item.value();	
				} else if (item.key() == "taskCycle") {
					task_sp->task_cycle = item.value();	
				} else if (item.key() == "taskDate") {
					task_sp->task_date = item.value();	
				} else if (item.key() == "liveStartTime") {
					task_sp->live_start_time = item.value();	
				} else if (item.key() == "liveEndTime") {
					task_sp->live_end_time = item.value();	
				} else if (item.key() == "liveTaskStatus") {
					task_sp->live_task_status = item.value();	
				} else if (item.key() == "liveTaskInvalidDate") {
					task_sp->live_task_invalid_date = item.value();	
				} else if (item.key() == "liveStatus") {
					task_sp->live_status = item.value();	
				} else if (item.key() == "liveDisableDuration"){
					task_sp->live_disable_duration = item.value();	
				} else if (item.key() == "liveDisableTime"){
					task_sp->live_disable_time = item.value();	
				} else if (item.key() == "liveRecoveryTime"){
					task_sp->live_recovery_time = item.value();	
				}
			}
			
			task_lst.push_back(task_sp);
		}
	
	} catch(std::runtime_error& e) {
		task_lst.clear();
	} catch(std::exception& e) {
		task_lst.clear();
	}

	return task_lst;
}

void GlobalCacheConf::recv_message(const std::string& subscribe_msg) {
	if (!subscribe_msg.empty()) {
		if (subscribe_msg.find("livePicStorageDays") != std::string::npos &&
		    subscribe_msg.find("liveVideoStorageDays") != std::string::npos) {
			std::shared_ptr<EnterpriseSaveInfo> enterprise_sp = parse_json_for_enterpriseSaveInfo(subscribe_msg);
			for (auto& item : deviceid_enterpriseid_) {
				if (enterprise_sp && item.second == enterprise_sp->enterpriseid) {
					//enterpriseid_save_[enterprise_sp->enterpriseid] = enterprise_sp;
					enterpriseid_save_[item.first] = enterprise_sp;
					spdlog::info("recieve enterprise subscribe msg successfully");
				}
			}

		} else {
			std::shared_ptr<ChannelStreamInfo> channel_sp = parse_json_for_channelStreamInfo(subscribe_msg);
			if (channel_sp && deviceid_cameraid_.find(channel_sp->camera_deviceid) != deviceid_cameraid_.end()) {
				deviceid_stream_info_[channel_sp->camera_deviceid] = channel_sp;
				deviceid_enterpriseid_[channel_sp->camera_deviceid] = channel_sp->enterpriseid;
				spdlog::info("recieve channel subscribe msg successfully");
			}
		}
	}
}

std::string GlobalCacheConf::topic() {
	return ServerConfig::get_instance()->redis_enterprise_stream_topic();
}

std::string GlobalCacheConf::query_camera_by_id(const std::string& deviceid) {
	return DBConnectionPool::get_instance()->fetch_answer_sync("HGET", "smd:camera:relation", deviceid);
}	
	
std::string GlobalCacheConf::query_channel_by_id(const std::string& cameraid) {
	return DBConnectionPool::get_instance()->fetch_answer_sync("HGET", "smd:camera", cameraid);
}	

std::string GlobalCacheConf::query_enterprise_by_id(const std::string& enterpriseid) {
	return DBConnectionPool::get_instance()->fetch_answer_sync("HGET", "smd:enterprise", enterpriseid);
}	
	
std::string GlobalCacheConf::query_task_by_id(const std::string& cameraid) {
	return DBConnectionPool::get_instance()->fetch_answer_sync("HGET", "smd:camera:task", cameraid);
}	
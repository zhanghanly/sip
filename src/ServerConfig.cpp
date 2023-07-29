#include <fstream>
#include <cstdlib>
#include "ServerConfig.h"
#include "spdlog/spdlog.h"

ServerConfig* ServerConfig::get_instance() {
    static ServerConfig instance;
    return &instance;
}

void ServerConfig::init_conf(const std::string& conf_file) {
    std::ifstream file(conf_file.c_str(), std::ios::in);
    std::string line;
    std::string sub_line;
    while (getline(file, line)) {
        if(line.find("[sip]") != std::string::npos) {
            while (getline(file, sub_line)) {
				if (sub_line.find("sip_port") != std::string::npos) {
                    sip_port_ = atoi(std::string(sub_line.begin() + 9, sub_line.end()).c_str());
                    spdlog::info("sip_port={}", sip_port_);
                
                } else if(sub_line.find("fix_recv_stream") != std::string::npos) {
                    rtp_fix_recv_stream_port_ = atoi(std::string(sub_line.begin() +16, sub_line.end()).c_str());
                    spdlog::info("sip_fix_recv_stream_port={}", rtp_fix_recv_stream_port_);    
                
                } else if(sub_line.find("recv_rtp_wai_ip") != std::string::npos) {
                    recv_rtp_wai_ip_ = std::string(sub_line.begin() + 16, sub_line.end());
                    spdlog::info("recv_rtp_wai_ip={}", recv_rtp_wai_ip_);    
                
                } else if (sub_line.find("wai_ip") != std::string::npos) {
                    sip_wai_ip_ = std::string(sub_line.begin()+ 7, sub_line.end());
                    spdlog::info("sip_wai_ip={}", sip_wai_ip_);                    
                
                } else if (sub_line.find("serial") != std::string::npos) {
                    sip_serial_ = std::string(sub_line.begin() +7, sub_line.end());
                    spdlog::info("sip_serial={}", sip_serial_);                    
                
                } else if (sub_line.find("realm") != std::string::npos) {
                    sip_realm_ = std::string(sub_line.begin() + 6, sub_line.end());
                    spdlog::info("sip_realm={}", sip_realm_);                    
                
                } else if (sub_line.find("password") != std::string::npos) {
                    sip_password_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("sip_password={}", sip_password_);                    
                
                } else if (sub_line.find("tcp_recv_rtp") != std::string::npos) {
                    if (sub_line.find("off") != std::string::npos) {
                        tcp_recv_rtp_ = false;
                        spdlog::info("tcp_recv_rtp=false");                    
                    } else {
                        tcp_recv_rtp_ = true;
                        spdlog::info("tcp_recv_rtp=true");                    
                    }
                
                } else if (sub_line.find("rtp_timeout") != std::string::npos) {
                    sip_rtp_timeout_ = std::stoi(std::string(sub_line.begin() + 12, sub_line.end()));
                    spdlog::info("sip_rtp_timeout={}", sip_rtp_timeout_);                    

                } else if (sub_line.find("sip_keepalive_timeout") != std::string::npos) {
                    sip_keepalive_timeout_ = std::stoi(std::string(sub_line.begin() + 22, sub_line.end()));
                    spdlog::info("sip_keepalive_timeout={}", sip_keepalive_timeout_);                    

                } else if (sub_line.find("catalog_interval") != std::string::npos) {
                    sip_catalog_interval_ = std::stoi(std::string(sub_line.begin() + 17, sub_line.end()));
                    spdlog::info("sip_catalog_interval={}", sip_catalog_interval_);                    

                } else if (sub_line.find("open_audio") != std::string::npos) {
                    if (sub_line.find("off") != std::string::npos) {
                        sip_open_audio_ = false;
                        spdlog::info("sip_open_audio=false");                    
                    } else {
                        sip_open_audio_ = true;
                        spdlog::info("sip_open_audio=true");                    
                    }

                } else if (sub_line.find("open_denoise") != std::string::npos) {
                    if (sub_line.find("off") != std::string::npos) {
                        sip_open_denoise_ = false;
                        spdlog::info("sip_open_denoise=false");                    
                    } else {
                        sip_open_denoise_ = true;
                        spdlog::info("sip_open_denoise=true");                    
                    }

                } else if (sub_line.find("open_save_video") != std::string::npos) {
                    if (sub_line.find("off") != std::string::npos) {
                        sip_open_save_video_ = false;
                        spdlog::info("sip_open_save_video=false");                    
                    } else {
                        sip_open_save_video_ = true;
                        spdlog::info("sip_open_save_video=true");                    
                    }

                } else if (sub_line.find("video_length_by_minute") != std::string::npos) {
                    sip_video_length_by_minute_ = std::stoi(std::string(sub_line.begin() + 23, sub_line.end()).c_str());
                    spdlog::info("video_length_by_minute={}", sip_video_length_by_minute_);                    
                
                } else if (sub_line.find("log_level") != std::string::npos) {
                    sip_log_level_ = std::string(sub_line.begin() + 10, sub_line.end());
                    spdlog::info("sip_log_level={}", sip_log_level_);                    

                } else if (sub_line.find("monitor_rtmp_url") != std::string::npos) {
                    sip_monitor_rtmp_url_ = std::string(sub_line.begin() + 17, sub_line.end());
                    spdlog::info("sip_monitor_rtmp_url={}", sip_monitor_rtmp_url_);                    

                } else if (sub_line.find("live_rtmp_url") != std::string::npos) {
                    sip_live_rtmp_url_ = std::string(sub_line.begin() + 14, sub_line.end());
                    spdlog::info("sip_live_rtmp_url={}", sip_live_rtmp_url_);                    

                } else if (sub_line.find("http_record_prefix") != std::string::npos) {
                    sip_http_record_prefix_ = std::string(sub_line.begin() + 19, sub_line.end());
                    spdlog::info("sip_http_record_prefix={}", sip_http_record_prefix_);                    

                } else if (sub_line.find("sip_subscribe_topic") != std::string::npos) {
                    sip_subscribe_topic_ = std::string(sub_line.begin() + 20, sub_line.end());
                    spdlog::info("sip_subscribe_topic={}", sip_subscribe_topic_);                    

                } else if (sub_line.find("rtp_keepalive_timeout") != std::string::npos) {
                    rtp_keepalive_timeout_ = std::stoi(std::string(sub_line.begin() + 22, sub_line.end()));
                    spdlog::info("rtp_keepalive_timeout={}", rtp_keepalive_timeout_);                    

                } else if (sub_line.find("rtp_max_push_stream_nums") != std::string::npos) {
                    rtp_max_push_stream_nums_ = std::stoi(std::string(sub_line.begin() + 25, sub_line.end()));
                    spdlog::info("rtp_max_push_stream_nums_={}", rtp_max_push_stream_nums_);                    

                } else if (sub_line.find("rtp_best_push_stream_nums") != std::string::npos) {
                    rtp_best_push_stream_nums_ = std::stoi(std::string(sub_line.begin() + 26, sub_line.end()));
                    spdlog::info("rtp_best_push_stream_nums={}", rtp_best_push_stream_nums_);                    

                } else if (sub_line.find("candidate_ip") != std::string::npos) {
                    candidate_ip_ = std::string(sub_line.begin() + 13, sub_line.end());
                    spdlog::info("candidate_ip={}", candidate_ip_);                    

                } else if (sub_line.find("candidate_port") != std::string::npos) {
                    candidate_port_ = std::stoi(std::string(sub_line.begin() + 15, sub_line.end()));
                    spdlog::info("candidate_port={}", candidate_port_);                    

				} else {
                    break;
                }
            }
        } else if(line.find("[storage]") != std::string::npos) {
            while (getline(file, sub_line)) {
                if (sub_line.find("AccessKeyId") != std::string::npos) {
                    accessKeyId_ = std::string(sub_line.begin() + 12, sub_line.end());
                    spdlog::info("accessKeyId={}", accessKeyId_);                    
                
                } else if (sub_line.find("save_way") != std::string::npos) {
                    save_way_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("save_way={}", save_way_);                    

                } else if (sub_line.find("AccessKeySecret") != std::string::npos) {
                    accessKeySecret_ = std::string(sub_line.begin() + 16, sub_line.end());
                    spdlog::info("accessKeySecret={}", accessKeySecret_);                    
                    
                } else if (sub_line.find("Endpoint") != std::string::npos) {
                    endpoint_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("endpoint={}", endpoint_);                    

                } else if (sub_line.find("BucketName") != std::string::npos) {
                    bucketName_ = std::string(sub_line.begin() + 11, sub_line.end());
                    spdlog::info("bucketName={}", bucketName_);                    

                } else if (sub_line.find("BasePath") != std::string::npos) {
                    basePath_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("basePath={}", basePath_);                    

                } else if (sub_line.find("save_monitor_video") != std::string::npos) {
                    if (sub_line.find("off") != std::string::npos) {
                        save_monitor_video_ = false;
                        spdlog::info("save_monitor_video=false");                    
                    } else {
                        save_monitor_video_= true;
                        spdlog::info("save_monitor_video=true");                    
                    }

                } else if (sub_line.find("save_live_video") != std::string::npos) {
                    if (sub_line.find("off") != std::string::npos) {
                        save_live_video_ = false;
                        spdlog::info("save_live_video=false");                    
                    } else {
                        save_live_video_= true;
                        spdlog::info("save_live_video=true");                    
                    }

                } else if (sub_line.find("save_snap") != std::string::npos) {
                    if (sub_line.find("off") != std::string::npos) {
                        save_snap_ = false;
                        spdlog::info("save_snap=false");                    
                    } else {
                        save_snap_ = true;
                        spdlog::info("save_snap=true");                    
                    }

                } else if (sub_line.find("snap_interval") != std::string::npos) {
                    snap_interval_ = std::stoi(std::string(sub_line.begin() + 14, sub_line.end()));
                    spdlog::info("snap_interval={}", snap_interval_);                    

                } else {
                    break;
                }
            }
        } else if(line.find("[http]") != std::string::npos) {
            while (getline(file, sub_line)) {
                if (sub_line.find("sip_http_port") != std::string::npos) {
                    sip_http_port_ = std::string(sub_line.begin() + 14, sub_line.end());
                    spdlog::info("sip_http_port_={}", sip_http_port_);                    
                } else if (sub_line.find("rtp_http_port") != std::string::npos) {
                    rtp_http_port_ = std::string(sub_line.begin() + 14, sub_line.end());
                    spdlog::info("rtp_http_port_={}", rtp_http_port_);                    
                } else {
                    break;
                }
            }
        } else if(line.find("[mysql]") != std::string::npos) {
            while (getline(file, sub_line)) {
                if (sub_line.find("host") != std::string::npos) {
                    mysql_host_ = std::string(sub_line.begin() + 5, sub_line.end());
                    spdlog::info("mysql_host={}", mysql_host_);                    

                } else if (sub_line.find("port") != std::string::npos) {
                    mysql_port_ = atoi(std::string(sub_line.begin() + 5, sub_line.end()).c_str());
                    spdlog::info("mysql_port={}", mysql_port_);                    

                } else if(sub_line.find("database") != std::string::npos) {
                    mysql_database_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("mysql_database={}", mysql_database_);                    

                } else if (sub_line.find("username") != std::string::npos) {
                    mysql_username_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("mysql_username={}", mysql_username_);                    

                } else if (sub_line.find("password") != std::string::npos) {
                    mysql_password_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("mysql_password={}", mysql_password_);                    

                } else {
                    break;
                }
            }
        } else if(line.find("[redis]") != std::string::npos) {
            while (getline(file, sub_line)) {
                if (sub_line.find("host") != std::string::npos) {
                    redis_host_ = std::string(sub_line.begin() + 5, sub_line.end());
                    spdlog::info("redis_host={}", redis_host_);                    

                } else if (sub_line.find("port") != std::string::npos) {
                    redis_port_ = atoi(std::string(sub_line.begin() + 5, sub_line.end()).c_str());
                    spdlog::info("redis_port={}", redis_port_);                    

                } else if (sub_line.find("username") != std::string::npos) {
                    redis_username_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("redis_username={}", redis_username_);                    

                } else if (sub_line.find("password") != std::string::npos) {
                    redis_password_ = std::string(sub_line.begin() + 9, sub_line.end());
                    spdlog::info("redis_password={}", redis_password_);                    

                } else if(sub_line.find("enterprise_stream_topic") != std::string::npos) {
                    redis_enterprise_stream_topic_ = std::string(sub_line.begin() + 24, sub_line.end());
                    spdlog::info("redis_enterprise_stream_topic={}", redis_enterprise_stream_topic_);                    

                } else {
                    break;
                }
            }
        } else {
            continue;
        }
    }

    file.close();
}


ServerType ServerConfig::judge_server_type(void) {
    if (!sip_wai_ip_.empty() &&
        !sip_serial_.empty() &&
        !sip_realm_.empty() &&
        !sip_password_.empty()) {
        return ServerType::SIP_SERVER;
    
    } else if (!recv_rtp_wai_ip_.empty() &&
	           !rtp_http_port_.empty()) {
        return ServerType::RTP_SERVER;

    } else {
        return ServerType::UNKNOWN;
    }
}
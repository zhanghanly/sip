#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <ctime>

enum class ServerType {
    SIP_SERVER,
    RTP_SERVER,
    UNKNOWN
};


class ServerConfig {
public:
    static ServerConfig* get_instance();
    /*load conf file*/
    void init_conf(const std::string& conf_file = "../conf/server.conf");
    ServerType judge_server_type();

    /*sip*/
    int sip_port() { return sip_port_; }
    int rtp_fix_recv_stream_port() { return rtp_fix_recv_stream_port_; }
    std::string recv_rtp_wai_ip() { return recv_rtp_wai_ip_; }
    std::string sip_wai_ip() { return sip_wai_ip_; }
    std::string sip_serial() { return sip_serial_; }
    std::string sip_realm() { return sip_realm_; }
    std::string sip_password() { return sip_password_; }
    std::time_t sip_rtp_timeout() { return sip_rtp_timeout_; }
    std::time_t sip_catalog_interval() { return sip_catalog_interval_; }
    std::time_t sip_keepalive_timeout() { return sip_keepalive_timeout_; }
    bool sip_open_audio() { return sip_open_audio_; }
    bool sip_open_denoise() { return sip_open_denoise_; }
    bool sip_open_save_video() { return sip_open_save_video_; }
    int sip_video_length_by_minute() { return sip_video_length_by_minute_; }
    std::string sip_log_level() { return sip_log_level_; }
    bool tcp_recv_rtp() { return tcp_recv_rtp_; }
    std::string sip_monitor_rtmp_url() { return sip_monitor_rtmp_url_; }
    std::string sip_live_rtmp_url() { return sip_live_rtmp_url_; }
    std::string sip_http_record_prefix() { return sip_http_record_prefix_; }
    std::string sip_subscribe_topic() { return sip_subscribe_topic_;}
    int rtp_keepalive_timeout() { return rtp_keepalive_timeout_; }
    int rtp_max_push_stream_nums() { return rtp_max_push_stream_nums_; }
    int rtp_best_push_stream_nums() { return rtp_best_push_stream_nums_; }
	std::string candidate_ip() { return candidate_ip_; }
	int candidate_port() { return candidate_port_; }

    /*fileStorage*/
    std::string fileStorage_save_way() { return save_way_; }
    std::string fileStorage_accessKeyId() { return accessKeyId_; }
    std::string fileStorage_accessKeySecret() { return accessKeySecret_; }
    std::string fileStorage_endpoint() { return endpoint_; }
    std::string fileStorage_bucketName() { return bucketName_; }
    std::string fileStorage_basePath() { return basePath_; }
    bool fileStorage_save_monitor_video() { return save_monitor_video_; }
    bool fileStorage_save_live_video() { return save_live_video_; }
    bool fileStorage_save_snap() { return save_snap_; }
    std::time_t fileStorage_snap_interval() { return snap_interval_; }

    /*http*/
    std::string sip_http_port() { return sip_http_port_; }
    std::string rtp_http_port() { return rtp_http_port_; }

    /*mysql*/
    std::string mysql_host() { return mysql_host_; }
    int mysql_port() { return mysql_port_; }
    std::string mysql_database() { return mysql_database_; }
    std::string mysql_usernaem() { return mysql_username_; }
    std::string mysql_password() { return mysql_password_; }

    /*redis*/
    std::string redis_host() { return redis_host_; }
    int redis_port() { return redis_port_; }
    std::string redis_usernaem() { return redis_username_; }
    std::string redis_password() { return redis_password_; }
    std::string redis_enterprise_stream_topic() { return redis_enterprise_stream_topic_; }

private:
    ServerConfig()=default;

private:
    /*sip*/
    int sip_port_;
    int rtp_fix_recv_stream_port_;
    std::string recv_rtp_wai_ip_;
    std::string sip_wai_ip_;
    std::string sip_serial_;
    std::string sip_realm_;
    bool tcp_recv_rtp_;    
    std::string sip_password_;
    std::time_t sip_rtp_timeout_;
    std::time_t sip_catalog_interval_;
    std::time_t sip_keepalive_timeout_;
    bool sip_open_audio_;
    bool sip_open_denoise_;
    bool sip_open_save_video_;
    int sip_video_length_by_minute_;
    std::string sip_log_level_;
    std::string sip_monitor_rtmp_url_;
    std::string sip_live_rtmp_url_;
    std::string sip_http_record_prefix_;
    std::string sip_subscribe_topic_;
    int rtp_keepalive_timeout_;
    int rtp_max_push_stream_nums_;
    int rtp_best_push_stream_nums_;
	std::string candidate_ip_;
	int candidate_port_;

    /*fileStorage*/
    std::string save_way_;
    std::string accessKeyId_;
    std::string accessKeySecret_;
    std::string endpoint_;
    std::string bucketName_;
    std::string basePath_;
    bool save_monitor_video_;
    bool save_live_video_;
    bool save_snap_;
    std::time_t snap_interval_;

    /*http*/
    std::string sip_http_port_;
    std::string rtp_http_port_;

    /*mysql*/
    std::string mysql_host_;
    int mysql_port_;
    std::string mysql_database_;
    std::string mysql_username_;
    std::string mysql_password_;

    /*redis*/
    std::string redis_host_;
    int redis_port_;
    std::string redis_username_;
    std::string redis_password_;
    std::string redis_enterprise_stream_topic_;
};

#endif
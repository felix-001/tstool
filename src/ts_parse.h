// Last Update:2019-03-26 20:53:49
/**
 * @file ts_parse.h
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-03-26
 */

#ifndef TS_PARSE_H
#define TS_PARSE_H

typedef struct{
    u8 adaption_field_length                :8;
    u8 discontinuity_indicator              :1;
    u8 random_access_indicator              :1;
    u8 elementary_stream_priority_indicator :1;
    u8 PCR_flag                             :1;
    u8 OPCR_flag                            :1;
    u8 splicing_point_flag                  :1;
    u8 transport_private_data_flag          :1;
    u8 adaption_file_extension_flag         :1;
} adp_field_t;

typedef struct {
    u16 pmt_pid;
    u16 audio_pid;
    u16 video_pid;
} ts_info_t;

int ts_parse( u8 *ts_data, u32 packet_nr );

#endif  /*TS_PARSE_H*/

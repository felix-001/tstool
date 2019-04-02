// Last Update:2019-03-27 05:17:13
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
#ifdef WORDS_BIGENDIAN
    u8                                      :2;
    u8 PES_scrambling_control               :2;
    u8 PES_priority                         :1;
    u8 data_alignment_indicator             :1;
    u8 copyright                            :1;
    u8 original_or_copy                     :1;
    u8 PTS_DTS_flag                         :2;
    u8 ESCR_flag                            :1;
    u8 ES_rate_flag                         :1;
    u8 DSM_trick_mode_flag                  :1;
    u8 additional_copyright_info_flag       :1;
    u8 PES_crc_flag                         :1;
    u8 PES_extension_flag                   :1;
    u8 PES_header_data_length               :8;
#else
    u8 PES_scrambling_control               :2;
    u8 PES_priority                         :1;
    u8 data_alignment_indicator             :1;
    u8 copyright                            :1;
    u8 original_or_copy                     :1;
    u8                                      :2;
    u8 ESCR_flag                            :1;
    u8 ES_rate_flag                         :1;
    u8 DSM_trick_mode_flag                  :1;
    u8 additional_copyright_info_flag       :1;
    u8 PES_crc_flag                         :1;
    u8 PES_extension_flag                   :1;
    u8 PTS_DTS_flag                         :2;
    u8 PES_header_data_length               :8;
#endif
} optional_pes_header_t;

typedef struct {
    u16 pmt_pid;
    u16 audio_pid;
    u16 video_pid;
} ts_info_t;

#define NALU_TYPE_SLICE     1
#define NALU_TYPE_DPA       2
#define NALU_TYPE_DPB       3
#define NALU_TYPE_DPC       4
#define NALU_TYPE_IDR       5
#define NALU_TYPE_SEI       6
#define NALU_TYPE_SPS       7
#define NALU_TYPE_PPS       8
#define NALU_TYPE_AUD       9
#define NALU_TYPE_EOSEQ     10
#define NALU_TYPE_EOSTREAM  11
#define NALU_TYPE_FILL      12


int ts_parse( u8 *ts_data, u32 packet_nr );

#endif  /*TS_PARSE_H*/

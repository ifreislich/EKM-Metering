/*
 * Copyright (c) 2012-2021 Ian Freislich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

static const uint16_t const ekmcrclut[256] = {
	0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
	0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
	0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
	0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
	0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
	0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
	0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
	0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
	0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
	0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
	0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
	0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
	0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
	0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
	0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
	0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
	0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
	0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
	0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
	0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
	0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
	0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
	0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
	0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
	0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
	0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
	0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
	0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
	0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
	0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
	0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
	0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

#define	EKM_PASSWORD	"\01P1\02(%s)\03"
#define	EKM_TIME	"\01W1\02" "0060(%02d%02d%02d%02d%02d%02d%02d)\03"
#define	EKM_METER_OPEN	"/?%012llu!\r\n"
#define	EKM_METER_CLOSE	"\x01" "B0\x03\x75"
#define	EKM_6MONTH_TOTAL	"\x01R1\x02" "0011\x03"
#define	EKM_6MONTH_REV	"\x01R1\x02" "0012\x03"
#define	EKM_SCHEDULE1	"\x01R1\x02" "0070\x03"
#define	EKM_SCHEDULE2	"\x01R1\x02" "0071\x03"
#define	EKM_SCHEDULE_HOLIDAY	"\x01R1\x02" "00B0\x03"

struct _tou_meter {
	char	 total_kwh[8];
	char	 tou[4][8];
} __attribute__ ((packed));

/*
 * Meter response to Open
 */
struct _ekmv3reply {
	char			 start;
	char			 model[2];
	char			 firmware;
	char			 address[12];
	struct _tou_meter	 total;
	struct _tou_meter	 reverse;
	char			 volts[3][4];
	char			 amps[3][5];
	char			 power[3][7];
	char			 total_power[7];
	char			 pf[3][4];
	char			 max_demand[7];
	char			 pad1;
	char			 demand_period;
	char			 date[14];
	char			 ct_size[4];
	char			 pulse[3][8];
	char			 pulseratio[3][4];
	char			 pad2[3];
	char			 pulse_h_l[3];
	char			 reserved1[17];
	char			 pad3[3];
	char			 end;
	u_int16_t		 crc;
} __attribute__ ((packed));

struct _ekm_schedule_entry {
	char	 start[4];
	char	 pad;
	char	 rate;
} __attribute__ ((packed));

struct _ekm_schedule_table {
	struct _ekm_schedule_entry	 table[5];
	char				 pad[24];
} __attribute__ ((packed));

/*
 * Response to Read Period Tables
 */
struct period_table {
	char				 start;
	char				 header[4];
	char				 delim_start;
	struct _ekm_schedule_table	 schedule[4];
	struct _ekm_schedule_table	 season;
	char				 pad[5];
	char				 delim_end;
	char				 end;
	u_int16_t			 crc;
} __attribute__ ((packed));

/*
 * meter response to read Holidays
 */
struct _ekm_meter_holidays {
	char		 start;
	char		 header[4];
	char		 delim_start;
	char		 holidays[20][4];
	char		 weekend_sched[2];
	char		 holiday_sched[2];
	char		 reserved[161];
	char		 delim_end;
	char		 end;
	u_int16_t	 crc;
} __attribute__ ((packed));

/*
 * Meter response to read last 6 months history.
 * Total and Reverse responses use this structure.
 */
struct _ekm_meter_history {
	char			 start;
	char			 header[4];
	char			 delim_start;
	struct _tou_meter	 month[6];
	char			 reserved[6];
	char			 delim_end;
	char			 end;
	u_int16_t		 crc;
} __attribute__ ((packed));

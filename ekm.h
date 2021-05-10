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

struct meter_tou {
	double	total;
	double	tou[4];
};

struct meter_response {
	u_int64_t		address;
	int			firmware;
	struct meter_tou	forward;
	struct meter_tou	reverse;
	double			volts[3];
	double			amps[3];
	int			power[3];
	int			total_power;
	double			pf[3];
	u_int64_t		max_demand;
	time_t			demand_period;
	time_t			time;
	int			ct_size;
	u_int64_t		pulse[3];
	int			pulseratio[3];
	char			pulsetrigger[3];
};

struct meter_history {
	struct meter_tou forward[6];
	struct meter_tou reverse[6];
};

struct meter_schedule {
	struct {
		u_int8_t	 hour;
		u_int8_t	 min;
		u_int8_t	 rate;
	} schedule[6];
	struct {
		u_int8_t	 month;
		u_int8_t	 day;
		u_int8_t	 schedule;
	} season[4];
	struct {
		u_int8_t	month;
		u_int8_t	day;
	} hoiday[20];
	u_int8_t	weekend_schedule;
	u_int8_t	holiday_schedule;
};

uint16_t ekmcrc(const void const *, uint16_t);
void ekm_flush(int);
int meter_open(int, struct meter_response *, u_int64_t);
int meter_login(int, char *);
void meter_close(int);
int readhistory(int, struct meter_history *);
int set_time(int);
int scheduleread(int, struct meter_schedule *);

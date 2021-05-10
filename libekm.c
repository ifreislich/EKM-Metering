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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "ekmprivate.h"
#include "ekm.h"

char *strdecpy(char *, const char *, size_t, size_t);

uint16_t
ekmcrc(const void const *dat, uint16_t len)
{
	uint16_t crc = 0xffff;

	while (len--) {
		crc = (crc >> 8) ^ ekmcrclut[(crc ^ *(uint8_t*)dat++) & 0xff];
	}

	crc = (crc << 8) | (crc >> 8);
	crc &= 0x7f7f;

	return crc;
}

/*
 * Copy a numeric string inserting a '.' at dst[dec]
 */
char *
strdecpy(char *dst, const char *src, size_t len, size_t dec)
{
	int	d, s;

	for(s = 0, d = 0; s < len; s++, d++) {
		if (dec > 0 && s == dec)
			dst[d++] = '.';
		dst[d] = src[s];
	}
	dst[d] = '\0';

	return(dst);
}

void
ekm_flush(int connection)
{
	char	 buffer[10];

	for (;;)
		if (read(connection, buffer, 10) <= 0)
			return;

}

ssize_t
ekm_read(int connection, void *buffer, size_t nbytes)
{
	struct pollfd	 pollfd;
	ssize_t		 got, len;

	pollfd.fd = connection;
	pollfd.events = POLLRDNORM;

	for (got = 0; got < nbytes;) {
		switch (poll(&pollfd, 1, 1000)) {
		    case -1:
			if (errno == EINTR)
				continue;
			return(-1);
			break;
		    case 0:
			return(-1);
			break;
		}
		len = read(connection, buffer + got, nbytes - got);
		if (len < 0 && !(errno || EINTR|EAGAIN))
			return(-1);
		got += len;
	}
	if (got != nbytes) {
		printf("ekm_read(): Failed read\n");
	}
	return(got);
}

int
read_response(int connection, void *buffer, size_t nbytes)
{
	size_t		 got;
	u_int16_t	 crc;

	got = ekm_read(connection, buffer, nbytes);
	if (got < 0)
		return(got);

	if (got != nbytes) {
		printf("read_response(): Failed read\n");
		return(-1);
	}

	crc = ekmcrc(buffer + 1, got - (sizeof(crc) + 1));
	return(crc == ntohs(*(u_int16_t *)&((char*)buffer)[got -sizeof(crc)]));
}

void
ekm_tou_cvt(struct _tou_meter *in, struct meter_tou *out)
{
	char	 buffer[260];
	int	 i;

	strdecpy(buffer, in->total_kwh, 8, 7);
	sscanf(buffer, "%lf", &out->total);
	for (i = 0; i < 4; i++) {
		strdecpy(buffer, in->tou[i], 8, 7);
		sscanf(buffer, "%lf", &out->tou[i]);
	}
}

/*
 * Open the meter and read the response.
 * XXX the read needs a poll loop and timeout.
 * XXX need to flush input before we start since there might be
 *     UART data stuck in the read buffer on the remote end.
 */
int
meter_open(int connection, struct meter_response *response, u_int64_t meter)
{
	struct tm		 tm;
	struct _ekmv3reply	 reply;
	char			 buffer[260];
	int			 i, len, result;

	ekm_flush(connection);
	write(connection, EKM_METER_CLOSE, strlen(EKM_METER_CLOSE));
	len = sprintf(buffer, EKM_METER_OPEN, meter);
	write(connection, buffer, len);
	result = read_response(connection, &reply, 255);
	if (result < 1) {
		ekm_flush(connection);
		return(result);
	}

	response->address = meter;
	response->firmware = reply.firmware;
	ekm_tou_cvt(&reply.total, &response->forward);
	ekm_tou_cvt(&reply.reverse, &response->reverse);
	response->forward.total -= response->reverse.total;
	for(i = 0; i < 4; i++)
		response->forward.tou[i] -= response->reverse.tou[i];

	strdecpy(buffer, reply.total_power, 7, 0);
	sscanf(buffer, "%d", &response->total_power);

	for(i = 0; i < 3; i++) {
		strdecpy(buffer, reply.volts[i], 4, 3);
		sscanf(buffer, "%lf", &response->volts[i]);
		strdecpy(buffer, reply.amps[i], 5, 4);
		sscanf(buffer, "%lf", &response->amps[i]);
		strdecpy(buffer, reply.power[i], 7, 0);
		sscanf(buffer, "%d", &response->power[i]);
		strdecpy(buffer, reply.pf[i], 4, 2);
		sscanf(buffer + 1, "%lf", &response->pf[i]);
		if (*buffer == 'C')
			response->pf[i] *= -1;
		strdecpy(buffer, reply.pulse[i], 8, 0);
		sscanf(buffer, "%llu", &response->pulse[i]);
		strdecpy(buffer, reply.pulseratio[i], 8, 0);
		sscanf(buffer, "%d", &response->pulseratio[i]);
		response->pulsetrigger[i] = reply.pulse_h_l[i];
	}

	strdecpy(buffer, reply.max_demand, 7, 0);
	sscanf(buffer, "%llu", &response->max_demand);

	response->demand_period = reply.demand_period;

	strdecpy(buffer, reply.date, 14, 0);
	buffer[7] = '0'; /* Date doesn't conform to ISO/IEC 9899:1990 */
	memset(&tm, '\0', sizeof(tm));
	strptime(buffer, "%y%m%d00%H%M%S", &tm);
	response->time = mktime(&tm);

	strdecpy(buffer, reply.ct_size, 4, 0);
	response->ct_size = atoi(buffer);

	return(result);
}

int
meter_login(int connection, char *password)
{
	u_int16_t	 crc;
	char		 buffer[260];
	int		 len, result;

	len = sprintf(buffer, EKM_PASSWORD, password);
	crc = ekmcrc(buffer + 1, len - 1);
	*(u_int16_t *)(buffer+len) = htons(crc);
	write(connection, buffer, len+2);
	if ((result = ekm_read(connection, buffer, 1)) < 0)
		return(result);
	return(*buffer == '\x06');
}

void
meter_close(int connection)
{
	write(connection, EKM_METER_CLOSE, strlen(EKM_METER_CLOSE));
}


/* Read 6 months ToU data
 */
int
readhistory(int con, struct meter_history *history)
{
	struct _ekm_meter_history	 history_total;
	struct _ekm_meter_history	 history_rev;
	char		 buffer[255];
	u_int16_t	 crc;
	int		 len, result, i, j;

	strcpy(buffer, EKM_6MONTH_TOTAL);
	len = strlen(buffer);
	crc = ekmcrc(buffer + 1, len - 1);
	*(u_int16_t *)(buffer+len) = htons(crc);
	write(con, buffer, len+2);
	if ((result = read_response(con, &history_total, 255)) != 1)
		return(result);
	strcpy(buffer, EKM_6MONTH_REV);
	len = strlen(buffer);
	crc = ekmcrc(buffer + 1, len - 1);
	*(u_int16_t *)(buffer+len) = htons(crc);
	write(con, buffer, len+2);
	if ((result = read_response(con, &history_rev, 255)) != 1)
		return(result);

	for (i = 0; i < 6; i++) {
		ekm_tou_cvt(&history_total.month[i], &history->forward[i]);
		ekm_tou_cvt(&history_rev.month[i], &history->reverse[i]);
		history->forward[i].total -= history->reverse[i].total;
		for (j = 0; j < 4; j++)
			history->forward[i].tou[j]-=history->reverse[i].tou[j];
			
	}
	return (1);
}

int
scheduleread(int con, struct meter_schedule *sched)
{
	char		 buffer[255];
	u_int16_t	 crc;
	int		 len;

	strcpy(buffer, EKM_SCHEDULE2);
	len = strlen(buffer);
	crc = ekmcrc(buffer + 1, len - 1);
	*(u_int16_t *)(buffer+len) = htons(crc);
	write(con, buffer, len+2);

	read_response(con, buffer, 255);
	write(1, buffer, 255);
	return(1);
}

int
set_time(int con)
{
	char		 buffer[255];
	u_int16_t	 crc;
	int		 len;
	struct tm	*tv;
	time_t		 clock;

	clock = time(NULL);
	tv = localtime(&clock);
	len = sprintf(buffer, EKM_TIME, tv->tm_year - 100,tv->tm_mon + 1,
	    tv->tm_mday, tv->tm_wday + 1, tv->tm_hour, tv->tm_min, tv->tm_sec);
	crc = ekmcrc(buffer + 1, len - 1);
	*(u_int16_t *)(buffer+len) = htons(crc);
	write(con, buffer, len+2);
	ekm_read(con, buffer, 1);

	return(*buffer == '\x06');
}

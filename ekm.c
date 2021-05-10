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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#include "ekm.h"

#undef ISERIAL

int
main(int argc, char **argv)
{
	FILE			*fp = NULL;
#ifdef ISERIAL
	struct sockaddr_in	 server;
	struct protoent		*prot;
	char			*address = "192.168.88.17";
#else
	struct termios		 old_tios, tios;
#endif
	struct meter_response	 reply;
	struct meter_history 	 history;
	struct meter_schedule	 schedule;
	struct meter_tou	*a, *b;
	struct timespec		 ts;
	struct kevent		 event, evlist;
	struct stat		 sb;
	#define	NUM_METERS	1
	u_int64_t		 meter[NUM_METERS] = { 13491 };
	time_t			 clock;
	char			*password = "00000000";
	char			*workdir = "/home/ianf/graphing/";
	char			*log = "ekm-imhoff.pending";
	char			 path[1024];
	int			 con, i, j, eventq, result;

	openlog("ekmreader", LOG_PID | LOG_CONS, LOG_DAEMON);

#ifdef ISERIAL
	prot = getprotobyname("TCP");
	con = socket(AF_INET, SOCK_STREAM, prot->p_proto);
	server.sin_family = AF_INET;
	inet_aton(address, &server.sin_addr);
	server.sin_port = htons(50000);

	if (connect(con, (struct sockaddr *)&server, sizeof(server))) {
		printf("Connection failed\n");
		exit(EX_OSERR);
	}
#else
	con = open("/dev/cuaU0", O_RDWR | O_NOCTTY | O_EXCL);
	tcgetattr(con, &old_tios);
	memset(&tios, '\0', sizeof(struct termios));
	cfsetispeed(&tios, B9600);
	cfsetospeed(&tios, B9600);
	tios.c_cflag |= (CREAD | CLOCAL);
	tios.c_cflag &= ~CSIZE;
	tios.c_cflag |= CS7;	/* 7 Bits */
	tios.c_cflag &= ~CSTOPB; /* 1 Stop Bit */
	tios.c_cflag |= PARENB; /* Even Parity */
	tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tios.c_iflag &= INPCK | ISTRIP;
	tios.c_iflag &= ~(IXON | IXOFF | IXANY);
	tios.c_oflag &= ~OPOST;	/* Raw ouput */
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 1;
	if (tcsetattr(con, TCSANOW, &tios) < 0) {
		perror("tcsetattr\n");
		exit(-1);
	}
#endif

	fcntl(con, F_SETFL, O_NONBLOCK);
	ekm_flush(con);

	eventq = kqueue();
	EV_SET(&event, 1, EVFILT_TIMER, EV_ADD|EV_ENABLE, 0, 1000, NULL);
	ts.tv_sec=0;
	ts.tv_nsec=0;
	kevent(eventq, &event, 1, &evlist, 1, &ts);

	for(;kevent(eventq, NULL, 0, &evlist, 1, NULL) != -1;) {
	 	if (evlist.data > 1)
			syslog(LOG_NOTICE, "Missed %d events", evlist.data - 1);
		strcpy(path, workdir);
		strcat(path, log);
		if ((fp = fopen(path, "a")) == NULL)
			continue;
		for (j = 0; j < NUM_METERS; j++) {
		/* We record the system time after opening the meter so we have
		 * the time as close to when the meter generates the response..
		 */
		result = meter_open(con, &reply, meter[j]);
		switch (result) {
		    case 0:
			syslog(LOG_NOTICE, "Bad CRC on meter %llu", meter[j]);
			goto close;
			break;
		    case -1:
			syslog(LOG_NOTICE, "Read timed out on meter %llu",
			    meter[j]);
			goto close;
			break;
		    default:
			break;
		}
		clock = time(NULL);

		/*
		 * Allow a clock drift of up to 3 seconds, otherwise, set the
		 * time
		 */
		if (abs(clock - reply.time) >= 3) {
			syslog(LOG_NOTICE, "Meter %llu clock drift "
			    "too large %d\n", meter[j],
			    clock - reply.time);
			/* Supply the password */
			result = meter_login(con, password);
			if (result < 0) {
				syslog(LOG_NOTICE, "Read timed out on "
				    "meter %llu", meter[j]);
				goto close;
			}
			if (result != 1) {
				syslog(LOG_NOTICE, "Wrong password for "
				    "meter %llu", meter[j]);
				goto close;
			}
			set_time(con);
		}

		fprintf(fp, "%d\n", clock);
		fprintf(fp, "meter: %llu %d\n", meter[j], reply.firmware);
		for(i = 0; i <= 2; i++)
	 		fprintf(fp, "L%d Volts: %-5.1lf\n", i+1, reply.volts[i]);

		for(i = 0; i <= 2; i++)
			fprintf(fp, "L%d Amps: %-6.1lf\n", i+1, reply.amps[i]);

		for(i = 0; i <= 2; i++)
			fprintf(fp, "L%d Power: %-8.1d\n", i+1, reply.power[i]);

		for(i = 0; i <= 2; i++)
			fprintf(fp, "L%d PF: %4.2lf\n", i + 1, reply.pf[i]);
		fprintf(fp, "CT Size: %d\n", reply.ct_size);

		fprintf(fp, "Frw kWh: %-9.1lf\n", reply.forward.total);
		fprintf(fp, "Rev kWh: %-9.1lf\n", reply.reverse.total);
		fprintf(fp, "Demand: %-9llu\n", reply.max_demand);
		fprintf(fp, "Demand Period: %c\n", reply.demand_period);

		sprintf(path, "%sreadhistory.%llu", workdir, meter[j]);
		if (!stat(path, &sb) && readhistory(con, &history)) {
			a = &reply.forward;
			b = &history.forward[0];
			fprintf(fp, "Current fwd:    "
			    "%-8.1lf %-8.1lf %-8.1lf %-8.1lf %-8.1lf\n",
			    a->total - b->total, a->tou[0] - b->tou[0],
			    a->tou[1] - b->tou[1], a->tou[2] - b->tou[2],
			    a->tou[3] - b->tou[3]);
			a = &reply.reverse;
			b = &history.reverse[0];
			fprintf(fp, "Current rev:    "
			    "%-8.1lf %-8.1lf %-8.1lf %-8.1lf %-8.1lf\n",
			    a->total - b->total, a->tou[0] - b->tou[0],
			    a->tou[1] - b->tou[1], a->tou[2] - b->tou[2],
			    a->tou[3] - b->tou[3]);
			for (i = 0; i < 5; i++) {
				a = &reply.forward;
				b = &history.forward[i];
				fprintf(fp, "History fwd -%d: "
				    "%-8.1lf %-8.1lf %-8.1lf %-8.1lf %-8.1lf\n",
				    i+1, a->total - b->total,
				    a->tou[0]-b->tou[0], a->tou[1]-b->tou[1],
				    a->tou[2]-b->tou[2], a->tou[3]-b->tou[3]);
				a = &reply.reverse;
				b = &history.reverse[i];
				fprintf(fp, "History rev -%d: "
				    "%-8.1lf %-8.1lf %-8.1lf %-8.1lf %-8.1lf\n",
				    i+1, a->total - b->total,
				    a->tou[0]-b->tou[0], a->tou[1]-b->tou[1],
				    a->tou[2]-b->tou[2], a->tou[3]-b->tou[3]);
			}
			unlink(path);
		}
		//scheduleread(con, &schedule);

close:
		/* Close the meter connection */
		meter_close(con);
		}
		fclose(fp);
	}

	exit(EX_OK);
}

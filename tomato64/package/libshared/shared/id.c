/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <trxhdr.h>
#include "shared.h"

/*
				HW_*                  boardtype    boardnum  boardrev  boardflags  others
				--------------------- ------------ --------- --------- ----------- ---------------
EA6400		    		BCM4708               0x0646       01        0x1100    0x0110      0:devid=0x43A9
EA6500v2	    		BCM4708               0xF646       01        0x1100    0x0110      0:devid=0x4332
EA6700		    		BCM4708               0xF646       01        0x1100    0x0110      0:devid=0x4332
EA6900		    		BCM4708               0xD646       01        0x1100    0x0110
EA6200		    		BCM47081A0            0xE646       20130125  0x1100
EA6350v1	    		BCM47081A0            0xE646       20140309  0x1200    0x00000110  0:devid=0x43A9
EA6350v2	    		BCM4708C0             0xE646       20150309  0x1200    0x00000110  0:devid=0x43A9

WZR-1750DHP	    		BCM4708               0xF646       00        0x1100    0x0110      0:devid=0x4332

Tenda AC15			BCM4708               0x0646       30        0x1100 //1:boardnum=AC155g  HINT: model=AC15V1.0 only available for some HW-Versions, so at least 2 different cfe´s from Tenda!
Tenda AC18			BCM4708               0x0646       30        0x1100 //1:boardnum=AC18_5G  HINT: model=AC18V1.0 may be not available for some HW-Versions (better use 1:boardnum right now!)

TrendNET			BCM4708               0x0646       1234      0x1100    0x80001200

DSL-AC68U			BCM4708A0             0x0646       <MAC>     0x1100    0x00000110
RT-N18U				BCM47081A0            0x0646       00        0x1100    0x00000110
RT-AC56U			BCM4708               0x0646	   00	     0x1100    0x00000110
RT-AC68U			BCM4708               0x0646       <MAC>     0x1100    0x00001000
RT-AC67U			BCM4708               0x0646       <MAC>     0x1103    0x00001000 // odmpid=RT-AC67U
RT-AC68U C1			BCM4709C0             0x0646       <MAC>     0x1103    0x00001000
RT-AC68P			BCM4709               0x0665       <MAC>     0x1103    0x00001000
RT-AC68U V3			BCM4708C0   	      0x0646       00        0x1103    0x00000110 // odmpid="empty" (not useable, let tomato set it right)
RT-AC1900U			BCM4708C0   	      0x0646       00        0x1103    0x00000110 // odmpid=RT-AC1900U
RT-AC66U_B1			BCM4708C0   	      0x0646       00        0x1103    0x00000110 // odmpid=RT-AC66U_B1
RT-AC1750_B1			BCM4708C0   	      0x0646       00        0x1103    0x00000110 // odmpid=RT-AC1750_B1
RT-N66U_C1			BCM4708C0   	      0x0646       00        0x1103    0x00000110 // odmpid=RT-N66U_C1
RT-AC1900P			BCM4709C0   	      0x072F       00        0x1500    0x00000110 // odmpid=RT-AC1900P
RT-AC68U B2			BCM4709C0   	      0x072F       00        0x1500    0x00000110 // odmpid=RT-AC68U
RT-AC3200			BCM4709               0x072f       <MAC>     0x1101
RT-AC88U			BCM4709               0x072F       <MAC>     0x1500    0x00000110 // model=RT-AC88U odmpid=RT-AC88U
RT-AC3100			BCM4709               0x072F       <MAC>     0x1102    0x00000110 // model=RT-AC3100 (assume the same base values like RT-AC5300)
RT-AC5300			BCM4709               0x072F       <MAC>     0x1102    0x00000110 // model=RT-AC5300

R7900				BCM4709               0x0665       32        0x1101 // board_id=U12H315T30_NETGEAR
R8000				BCM4709               0x0665       32        0x1101 // board_id=U12H315T00_NETGEAR
AC1450				BCM4708               0x0646       679       0x1110 // CH/Charter version has the same signature
R6900				BCM4709               0x0665       32        0x1301    0x1000
R7000				BCM4709               0x0665       32        0x1301    0x1000
R6200v2				BCM47081A             0x0646       679       0x1110 // Single Core same board detection as R6250 amd R6300v2
R6250				BCM4708               0x0646       679       0x1110 // same as R6300v2 well we use the same MODEL definition
R6300v2				BCM4708               0x0646       679       0x1110 // CH/Charter version has the same signature
R6400				BCM4708               0x0646       32        0x1601
R6400v2				BCM4708C0             0x0646       32        0x1601
R6700v1				BCM4709               0x0665       32        0x1301    0x1000
R6700v3				BCM4708C0             0x0646       32        0x1601 // same as R6400v2
XR300				BCM4708C0             0x0646       32        0x1601 // same as R6400v2

DIR-868L			BCM4708               0x0646       24        0x1110
DIR-868LC1			BCM4708               0x0646       24        0x1101 //same as rev a/b but different boardrev
WS880				BCM4708               0x0646       1234      0x1101
R1D				BCM4709               0x0665       32        0x1301 //same as R7000
F9K1113v2			BCM40781A0            0x0646       02        0x1100    0x00000110  1:devid=0x43A9
F9K1113v2			BCM40781A0            0x0646       AC1200v2  0x1100    0x00000110  pci/2/1/devid=0x43A9 // v2 version 2000 and 2010

BFL_ENETADM	0x0080
BFL_ENETVLAN	0x0100
*/

int check_hw_type(void)
{
	const char *s;

	s = nvram_safe_get("boardtype");

	switch (strtoul(s, NULL, 0)) {
#ifdef CONFIG_BCMWL6A
	case 0x0646:	/* EA6400, R6400, R6400v2, R6700v3 */
	case 0x0665:	/* R6700v1, R6900, R7000, R1D */
	case 0xf646:	/* EA6700, WZR-1750 */
	case 0xd646:	/* EA6900 */
	case 0xe646:	/* EA6200, EA6350v1 */
	case 0x072f:	/* RT-AC5300, RT-AC88U, RT-AC3200, RT-AC1900P, RT-AC68U B2 */
		return HW_BCM4708; /* and also for 4709 right now!  */
#endif /* CONFIG_BCMWL6A */
	}

	return HW_UNKNOWN;
}

static int get_model_once(void)
{
	static int hw = 0;  /* initialize with 0 / HW_UNKNOWN */

	if (hw == 0) { /* hw unknown OR detect hw for the first time at function get_model(). */
		hw = check_hw_type();
	}

#ifdef CONFIG_BCMWL6A
	if (hw == HW_BCM4708) {
#ifdef TCONFIG_BCM714
		if ((nvram_match("model", "RT-AC88U")) || (nvram_match("productid", "RT-AC88U")) || (nvram_match("odmpid", "RT-AC88U"))) return MODEL_RTAC88U;
		if ((nvram_match("model", "RT-AC3100")) || (nvram_match("productid", "RT-AC3100")) || (nvram_match("odmpid", "RT-AC3100"))) return MODEL_RTAC3100;
#ifdef TCONFIG_AC5300
		if ((nvram_match("model", "RT-AC5300")) || (nvram_match("productid", "RT-AC5300")) || (nvram_match("productid", "RT-AC5300R"))) return MODEL_RTAC5300;
#endif
#endif /* TCONFIG_BCM714 */
#ifdef TCONFIG_AC3200
		if ((nvram_match("boardrev", "0x1101")) && (nvram_match("boardnum", "32"))) return MODEL_R8000;
		if ((nvram_match("boardrev", "0x1101")) && (nvram_match("model", "RT-AC3200"))) return MODEL_RTAC3200;
#endif
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("model", "R1D"))) return MODEL_R1D;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-N18U"))) return MODEL_RTN18U;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC56U"))) return MODEL_RTAC56U;
		if (nvram_match("odmpid", "RT-AC1900P")) return MODEL_RTAC1900P;
		if ((nvram_match("boardrev", "0x1500")) && (nvram_match("odmpid", "RT-AC68U"))) return MODEL_RTAC1900P; /* RT-AC68U B2 --> (almost) the same like RT-AC1900P */
		if (nvram_match("model", "DSL-AC68U")) return MODEL_DSLAC68U; /* DSL-AC68U */
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC68U"))) return MODEL_RTAC68U; /* RT-AC68R/U */
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U")) && (nvram_match("odmpid", "RT-N66U_C1"))) return MODEL_RTAC66U_B1; /* RT-N66U_C1 --> (almost) the same like RT-AC66U_B1 */
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U")) && (nvram_match("odmpid", "RT-AC1750_B1"))) return MODEL_RTAC66U_B1; /* RT-AC1750_B1 --> (almost) the same like RT-AC66U_B1 */
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U")) && (nvram_match("odmpid", "RT-AC66U_B1"))) return MODEL_RTAC66U_B1; /* RT-AC66U_B1 */
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U")) && (nvram_match("odmpid", "RT-AC1900U"))) return MODEL_RTAC67U; /* RT-AC1900U --> (almost) the same like RT-AC67U */
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U")) && (nvram_match("odmpid", "RT-AC67U"))) return MODEL_RTAC67U; /* RT-AC67U */
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("PA", "8527"))) return MODEL_RTAC68UV3; /* RT-AC68U V3 */
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U"))) return MODEL_RTAC68U; /* RT-AC68P/U B1 OR RT-AC68U C1 */
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H240T99_NETGEAR"))) return MODEL_AC1450;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H264T00_NETGEAR"))) return MODEL_R6200v2; /* single core */
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H245T00_NETGEAR"))) return MODEL_R6250;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H240T00_NETGEAR"))) return MODEL_R6300v2;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H240T70_NETGEAR"))) return MODEL_R6300v2;
		if ((nvram_match("boardrev", "0x1601")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H332T00_NETGEAR"))) return MODEL_R6400;
		if ((nvram_match("boardrev", "0x1601")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H332T20_NETGEAR"))) return MODEL_R6400v2;
		if ((nvram_match("boardrev", "0x1601")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H332T30_NETGEAR"))) return MODEL_R6400v2;
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H270T10_NETGEAR"))) return MODEL_R6700v1;
		if ((nvram_match("boardrev", "0x1601")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H332T77_NETGEAR"))) return MODEL_R6700v3;
		if ((nvram_match("boardrev", "0x1601")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H332T78_NETGEAR"))) return MODEL_XR300;
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H270T11_NETGEAR"))) return MODEL_R6900;
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("boardnum", "32")) && (nvram_match("board_id", "U12H270T00_NETGEAR"))) return MODEL_R7000;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "24"))) return MODEL_DIR868L;
		if ((nvram_match("boardrev", "0x1101")) && (nvram_match("boardnum", "24"))) return MODEL_DIR868L;  /* rev c --> almost the same like rev a/b but different boardrev */
		if ((nvram_match("boardrev", "0x1101")) && (nvram_match("boardnum", "1234"))) return MODEL_WS880;
		if ((nvram_match("boardtype","0xE646")) && (nvram_match("boardnum", "20140309"))) return MODEL_EA6350v1;
		if ((nvram_match("boardtype","0xE646")) && (nvram_match("boardnum", "20130125"))) return MODEL_EA6350v1; /* EA6200 --> same like EA6350v1, AC1200 class router */
		if (nvram_match("t_fix1", "EA6350v2") ||
		    ((nvram_match("boardtype","0xE646")) && (nvram_match("boardnum", "20150309")))) return MODEL_EA6350v2; /* EA6350v2 */
		if ((nvram_match("boardtype","0x0646")) && (nvram_match("boardnum", "01"))) return MODEL_EA6400;
		if ((nvram_match("boardtype","0xF646")) && (nvram_match("boardnum", "01"))) return MODEL_EA6700;
		if ((nvram_match("boardtype","0xF646")) && (nvram_match("boardnum", "00"))) return MODEL_WZR1750;
		if ((nvram_match("boardtype","0xD646")) && (nvram_match("boardrev", "0x1100"))) return MODEL_EA6900;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("1:boardnum", "AC155g"))) return MODEL_AC15; /* Tenda AC15 */
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("1:boardnum", "AC18_5G"))) return MODEL_AC18; /* Tenda AC18 */
		if ((nvram_match("boardtype","0x0646")) && (nvram_match("boardrev", "0x1100")) && (nvram_match("boardnum", "AC1200v2"))) return MODEL_F9K1113v2_20X0; /* version 2000 and 2010 */
		if ((nvram_match("boardtype","0x0646")) && (nvram_match("boardrev", "0x1100")) && (nvram_match("boardnum", "02"))) return MODEL_F9K1113v2;
	}
#endif /* CONFIG_BCMWL6A */

	return MODEL_UNKNOWN;
}

/* return the MODEL number
 * cache the result for safe multiple use
 */
int get_model(void)
{
	static int model = MODEL_UNKNOWN;  /* initialize with 0 / MODEL_UNKNOWN */

	if (model == MODEL_UNKNOWN) { /* model unknown OR detect router for the first time */
		model = get_model_once();
	}

	return model;
}

int supports(unsigned long attr)
{
	return (strtoul(nvram_safe_get("t_features"), NULL, 0) & attr) != 0;
}

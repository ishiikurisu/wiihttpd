/*

Wiihttpd -- an HTTP server for the Wii

Modified to Wiihttpd by tekencal
Original Source from ftpii v0.0.5

ftpii Source Code Copyright (C) 2008 Joseph Jordan <joe.ftpii@psychlaw.com.au>

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1.The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be
appreciated but is not required.

2.Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3.This notice may not be removed or altered from any source distribution.

*/
#include <errno.h>
#include <fat.h>
#include <math.h>
#include <network.h>
#include <ogc/lwp_watchdog.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "common.h"

#define NET_BUFFER_SIZE 1024
#define FREAD_BUFFER_SIZE 1024

const char *CRLF = "\r\n";
const u32 CRLF_LENGTH = 2;

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static volatile u8 reset = 0;
static lwp_t reset_thread;

static void reset_called() {
	reset = 1;
}

static void *run_reset_thread(void *arg) {
	while (!reset && !(WPAD_ButtonsHeld(0) & WPAD_BUTTON_A)) {
		sleep(1);
		WPAD_ScanPads();
	}
	printf("\nWiihttpd Webserver shutting down...\n");
	exit(0);
}

u8 initialise_reset_button() {
	s32 result = LWP_CreateThread(&reset_thread, run_reset_thread, NULL, NULL, 0, 80);
	if (result == 0) SYS_SetResetCallback(reset_called);
	return !result;
}

void die(char *msg) {
	perror(msg);
	sleep(5);
	fatUnmount(0);
	//exit(1);
}

void initialise_video() {
    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync(); 
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}

static bool can_open_root_fs() {
    DIR_ITER *root = diropen("/");
    if (root) dirclose(root);
    return (bool)root;
}

void initialise_fat() {
    if (!fatInit(8192, true)) die("Unable to initialise FAT subsystem, exiting");
    // try to open root filesystem - if we don't check here, mkdir crashes later
    if (!can_open_root_fs()) die("Unable to open root filesystem, exiting");
    //if (!fatEnableReadAhead(PI_DEFAULT, 64, 128)) printf("Unable to enable FAT read-ahead caching, speed will suffer...\n");
}

static s32 initialise_network() {
    s32 result;
    while ((result = net_init()) == -EAGAIN);
    return result;
}

void wait_for_network_initialisation() {
    printf("Waiting for network to initialise...\n");
    if (initialise_network() >= 0) {
        char myIP[16];
        if (if_config(myIP, NULL, NULL, true) < 0) die("Error reading IP address, exiting");
        printf("Network initialised.  Wii IP address: %s\n", myIP);
    } else {
        die("Unable to initialise network, exiting");
    }
}

s32 create_server(u16 port) {
    s32 server = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server < 0) die("Error creating socket, exiting");

    struct sockaddr_in bindAddress;
    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_port = htons(port);
    bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (net_bind(server, (struct sockaddr *)&bindAddress, sizeof(bindAddress)) < 0) {
        net_close(server);
        die("Error binding socket");
    }
    if (net_listen(server, 3) < 0) {
        net_close(server);
        die("Error listening on socket");
    }

    return server;
}

s32 accept_peer(s32 server, struct sockaddr_in *addr) {
    s32 peer;
    socklen_t addrlen = sizeof(*addr);
    while ((peer = net_accept(server, (struct sockaddr *)addr, &addrlen)) < 0) {
        //net_close(server);
        die("Error accepting connection");
		
    }
    printf("\nAccepted connection from %s!\n", inet_ntoa(addr->sin_addr));
    return peer;
}

typedef s32 (*transferrer_type)(s32 s, void *mem, s32 len);
inline static s32 transfer_exact(s32 s, char *buf, s32 length, transferrer_type transferrer) {
    s32 bytes_transferred = 0;
    s32 remaining = length;
    while (remaining) {
        if ((bytes_transferred = transferrer(s, buf, remaining > NET_BUFFER_SIZE ? NET_BUFFER_SIZE : remaining)) > 0) {
            remaining -= bytes_transferred;
            buf += bytes_transferred;
        } else if (bytes_transferred < 0) {
            return bytes_transferred;
        } else {
            return -ENODATA;
        }
    }
    return 0;
}

inline s32 write_exact(s32 s, char *buf, s32 length) {
    return transfer_exact(s, buf, length, (transferrer_type)net_write);
}

// Read from a file
s32 write_from_file(s32 s, FILE *f) {
    char buf[FREAD_BUFFER_SIZE];
    s32 bytes_read, bytes_written;
    while (1) {
        bytes_read = fread(buf, 1, FREAD_BUFFER_SIZE, f);
        if (bytes_read > 0) {
            if ((bytes_written = write_exact(s, buf, bytes_read)) < 0) {
                printf("DEBUG: write_from_file() net_write error: [%i] %s\n", -bytes_written, strerror(-bytes_written));
                return bytes_written;
            }
        }
        if (bytes_read < FREAD_BUFFER_SIZE) {
            s32 result = -!feof(f);
            if (result < 0) {
                printf("DEBUG: write_from_file() fread error: [%i] %s\n", ferror(f), strerror(ferror(f)));
            }
            return result;
        }
    }
}


// Result must be able to hold up to maxsplit+1 null-terminated strings of length strlen(s)
// Returns the number of strings stored in the result array (up to maxsplit+1)
u32 split(char *s, char sep, u32 maxsplit, char *result[]) {
    u32 num_results = 0;
    u32 result_pos = 0;
    u32 trim_pos = 0;
    bool in_word = false;
    for (; *s; s++) {
        if (*s == sep) {
            if (num_results <= maxsplit) {
                in_word = false;
                continue;
            } else if (!trim_pos) {
                trim_pos = result_pos;
            }
        } else if (trim_pos) {
            trim_pos = 0;
        }
        if (!in_word) {
            in_word = true;
            if (num_results <= maxsplit) {
                num_results++;
                result_pos = 0;
            }
        }
        result[num_results - 1][result_pos++] = *s;
        result[num_results - 1][result_pos] = '\0';
    }
    if (trim_pos) {
        result[num_results - 1][trim_pos] = '\0';
    }
    u32 i = num_results;
    for (i = num_results; i <= maxsplit; i++) {
        result[i][0] = '\0';
    }
    return num_results;
}

/*

Wiihttpd -- an HTTP server for the Wii

Version: 0.0.2
Created: 19/06/2008
Last Modified: 24/06/2008

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
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include <stdio.h>
#include <string.h>
#include <sys/dir.h>
#include <time.h>
#include <wiiuse/wpad.h>
#include <unistd.h>

#include "common.h"

#define BUFFER_SIZE 512

static const s32 EQUIT = 696969;
static const u16 PORT = 80;
static const u8 MAX_CLIENTS = 15;

char server_name[50] = "Wiihttpd Webserver"; 
//char admin_password[50] = "test";
char server_dir[15] = "/www";
int server_timezone = 5; // Set your timezone in relation to GMT time (in this case GMT + 5)
int server_start_time = 0;
int server_pages_served = 0;
int server_pages_not_found = 0;

struct http_request_struct {
	char* method;
	char* path;
	char* version;
};

struct http_header_struct {
	char* statuscode;
	char* mimetype;
	int length;
};

struct client_struct {
	s32 socket;
	struct sockaddr_in address;
	struct http_request_struct http_request;
	struct http_header_struct http_header;
};


typedef struct client_struct client_t;

static volatile u8 num_clients = 0;

static s32 write_http_reply(client_t *client, char *msg) {
	u32 msglen = strlen(msg);
	char msgbuf[msglen + 1];
	if (msgbuf == NULL) return -ENOMEM;
	sprintf(msgbuf, "%s", msg);
	printf("Wrote reply: %s", msgbuf);
	return write_exact(client->socket, msgbuf, msglen);
}

static mutex_t chdir_mutex;

typedef s32 (*http_command_handler)(client_t *client);


// Returns the mime type that matches the extension
char* find_mime_type (char* extension) {
	char *mime_type = "text/html"; // Default
	
	int i;
	for (i = 0; mime_types[i].extension != NULL; i++)  {
		if ((strcmp(extension, mime_types[i].extension)) == 0) {
			mime_type = mime_types[i].mime_type;
		}
	}

	return mime_type;
}


// Make and send the HTTP headers to the client
void send_headers(client_t *client) {
	
	// Temp vars
	char* http_version;
	char* http_statustext;
	char* http_status = "";
	char* http_date = "";
	char* http_server = "";
	char* http_mimetype = "";
	char* http_length = "";
	
	// HTTP Version/Status
	if ((strcmp(client->http_request.version,"HTTP/1.1")) == 0) {
		http_version = "1.1";
	}
	else {
		http_version = "1.0";
	}
	if ((strcmp(client->http_header.statuscode,"200")) == 0) {
		http_statustext = "200 OK";
	}
	else {
		http_statustext = "404 Not Found";
	}
	
	// Date/time
	long the_time = time(0) - (server_timezone * 3600);
	
	char* date_time[5];
	char* tok;
	int x = 0;
	
	tok = strtok (ctime (&the_time)," ");
	while (tok != NULL) {
		date_time[x] = tok;
		tok = strtok (NULL, " ");
		x++;
	}
	
	char year[6] = "";
	strncpy (year,date_time[4], 4);
	year[5]='\0';
	
	
	sprintf(http_status, "HTTP/%s %s\r\n", http_version, http_statustext);
	write_http_reply(client, http_status);
	sprintf(http_date, "Date: %s, %s %s %s %s GMT\r\n", date_time[0], date_time[1], date_time[2], year, date_time[3]);
	write_http_reply(client, http_date);
	
	if ((strcmp(client->http_request.version,"HTTP/1.1")) == 0) {
		sprintf(http_server, "Server: %s\r\n", server_name);
		write_http_reply(client, http_server);
		write_http_reply(client, "Connection: close\r\n");
	}
	
	sprintf(http_mimetype, "Content-Type: %s\r\n", client->http_header.mimetype);
	write_http_reply(client, http_mimetype);
	sprintf(http_length, "Content-Length: %i\r\n", client->http_header.length);
	write_http_reply(client, http_length);
	write_http_reply(client, "\r\n");
}

void send_page (client_t *client, char* statuscode, char* mimetype, char* message) {

	int str_length = strlen(message);
	
	// HTTP codes
	client->http_header.statuscode = statuscode;
	client->http_header.mimetype = mimetype;
	client->http_header.length = str_length;
	
	// Send HTTP headers and the message
	send_headers(client);
	write_http_reply(client, message);
	
	usleep(50000);
}

// Server stats
void server_stats(client_t *client) {
	client->http_header.statuscode = "200";
	client->http_header.mimetype = "text/html";
	
	int server_uptime = time(0) - server_start_time;
	char* server_uptime_str = "";
	
	int uptime_days = server_uptime / 86400;
	int uptime_hours = server_uptime / 3600;
	int uptime_minutes = server_uptime / 60;
	int uptime_seconds = 0;
	
	char* day = "day";
	char* hour = "hour";
	char* minute = "minute";
	char* second = "second";
	
	// Ugly but it works...
	if (uptime_days >= 1) {
		uptime_hours = ((server_uptime - (uptime_days * 86400)) / 3600);
		
		if (uptime_hours >= 1) {
			uptime_minutes = ((server_uptime - ((uptime_days * 86400) + (uptime_hours * 3600))) / 60);
			
			if (uptime_minutes >= 1) {
				uptime_seconds = (server_uptime - ((uptime_days * 86400) + (uptime_hours * 3600) + (uptime_minutes * 60)));
			}
			else if (uptime_minutes <= 0) {
				uptime_minutes = 0;
				uptime_seconds = server_uptime - ((uptime_days * 86400) + (uptime_hours * 3600));
			}
		}
		else if (uptime_hours <= 0) {
			uptime_hours= 0;
			
			uptime_minutes = ((server_uptime - (uptime_days * 86400)) / 60);
			
			if (uptime_minutes >= 1) {
				uptime_seconds = (server_uptime - ((uptime_days * 86400) + (uptime_minutes * 60)));
			}
			else if (uptime_minutes <= 0) {
				uptime_minutes = 0;
				uptime_seconds = server_uptime - (uptime_days * 86400);
			}
		}
	}
	else if (uptime_hours >= 1) {
		uptime_minutes = ((server_uptime - (uptime_hours * 3600)) / 60);
		
		if (uptime_minutes >= 1) {
			uptime_seconds = (server_uptime - ((uptime_hours * 3600) + (uptime_minutes * 60)));
		}
		else if (uptime_minutes <= 0) {
			uptime_minutes = 0;
			uptime_seconds = server_uptime - (uptime_hours * 3600);
		}
	}
	else if (uptime_minutes >= 1) {
		uptime_seconds = (server_uptime - (uptime_minutes * 60));
	}
	else {
		uptime_seconds = server_uptime;
	}
	
	if (uptime_days == 0 || uptime_days >= 2) {
		day = "days";
	}
	if (uptime_hours == 0 || uptime_hours >= 2) {
		hour = "hours";
	}
	if (uptime_minutes == 0 || uptime_minutes >= 2) {
		minute = "minutes";
	}
	if (uptime_seconds == 0 || uptime_seconds >= 2) {
		second = "seconds";
	}
	sprintf(server_uptime_str, "%i %s %i %s %i %s %i %s", uptime_days, day, uptime_hours, hour, uptime_minutes, minute, uptime_seconds, second);

	
	char output_html[250];
	int str_length;
	str_length = sprintf(output_html, "<html><head><title>%s</title></head><body><b>Server Uptime:</b> %s<br/> <b>Pages/Files Served:</b> %i<br/> <b>Pages/Files Not Found:</b> %i<br/></body></html>\r\n", server_name, server_uptime_str, server_pages_served, server_pages_not_found);
	
	// HTTP codes
	client->http_header.statuscode = "200 OK";
	client->http_header.mimetype = "text/html";
	client->http_header.length = str_length;
	
	// Send HTTP headers and file
	send_headers(client);
	write_http_reply(client, output_html);
	
	usleep(50000);
	
	printf("Sent data - Server Status\n");
}


// If client requests a file using the GET method
static s32 http_GET(client_t *client) {
	
	// Index page
	if ((strcmp(client->http_request.path,"/")) == 0) {
		client->http_request.path = "/index.html";
	}
	else if ((strcmp(client->http_request.path,"/stats")) == 0) {
		server_stats(client);
		return -EQUIT;
	}
	
	// Check for ./ and ../
	if (strstr(client->http_request.path, "./") || strstr(client->http_request.path, "../")) {
		char output_html[150];
		sprintf(output_html, "<html><head><title>%s</title></head><body>404 File Not Found</body></html>\r\n", server_name);
		send_page (client, "404", "text/html", output_html);
		server_pages_not_found++;
		printf("Sent data - 404 File Not Found\n");
		return -EQUIT;
	}
	
	char* path = "";
	strcpy(path, server_dir);
	strcat(path, client->http_request.path);
	
	// Find %20 and replace with a space
	char* space_tok= "";
	char* space_replace = "";
	
	space_tok = strtok (path,"%20");
	space_replace = space_tok;
	while (space_tok != NULL) {
		space_tok = strtok (NULL, "%20"); 
		if (space_tok != NULL) {
			strcat(space_replace, " ");
			strcat(space_replace, space_tok);
		}
	}
	path = space_replace;
	
	// Open the file
	FILE *f = fopen(path, "rb");
	
	// 404 File Not Found 
	if (!f) {
		
		char output_html[150];
		sprintf(output_html, "<html><head><title>%s</title></head><body>404 File Not Found</body></html>\r\n", server_name);
		
		send_page (client, "404", "text/html", output_html);
		
		server_pages_not_found++;
		printf("Sent data - 404 File Not Found\n");
		
		return -EQUIT;
	}
	
	// 500 Internal Server Error
	if (fseek(f, 0, SEEK_SET)) {
		//s32 fseek_error = errno;
		fclose(f);
		
		char output_html[150];
		sprintf(output_html, "<html><head><title>%s</title></head><body>500 Internal Server Error</body></html>\r\n", server_name);
		
		send_page (client, "500", "text/html", output_html);
		
		printf("Sent data - 500 Internal Server Error or user may have cancelled loading a file/page.\n");
		
		return -EQUIT;
	}
	
	// 200 OK
	// HTTP codes
	client->http_header.statuscode = "200";
	
	// Mimetype
	char* file_extension;
	char* file_extension_last = "";
	
	file_extension = strtok (path,".");
	while (file_extension != NULL) {
		file_extension_last = file_extension;
		file_extension = strtok (NULL, "."); 
	}
	
	if (file_extension_last != NULL) { 
		file_extension = file_extension_last;
	}
	
	client->http_header.mimetype = find_mime_type(file_extension);
	
	// File size
	fseek (f , 0, SEEK_END);
	client->http_header.length = ftell (f);
	rewind (f);
	
	// Send HTTP headers and file
	send_headers(client); 
	s32 result;
	
	// If requested method was HEAD then don't send any content
	if ((strcmp(client->http_request.method,"HEAD")) == 0) {
		send_headers(client);  
		result = 0;
	}
	else {
		result = write_from_file(client->socket, f);
	}
	
	fclose(f);
	
	usleep(50000);
	server_pages_served++;
	
	// We don't really have to care about transfers not being completed
	if (result < 0) {
		printf("Sent data - Internal Server Error.\n");
	} else {
		printf("Sent data.\n");
	}
	
	return -EQUIT;
}

// If we can't understand the request, ignore it. This includes anything that the client says
// after the first line: GET x HTTP/1.x
static s32 http_UNKNOWN(client_t *client) {
	return 1;
}

// Process the requests given by the client
// Returns a negative to signal an error that requires closing the connection
static s32 process_command(client_t *client, char *cmd_line) {
	//printf("Got command: %s\n", cmd_line);
	char cmd[BUFFER_SIZE], rest[BUFFER_SIZE];
	char *args[] = { cmd, rest };
	u32 num_args = split(cmd_line, ' ', 1, args); 
	if (num_args == 0) {
		return 0;
	}
	
	http_command_handler handler = http_UNKNOWN;
	
	// Split up the HTTP request
	client->http_request.method = strtok(cmd_line, " ");
	client->http_request.path = strtok(NULL, " ");
	client->http_request.version = strtok(NULL, " ");
	printf("Users = %i\n", num_clients); 
	printf("HTTP Request: %s %s %s\n", client->http_request.method, client->http_request.path, client->http_request.version);
	
	// Valid HTTP request?
	if (client->http_request.method != NULL && client->http_request.path != NULL && client->http_request.version != NULL) {
		if ( ((strcmp(client->http_request.method,"GET")) == 0) || ((strcmp(client->http_request.method,"HEAD")) == 0) ) {
			handler = http_GET;
		}
	}

	return handler(client);
}

static void *process_connection(void *client_ptr) {
	client_t *client = client_ptr;

	char buf[BUFFER_SIZE];
	s32 offset = 0;
	s32 bytes_read;
	while (offset < (BUFFER_SIZE - 1)) {
		char *offset_buf = buf + offset;
		if ((bytes_read = net_read(client->socket, offset_buf, BUFFER_SIZE - 1 - offset)) < 0) {
			printf("Read error %i occurred, closing client.\n", bytes_read);
			goto recv_loop_end;
		} else if (bytes_read == 0) {
			goto recv_loop_end; // EOF from client
		}
		offset += bytes_read;
		buf[offset] = '\0';
		
		if (strchr(offset_buf, '\0') != (buf + offset)) {
			printf("Received a null byte from client, closing connection ;-)\n"); // i have decided this isn't allowed =P
			goto recv_loop_end;
		}
		
		char *next;
		char *end;
		for (next = buf; (end = strstr(next, CRLF)); next = end + CRLF_LENGTH) {
			*end = '\0';
			if (strchr(next, '\n')) {
				printf("Received a line-feed from client without preceding carriage return, closing connection ;-)\n"); // i have decided this isn't allowed =P
				goto recv_loop_end;
			}
			
			if (*next) {
				s32 result;
				if ((result = process_command(client, next)) < 0) {
					if (result != -EQUIT) {
						printf("Closing connection due to error while processing command: %s\n", next);
					}
					goto recv_loop_end;
				}
			}
		}
		
		if (next != buf) { // some lines were processed
			offset = strlen(next);
			char tmp_buf[offset];
			memcpy(tmp_buf, next, offset);
			memcpy(buf, tmp_buf, offset);
		}
	}
	
	printf("Received line longer than %u bytes, closing client.\n", BUFFER_SIZE - 1);
	
	recv_loop_end:
	
	usleep(5000);
	
	net_close(client->socket);
	free(client);
	num_clients--;
	
	printf("Connection finished.\n");
	return NULL;
}

static void mainloop() {
	s32 server = create_server(PORT);
	printf("\nListening on TCP port %u...\n", PORT);
	server_start_time = time(0);
	
	while (1) {
		struct sockaddr_in client_address;
		s32 peer = accept_peer(server, &client_address);
		
		if (num_clients == MAX_CLIENTS) {
			printf("Maximum of %u clients reached, not accepting client.\n", MAX_CLIENTS);
			net_close(peer);
			continue;
		}
		
		// Very simple flood protection, server may pause for a little once in a while when 
		// receiving multiple connections at nearly the same time but it's better than having a code dump
		if (num_clients >= 1) {
			usleep((20000 * num_clients));
		}
		
		num_clients++;
		
		client_t *client = malloc(sizeof(client_t));
		if (!client) {
			printf("Could not allocate memory for client state, not accepting client.\n");
			net_close(peer);
			continue;
		}
		client->socket = peer;
		memcpy(&client->address, &client_address, sizeof(client_address));
		
		lwp_t client_thread;
		LWP_ThreadBroadcast(client_thread); // Set blocked threads to running
		if (LWP_CreateThread(&client_thread, process_connection, client, NULL, 0, 80)) {
			printf("Error creating client thread, not accepting client.\n");
			net_close(peer);
			free(client);
		}
	}
}

int main(int argc, char **argv) {
	initialise_video();
	printf("\x1b[2;0H");
	printf("Wiihttpd Webserver v0.0.1\n");
	initialise_fat();
	if (chdir("/")) die("Could not change to root directory, exiting.\n");
	WPAD_Init();
	if (initialise_reset_button()) {
		printf("To exit, press A or press the reset button.\n");
	} else {
		printf("Unable to start reset thread - hold down the power button to exit.\n");
	}
	wait_for_network_initialisation();
	if (LWP_MutexInit(&chdir_mutex, true)) die("Could not initialise chdir mutex, exiting");
	
	mainloop();
	
	return 0;
}

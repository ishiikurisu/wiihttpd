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
#ifndef _COMMON_H_
#define _COMMON_H_

#include <ogcsys.h>

const char *CRLF;
const u32 CRLF_LENGTH;

static const struct {
	char* extension;
	char* mime_type;
}

mime_types[] =  {
	{"arj",		"application/x-arj-compressed"	},
	{"asf",		"video/x-ms-asf"				},
	{"avi",		"video/x-msvideo"				},
	{"bin",		"application/octet-stream"		},
	{"bmp",		"image/bmp"						},
	{"c",		"text/plain"					},
	{"css",		"text/css"						},
	{"doc",		"application/msword"			},
	{"exe",		"application/octet-stream"		},
	{"gif",		"image/gif"						},
	{"gtar",	"application/x-gtar"			},
	{"gz",		"application/x-gunzip"			},
	{"h",		"text/plain"					},
	{"htm",		"text/html"						},
	{"html", 	"text/html"						},
	{"ico",		"image/x-icon"					},
	{"java",	"text/plain"					},
	{"jpe",		"image/jpeg"					},
	{"jpeg",	"mage/jpeg"						},
	{"jpg",		"mage/jpeg"						},
	{"js",		"application/x-javascript"		},
	{"m3u",		"audio/x-mpegurl"				},
	{"mid",		"audio/mid"						},	
	{"midi",	"audio/midi"					},
	{"mod",		"audio/mod"						},
	{"mp2",		"video/mpeg"					},
	{"mp3",		"audio/x-mp3"					},
	{"mpa",		"video/mpeg"					},
	{"mpe",		"video/mpeg"					},
	{"mpeg",	"video/mpeg"					},
	{"mpg",		"video/mpeg"					},
	{"mpp",		"application/vnd.ms-project"	},
	{"mpv2",	"video/mpeg"					},
	{"mp3",		"audio/x-mp3"					},
	{"mov",		"video/quicktime"				},
	{"pdf",		"application/pdf"				},
	{"png",		"image/png"						},
	{"pps",		"application/vnd.ms-powerpoint"	},
	{"ppt",		"application/vnd.ms-powerpoint"	},
	{"qt",		"video/quicktime"				},
	{"ra",		"audio/x-pn-realaudio"			},
	{"ram",		"audio/x-pn-realaudio"			},
	{"rar",		"application/x-arj-compressed"	},
	{"rtf",		"application/rtf"				},
	{"rtx",		"text/richtext"					},
	{"svg",		"image/svg+xml"					},
	{"swf",		"application/x-shockwave-flash"	},
	{"tar",		"application/x-tar"				},
	{"tif",		"image/tiff"					},
	{"tiff",	"image/tiff"					},
	{"txt",		"text/plain"					},
	{"tgz",		"application/x-tar-gz"			},
	{"torrent",	"application/x-bittorrent"		},
	{"txt",		"text/plain"					},
	{"wav",		"audio/x-wav"					},
	{"xls",		"application/excel"				},
	{"xla",		"application/vnd.ms-excel"		},
	{"xlc",		"application/vnd.ms-excel"		},
	{"xlm",		"application/vnd.ms-excel"		},
	{"xls",		"application/vnd.ms-excel"		},
	{"xlt",		"application/vnd.ms-excel"		},
	{"xlw",		"application/vnd.ms-excel"		},
	{"zip",		"application/x-zip-compressed"	},
	{NULL, 		NULL}
};

u8 initialise_reset_button();

void die(char *msg);

void initialise_video();

void initialise_fat();

void wait_for_network_initialisation();

s32 create_server(u16 port);

s32 accept_peer(s32 server, struct sockaddr_in *addr);

s32 write_exact(s32 s, char *buf, s32 length);

s32 write_from_file(s32 s, FILE *f);

u32 split(char *s, char sep, u32 maxsplit, char *result[]);

#endif /* _COMMON_H_ */

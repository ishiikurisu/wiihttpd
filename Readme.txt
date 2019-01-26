Wiihttpd for Nintendo Wii
---------------------------------
A simple multi-threaded HTTP Webserver for the Wii.
Wiihttpd is built on the ftpii source code.


Features:
------------------
- Basic HTTP/1.0 and HTTP/1.1 support
- Dynamic HTTP headers
- Able to serve any file type you like (you can add more file types in common.h)
- Able to use spaces and dots in file names
- View server uptime, pages/files served and not found by going to /stats


Author:
------------------
teknecal (teknecal@gmail.com) - www.wiiregionworld.com


Usage:
------------------
Create a /www directory on the root of your SD card and place any files you would 
like to be served in there.

Run the included ELF with your favourite method to boot homebrew.


History:
------------------
24 June 2008 (v0.0.2)
- Removed configurable GMT text setting in HTTP header and instead allowed timezone change
- Added Connection: close to HTTP/1.1 header
- Added more mime types
- Added HTTP HEAD method
- Protection against ./ and ../ in HTTP path by displaying a 404 page

21 June 2008 (v0.0.1)
- Initial Release


Credits:
------------------
Joe Jordan <joe.ftpii@psychlaw.com.au> - Used ftpii source to code from
Cboomf + Felix123 - used your Wii Webserver source for mime types list/struct/for loop


Notes:
------------------
Source code is included :)
Visit the Wii homebrew developers in #wiidev @ EFnet
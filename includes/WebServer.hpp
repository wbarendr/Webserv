#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

# define CRLF "\r\n"
# define CRLF_LEN 2
# define TYPE_UNDEFINED -1
# define HEX_STR "0123456789ABCDEF"
# define RECV_BUF_SIZE 500000

# define SERVER_VERSION "webserv/1.0"

enum	e_status {
	INVALID 	= -1,
	SUCCESS 	= 0,
	IN 			= 0,
	ERROR 		= 1,
	CONTINUE 	= 1,
	OUT 		= 1
};

enum e_methods
{
	GET,
	HEAD,
	POST,
	PUT,
	DELETE
};

enum e_headers
{
	ACCEPTCHARSET,
	ACCEPTLANGUAGE,
	ALLOW,
	AUTHORIZATION,
	CONTENTLANGUAGE,
	CONTENTLENGTH,
	CONTENTLOCATION,
	CONTENTTYPE,
	DATE,
	HOST,
	LASTMODIFIED,
	LOCATION,
	REFERER,
	RETRYAFTER,
	SERVER,
	TRANSFERENCODING,
	USERAGENT,
	WWWAUTHENTICATE
};

enum e_protocol
{
	HTTP1 = 1
};

#endif

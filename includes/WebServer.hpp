#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

# define CRLF "\r\n"

enum	e_status {
	INVALID = -1,
	SUCCESS = 0,
	IN = 0,
	ERROR = 1,
	OUT = 1
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
	ACCEPTCHARSETS,
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

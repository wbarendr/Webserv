#include "Client.hpp"
#include "Server.hpp"
#include "Error.hpp"
#include "WebServer.hpp"
#include "Logger.hpp"
#include <unistd.h>

Request::Request()
	: m_owner(0),
	m_method(-1),
	m_uri(""),
	m_protocol(-1),
	m_content_length(0),
	m_headers(18, ""),
	x_headers(),
	m_if_body(false),
	m_body(""),
	m_tmp_body(""),
	m_metadata_parsed(false),
	m_done(false),
	m_chunked(false),
	m_cgi(0),
	m_autoindex(0),
	m_status_code(0),
	m_start(0),
	m_location(),
	m_query_string(""),
	m_path_info(""),
	m_real_path(""),
	m_file(""),
	m_remote_user(""),
	m_file_type(TYPE_UNDEFINED),
	m_looking_for_size(true),
	m_last_chunk(false)
{
}

Request::Request(Request const & src) 
	:m_method(src.m_method),
	 m_uri(src.m_uri),
	 m_protocol(src.m_protocol),
	 m_content_length(src.m_content_length),
	 m_headers(src.m_headers.begin(), src.m_headers.end()),
	 x_headers(src.x_headers.begin(), src.x_headers.end()),
	 m_if_body(src.m_if_body),
	 m_body(src.m_body),
	 m_tmp_body(src.m_tmp_body),
	 m_metadata_parsed(src.m_metadata_parsed),
	 m_done(src.m_done),
	 m_chunked(src.m_chunked),
	 m_cgi(src.m_cgi),
	 m_autoindex(src.m_autoindex),
	 m_status_code(src.m_status_code),
	 m_start(src.m_start),
	 m_location(src.m_location),
	 m_query_string(src.m_query_string),
	 m_path_info(src.m_path_info),
	 m_real_path(src.m_real_path),
	 m_file(src.m_file),
	 m_remote_user(src.m_remote_user),
	 m_file_type(src.m_file_type),
	 m_looking_for_size(src.m_looking_for_size),
	 m_last_chunk(src.m_last_chunk)
{
}

Request &Request::operator=(Request const & rhs) {
	 this->m_method         = rhs.m_method;
	 this->m_uri            = rhs.m_uri;
	 this->m_protocol       = rhs.m_protocol;
	 this->m_content_length = rhs.m_content_length;
	 this->m_headers.assign(rhs.m_headers.begin(), rhs.m_headers.end());
	 this->x_headers 		= rhs.x_headers;
	 this->m_if_body        = rhs.m_if_body;
	 this->m_body           = rhs.m_body;
	 this->m_tmp_body       = rhs.m_tmp_body;
	 this->m_metadata_parsed= rhs.m_metadata_parsed;
	 this->m_done           = rhs.m_done;
	 this->m_chunked        = rhs.m_chunked;
	 this->m_cgi            = rhs.m_cgi;
	 this->m_autoindex      = rhs.m_autoindex;
	 this->m_status_code    = rhs.m_status_code;
	 this->m_start          = rhs.m_start;
	 this->m_location       	= rhs.m_location;
	 this->m_query_string   	= rhs.m_query_string;
	 this->m_path_info      	= rhs.m_path_info;
	 this->m_real_path      	= rhs.m_real_path;
	 this->m_file           	= rhs.m_file;
	 this->m_remote_user		= rhs.m_remote_user;
	 this->m_file_type          = rhs.m_file_type;
	 this->m_looking_for_size 	= rhs.m_looking_for_size;
	 this->m_last_chunk 		= rhs.m_last_chunk;
	 return *this;
}

void	Request::reset() {
	this->m_method = -1;
	this->m_uri.clear();
	this->m_protocol = -1;
	this->m_content_length = 0;
	this->m_headers.assign(18, "");
	this->m_headers.clear();
	this->m_if_body = false;
	this->m_body.clear();
	this->m_tmp_body.clear();
	this->m_metadata_parsed = false;
	this->m_done = false;
	this->m_chunked = false;
	this->m_cgi = 0;
	this->m_autoindex = 0;
	this->m_status_code = 0;
	this->m_start = 0;
	this->m_location  = s_v_server_conf::t_routes::iterator();
	this->m_query_string.clear();
	this->m_path_info.clear();
	this->m_real_path.clear();
	this->m_file.clear();
	this->m_file_type = TYPE_UNDEFINED;
	this->m_looking_for_size = true;
	this->m_last_chunk = false;
}

Response::Response()
	:m_cgi_metadata_parsed(false),
	 m_cgi_metadata_sent(false),
	 m_content_type(""),
	 m_body(""),
	 m_response_headers(0),
	 m_content_language(""),
	 m_content_location("")
{
}

void	Response::reset() {
	this->m_cgi_metadata_parsed = false;
	this->m_cgi_metadata_sent = false;
	this->m_content_type = "";
	this->m_body = "";
	this->m_response_headers.clear();
	this->m_content_language = "";
	this->m_content_location = "";
}

Client::Client() 
	: m_request_str(""),
	m_response_str(""),
	m_request_data(),
	m_response_data(),
	m_v_server(0),
	m_socket(-1),
	m_sockaddr(),
	m_addrlen(sizeof(m_sockaddr)),
	m_cgi_pid(-1),
	m_cgi_running(0),
	m_cgi_post(false),
	m_cgi_end_chunk(0),
	m_cgi_write_offset(0),
	m_cgi_out_buf(),
	m_range_fd(0)
{
	this->m_cgi_read_pipe[IN] = -1;
	this->m_cgi_read_pipe[OUT] = -1;
	this->m_cgi_write_pipe[IN] = -1;
	this->m_cgi_write_pipe[OUT] = -1;
	this->m_request_data.m_owner = this;
}


Client::Client(Client const & src)
	: m_request_str(src.m_request_str),
	m_response_str(src.m_response_str),
	m_request_data(src.m_request_data),
	m_response_data(src.m_response_data),
	m_v_server(src.m_v_server),
	m_v_server_blocks(src.m_v_server_blocks),
	m_socket(src.m_socket),
	m_sockaddr(src.m_sockaddr),
	m_addrlen(src.m_addrlen),
	m_cgi_pid(src.m_cgi_pid),
	m_cgi_running(src.m_cgi_running),
	m_cgi_post(src.m_cgi_post),
	m_cgi_end_chunk(src.m_cgi_end_chunk),
	m_cgi_write_offset(src.m_cgi_write_offset),
	m_cgi_out_buf(src.m_cgi_out_buf),
	m_range_fd(src.m_range_fd)
{
	this->m_cgi_read_pipe[IN] = src.m_cgi_read_pipe[IN];
	this->m_cgi_read_pipe[OUT] = src.m_cgi_read_pipe[OUT];
	this->m_cgi_write_pipe[IN] = src.m_cgi_write_pipe[IN];
	this->m_cgi_write_pipe[OUT] = src.m_cgi_write_pipe[OUT];
	this->m_request_data.m_owner = this;
}

Client &Client::operator=(Client const & rhs) {
	this->m_request_str = rhs.m_request_str;
	this->m_response_str = rhs.m_response_str;
	this->m_request_data = rhs.m_request_data;
	this->m_v_server = rhs.m_v_server;
	this->m_v_server_blocks = rhs.m_v_server_blocks;
	this->m_socket = rhs.m_socket;
	this->m_sockaddr = rhs.m_sockaddr;
	this->m_addrlen = rhs.m_addrlen;
	this->m_cgi_pid = rhs.m_cgi_pid;
	this->m_cgi_running = rhs.m_cgi_running;
	this->m_cgi_post = rhs.m_cgi_post;
	this->m_cgi_end_chunk= rhs.m_cgi_end_chunk;
	this->m_request_data.m_owner = this;
	this->m_cgi_read_pipe[IN] = rhs.m_cgi_read_pipe[IN];
	this->m_cgi_read_pipe[OUT] = rhs.m_cgi_read_pipe[OUT];
	this->m_cgi_write_pipe[IN] = rhs.m_cgi_write_pipe[IN];
	this->m_cgi_write_pipe[OUT] = rhs.m_cgi_write_pipe[OUT];
	this->m_cgi_write_offset = rhs.m_cgi_write_offset;
	this->m_cgi_out_buf = rhs.m_cgi_out_buf;
	this->m_range_fd = rhs.m_range_fd;
	return *this;
}


void	Client::updateServerConf()
{
	std::string const & host = this->m_request_data.m_headers[HOST];
	size_t	len = 0;

	if (host.empty()) {
		throw HTTPError("updateServerConf", "empty host header." , 400);
	}
	for (size_t i = 0; i < (*(this->m_v_server_blocks)).size(); ++i) {
		t_v_server & v_server = (*(this->m_v_server_blocks))[i];
		t_directives	const & directives = v_server.m_configs.m_directives;
		t_directives::const_iterator	 const & it = directives.find("server_name");
		if (directives.find("server_name") != directives.end()) {
			len = host.find(':', 0);
			if (it->second.compare(0, it->second.size(), host, 0, len) == 0) {
				this->m_v_server = &v_server;
				return ;
			}
		}
	}
	this->m_v_server = &((*(this->m_v_server_blocks))[0]);//if not found, return the first added,default one
}

void	Client::reset() {
	this->m_request_str.clear();
	this->m_response_str.clear();
	this->m_request_data.reset();
	this->m_response_data.reset();
	//client always linked to a virtual context (blocks of virtual servers on the same ip:port), not to a particular block, so only reset v_server
	this->m_v_server = 0;
	//cgi
	this->m_cgi_pid = -1;
	this->m_cgi_running = false;
	this->m_cgi_post = false;
	this->m_cgi_write_offset = 0;
	this->m_cgi_end_chunk = false;
	this->m_cgi_out_buf.clear();
}

int	&Client::getWriteFd() {
	return  this->m_cgi_write_pipe[OUT];
}

int	&Client::getReadFd() {
	return  this->m_cgi_read_pipe[IN];
}

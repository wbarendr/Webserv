#include "Cgi.hpp"
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "Error.hpp"
#include "utils.hpp"
#include "WebServer.hpp"
#include "RequestParser.hpp"
#include "Logger.hpp"

Cgi::Cgi()
	: m_env_array(0), m_argv(0), m_env_array_size(0)
{
}

Cgi::~Cgi() {
	this->clear();
}

//only allocates memory when Cgi is used
void	Cgi::init(t_client const & c) {
	size_t	const size = CGI_ENV_DEFAULT_SIZE + c.m_request_data.x_headers.size();
	if (size < this->m_env_array_size)
		return ;
	this->m_env_array_size = size;
	if (this->m_env_array) {
		free(this->m_env_array);
	}
	this->m_env_array = reinterpret_cast<char **>(malloc(sizeof(char *) * size));
	if (!this->m_env_array)
		throw (serverError("Cgi::init", "failed to allocate cgi env"));
	if (this->m_argv)
		return ;
	this->m_argv = reinterpret_cast<char **>(malloc(sizeof(char *) * 3));
}

void	Cgi::convertEnv(t_client const & c) {
	t_cgi_env_map::iterator it = this->m_env_map.begin();
	t_request_data const & request = c.m_request_data;
	size_t	i = 0;
	while (it != this->m_env_map.end()) {
		std::string	convert(it->first + "=" + it->second);
		this->m_env_array[i] = ft::strdup(convert);
		if (!this->m_env_array[i])
			throw (serverError("Cgi::convertEnv", "failed to allocate cgi env"));
		Logger::Log()<<this->m_env_array[i]<<std::endl;//remove
		++it;
		++i;
	}
	this->m_env_array[i] = 0;
	this->m_argv[0] = ft::strdup(request.m_location->second["cgi_path"]);
	this->m_argv[1] = ft::strdup(request.m_stat_file);
	this->m_argv[2] = 0;
}

void	Cgi::read(t_client &c) {
	char buf[CGI_BUF_SIZE];

	ssize_t	nbytes = ::read(c.getReadFd(), buf, CGI_BUF_SIZE - 1);
	if (nbytes <= 0) {
		if (nbytes == -1)
			throw HTTPError("Cgi::read", strerror(errno), 500);
		c.m_cgi_end_chunk = true;
	}
	else
	{
		buf[nbytes] = '\0';
		c.m_cgi_out_buf.append(buf);
	}
}


void	Cgi::closeAll(t_client &c) {
	if (c.m_cgi_read_pipe[IN] != -1 && ::close(c.m_cgi_read_pipe[IN]) == -1) {
		Logger::Log() << "Cgi::closeAll: close(m_cgi_read_pipe[IN]): " << strerror(errno)<<std::endl;
	}
	if (c.m_cgi_read_pipe[OUT] != -1 && ::close(c.m_cgi_read_pipe[OUT]) == -1) {
		Logger::Log() << "Cgi::closeAll: close(m_cgi_read_pipe[OUT]): " << strerror(errno)<<std::endl;
	}
	if (c.m_cgi_write_pipe[IN] != -1 && ::close(c.m_cgi_write_pipe[IN]) == -1) {
		Logger::Log() << "Cgi::closeAll: close(m_cgi_write_pipe[IN]): " << strerror(errno)<<std::endl;
	}
	if (c.m_cgi_write_pipe[OUT] != -1 && ::close(c.m_cgi_write_pipe[OUT]) == -1) {
		Logger::Log() << "Cgi::closeAll: close(m_cgi_write_pipe[OUT]): " << strerror(errno)<<std::endl;
	}
	c.m_cgi_read_pipe[IN] =-1;
	c.m_cgi_read_pipe[OUT] =-1;
	c.m_cgi_write_pipe[IN] =-1;
	c.m_cgi_write_pipe[OUT] =-1;
	c.m_range_fd = 0;
}

void	Cgi::write(t_client &c) {
	size_t	len = c.m_request_data.m_body.size() + 1 - c.m_cgi_write_offset;
	ssize_t	nbytes = ::write(c.getWriteFd(), 
			c.m_request_data.m_body.c_str() + c.m_cgi_write_offset,
			len);

	if (nbytes == -1) {
		throw HTTPError("Cgi::write: ", strerror(errno), 500);
	}
	c.m_cgi_write_offset += nbytes;
	if (c.m_cgi_write_offset >= c.m_request_data.m_body.size()) {
		close(c.m_cgi_write_pipe[OUT]);
		c.m_cgi_write_pipe[OUT] = -1;
	}
}

void	Cgi::setParentIo(t_client &c) {
	if (pipe(c.m_cgi_write_pipe) == -1) {
		throw HTTPError("Cgi::run: pipe(cgi_write_pipe)", strerror(errno), 500);
	}
	if (pipe(c.m_cgi_read_pipe) == -1) {
		throw HTTPError("Cgi::run: pipe(cgi_read_pipe)", strerror(errno), 500);
	}
	if (fcntl(c.m_cgi_read_pipe[IN], F_SETFL, O_NONBLOCK) == -1)
		Logger::Log() << "Cgi::init : fcntl(m_cgi_read_pipe[IN]): " << strerror(errno)<<std::endl;
	if (fcntl(c.m_cgi_read_pipe[OUT], F_SETFL, O_NONBLOCK) == -1)
		Logger::Log() << "Cgi::init : fcntl(m_cgi_read_pipe[OUT]): " << strerror(errno)<<std::endl;
	if (fcntl(c.m_cgi_write_pipe[OUT], F_SETFL, O_NONBLOCK) == -1)
		Logger::Log() << "Cgi::init : fcntl(m_cgi_write_pipe[OUT]): " << strerror(errno)<<std::endl;
}

void	Cgi::setChildIo(t_client &c) {
	if (dup2(c.m_cgi_read_pipe[OUT], STDOUT_FILENO) == -1) {
		throw HTTPError("Cgi::exec: dup2(cgi_read_pipe[OUT], STDOUT_FILENO)", strerror(errno), 500);
	}
	if (close(c.m_cgi_read_pipe[IN]) == -1
		|| close(c.m_cgi_read_pipe[OUT]) == -1)
		throw HTTPError("Cgi::setChildIo: close(cgi_read_pipe[IN-OUT])", strerror(errno), 500);
	if (dup2(c.m_cgi_write_pipe[IN], STDIN_FILENO) == -1) {
			throw HTTPError("Cgi::exec: dup2(cgi_write_pipe[IN], STDIN_FILENO)", strerror(errno), 500);
		}
	if (close(c.m_cgi_write_pipe[IN]) == -1
		|| close(c.m_cgi_write_pipe[OUT]) == -1)
		throw HTTPError("Cgi::setChildIo: close(cgi_write_pipe[IN-OUT])", strerror(errno), 500);
}


void	Cgi::exec(t_client &c) {
	if ((c.m_cgi_pid = fork()) == -1) {
		this->clear();
		throw HTTPError("Cgi::exec: fork", strerror(errno), 500);
	}
	if (!c.m_cgi_pid) {
		this->setChildIo(c);
		if (execve(this->m_argv[0], this->m_argv, this->m_env_array) == -1) {
			this->clear();
			::exit(EXIT_FAILURE);
		}
	}
	this->reset();
	if (close(c.m_cgi_write_pipe[IN]) == -1)
		throw HTTPError("Cgi::exec: close(m_cgi_write_pipe[IN])", strerror(errno), 500);
	c.m_cgi_write_pipe[IN] = -1;
	if (close(c.m_cgi_read_pipe[OUT]) == -1)
		throw HTTPError("Cgi::exec: close(m_cgi_read_pipe[OUT])", strerror(errno), 500);
	c.m_cgi_read_pipe[OUT] = -1;
}

void	Cgi::run(t_client &c) {
	Logger::Log()<<"run cgi"<<std::endl;
	if (!c.m_cgi_running) {
		c.m_cgi_running = true;
		c.m_cgi_post = c.m_request_data.m_method == POST;
		this->init(c);
		this->fillEnv(c);
		this->convertEnv(c);
		this->setParentIo(c);
		this->exec(c);
	}
}

void	Cgi::fillEnv(t_client const & c) {
	t_request_data	const & request = c.m_request_data;
	std::string const &auth = request.m_headers[AUTHORIZATION];
	this->m_env_map["AUTH_TYPE"]=std::string(auth, 0, auth.find(' '));
	this->m_env_map["CONTENT_LENGTH"]=ft::intToString(request.m_body.size());
	this->m_env_map["CONTENT_TYPE"]=request.m_headers[CONTENTTYPE];
	this->m_env_map["GATEWAY_INTERFACE"]="CGI/1.1";
	this->m_env_map["PATH_INFO"]= request.m_path_info;
	this->m_env_map["PATH_TRANSLATED"]= request.m_stat_file.substr(0, request.m_stat_file.rfind('/')) + request.m_path_info;
	this->m_env_map["QUERY_STRING"] = request.m_query_string;
	struct	sockaddr_in	*tmp = reinterpret_cast<struct sockaddr_in*>(&request.m_owner->m_sockaddr);
	this->m_env_map["REMOTE_ADDR"] = ft::inet_ntoa(tmp->sin_addr);
	this->m_env_map["REMOTE_IDENT"] =""; //not supported
	this->m_env_map["REMOTE_USER"] =request.m_remote_user;
	this->m_env_map["REQUEST_METHOD"] = methods[request.m_method];
	this->m_env_map["REQUEST_URI"] = request.m_uri;
	this->m_env_map["SCRIPT_NAME"] = request.m_file;
	this->m_env_map["SCRIPT_FILENAME"] = request.m_stat_file; //so it works with php-cgi
	this->m_env_map["SERVER_NAME"] =SERVER_VERSION;
	this->m_env_map["SERVER_PORT"] = request.m_owner->m_v_server->m_port;
	this->m_env_map["SERVER_PROTOCOL"]="HTTP/1.1";
	this->m_env_map["SERVER_SOFTWARE"]="HTTP 1.1";
	this->m_env_map["REDIRECT_STATUS"]="true";
	this->m_env_map["HTTP_REFERER"]=request.m_headers[REFERER];
	for (std::map<std::string, std::string>::const_iterator it = request.x_headers.begin();
			it != request.x_headers.end(); ++it) {
		std::string	key = "HTTP_" + it->first;
		std::replace(key.begin(), key.end(), '-', '_');
		this->m_env_map[key] = it->second;
	}
}

void	Cgi::reset() {
	if (this->m_env_array) {
		for (size_t	i = 0; this->m_env_array[i]; i++) {
			if (this->m_env_array[i])
				free(this->m_env_array[i]);
			this->m_env_array[i] = 0;
		}
	}
	if (this->m_argv)
	{
		if (this->m_argv[0])
			free(this->m_argv[0]);
		if (this->m_argv[1])
			free(this->m_argv[1]);
		this->m_argv[0] = 0;
		this->m_argv[1] = 0;
	}
}

void	Cgi::reset(t_client &c) {
	this->reset();
	this->closeAll(c);
	this->kill(c);
}

void	Cgi::clear() {
	this->reset();
	if (this->m_env_array)
		free(this->m_env_array);
	if (this->m_argv)
		free(this->m_argv);
	this->m_env_array = 0;
	this->m_argv = 0;
}

void	RequestHandler::handleCgiResponse(t_client &c) {
	if (!c.m_response_data.m_cgi_metadata_parsed) {

		size_t	metadata_index = ft::fullMetaData(c.m_cgi_out_buf);
		if (metadata_index == std::string::npos) {
			if (c.m_cgi_end_chunk) { //script didn't provide http header
				c.m_response_str = this->generateErrorPage(500);
				c.m_response_data.m_cgi_metadata_parsed = true;
			}
			return ;
		}
		size_t	status_index = c.m_cgi_out_buf.find("Status: ");
		if (status_index != std::string::npos) {
			std::string status_str = c.m_cgi_out_buf.substr(status_index + 8,  std::string::npos);
			c.m_response_str.append(this->statusLine(ft::Atoi(status_str.c_str())));
		}
		else {
			c.m_response_str.append(this->statusLine(200));
		}
		c.m_response_str.append(this->GetDate());
		c.m_response_str.append(this->GetServer());
		c.m_response_str.append(this->GetTransferEncoding());
		c.m_response_str.append(c.m_cgi_out_buf, 0, metadata_index + CRLF_LEN);
		c.m_response_str.append(CRLF);
		c.m_response_data.m_cgi_metadata_parsed = true;
		c.m_cgi_out_buf.erase(0, metadata_index + CRLF_LEN + CRLF_LEN);
		if (!c.m_cgi_out_buf.empty()) {
			c.m_response_str.append(ft::hexString(c.m_cgi_out_buf.size()) + CRLF + c.m_cgi_out_buf + CRLF);
			c.m_cgi_out_buf.clear();
		}
	}
	if (c.m_response_data.m_cgi_metadata_sent) {
		if (c.m_cgi_out_buf.empty()) {
			c.m_cgi_end_chunk = 1;
		}
		c.m_response_str.append(ft::hexString(c.m_cgi_out_buf.size()) + CRLF + c.m_cgi_out_buf + CRLF);
		c.m_cgi_out_buf.clear();
	}
}

void	RequestHandler::handleCgiMetadata(t_request &request, std::string &stat_file) {
	request.m_cgi = true;
	request.m_path_info = request.m_uri;
	request.m_stat_file= stat_file;
	if (request.m_real_path.size() == stat_file.size()) {
		return ;
	}
	size_t query_string_index = request.m_uri.find('?', 0);
	if (query_string_index != std::string::npos) {
		request.m_query_string = request.m_uri.substr(query_string_index + 1, std::string::npos);
		request.m_path_info.resize(query_string_index);
	}
	Logger::Log()<<"m_stat_file: "<<request.m_stat_file<<std::endl;
	Logger::Log()<<"path_info: "<<request.m_path_info<<std::endl;
	Logger::Log()<<"query_string : "<<request.m_query_string<<std::endl;
}

bool	RequestHandler::validCgi(t_request &request, size_t extension_index) {
	if (!(request.m_method == GET || request.m_method == HEAD || request.m_method == POST))
		return false;
	t_directives::iterator	path_found = request.m_location->second.find("cgi_path");
	t_directives::iterator	extension_found = request.m_location->second.find("cgi");
	std::string	&file = request.m_file;
//
	if (extension_found != request.m_location->second.end()) {
		//compare  with uri
		if (file.compare(extension_index, extension_found->second.size(), extension_found->second))
			return false; //uri is not to be treated as cgi
		if (path_found == request.m_location->second.end()) {
			Logger::Log()<<"handleCgiMetadata: no path to cgi exec provided"<<std::endl;
			return false;
		}
		//check for existence of path here
		if (stat(path_found->second.c_str(), &this->m_statbuf)) {
			Logger::Log()<<"handleCgiMetadata: invalid path to cgi executable"<<std::endl;
			return false;
		}
		return true;
	}
	else if (path_found != request.m_location->second.end()) {
		Logger::Log()<<"handleCgiMetadata: no cgi extension provided"<<std::endl;
	}
	return false;
}


void	Cgi::setCgiFd(fd_set *read_set, fd_set *write_set, t_client &c) {
	FD_ZERO(read_set);
	FD_ZERO(write_set);
	c.m_range_fd = 0;
	if (!c.m_cgi_end_chunk) {
		FD_SET(c.m_cgi_read_pipe[IN], read_set);
		if (c.m_cgi_read_pipe[IN] > c.m_range_fd)
			c.m_range_fd = c.m_cgi_read_pipe[IN];
	}
	if (c.m_cgi_write_pipe[OUT] != -1) {
		FD_SET(c.m_cgi_write_pipe[OUT], write_set);
		if (c.m_cgi_write_pipe[OUT] > c.m_range_fd)
			c.m_range_fd = c.m_cgi_write_pipe[OUT];
	}
}

void	Cgi::kill(t_client &c)
{
	if (c.m_cgi_pid > 0) {
		if (waitpid(c.m_cgi_pid, NULL, WNOHANG) == -1) {
			Logger::Log() << "Cgi::kill: waitpid: " << strerror(errno)<<std::endl;
		}
		if (::kill(c.m_cgi_pid, 0) != -1 && ::kill(c.m_cgi_pid, SIGKILL) == -1) {
			Logger::Log() << "Cgi::kill: kill: " << strerror(errno)<<std::endl;
		}
		c.m_cgi_pid = -1;
	}
	c.m_cgi_running = false;
}

int RequestHandler::handleCgi(t_client &c) {
	pid_t	wpid = 0;
	int		wstatus = 0;
	fd_set	read_set, write_set;
	int	&read_fd = c.getReadFd();
	int	&write_fd = c.getWriteFd();

	try {
		//wait
		if (c.m_cgi_running) {
			wpid = waitpid(c.m_cgi_pid, &wstatus, WNOHANG);
			if (wpid == -1)
				throw HTTPError("RequestHandler::handleCgi : wait", strerror(errno), 500);
			if (wpid) {
				c.m_cgi_running = false;
				if (WEXITSTATUS(wstatus) ==  EXIT_FAILURE)
					throw HTTPError("Cgi::exec: execve", strerror(errno), 500);
			}
		}
		//select
		this->m_cgi.setCgiFd(&read_set, &write_set, c);

		struct timeval	tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		if (select(c.m_range_fd + 1, &read_set, &write_set, NULL, &tv) == -1)
			throw HTTPError("RequestHandler::handleCgi: select", strerror(errno), 500);
		//write
		if (write_fd != -1 &&  FD_ISSET(write_fd, &write_set)) {
			this->m_cgi.write(c);
		}
		//read
		if (!FD_ISSET(read_fd, &read_set))
			return CONTINUE;
		this->m_cgi.read(c);
		//generate response
		if (c.m_cgi_end_chunk) //just read last chunk
			this->m_cgi.closeAll(c);
		this->handleCgiResponse(c);
		if (!c.m_response_data.m_cgi_metadata_parsed)
			return CONTINUE; //hasn't received fullmetadata
	}
	catch (HTTPError &e) {
		std::cerr << e.what() << std::endl;
		this->m_cgi.closeAll(c);
		if (c.m_cgi_running)
			this->m_cgi.kill(c);
		this->m_client->m_request_data.m_status_code = e.HTTPStatusCode();
		//once the transfer has started, we can only send last chunk
		c.m_cgi_out_buf.clear();
		this->handleCgiResponse(c);
	}
	return SUCCESS;
}

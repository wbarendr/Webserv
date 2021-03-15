#include <iostream>
#include "Error.hpp"
#include "RequestHandler.hpp"
#include "WebServer.hpp"
#include "Server.hpp"
#include "utils.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

RequestHandler::RequestHandler() {
	initStatusCodes();
	initMimeTypes();
}

RequestHandler::~RequestHandler() {}

std::string RequestHandler::Content_Length(std::string const & body) {
	std::string content_length = "Content-Length: " + intToString(body.length());

	return content_length + CRLF;
}

std::string RequestHandler::Content_Type() {
	std::string content_type;

	return content_type + CRLF;
}

std::string RequestHandler::Server() {
	std::string server = "webserv/1.0.0";

	return server + CRLF;
}

std::string RequestHandler::statusLine() {
	int error_code = m_client->m_request_data.m_error;
	std::string	status_line;

	status_line.append("HTTP/1.1 ");
	status_line.append(intToString(error_code));
	status_line.append(" ");
	status_line.append(m_status_codes[error_code]);

	return status_line;
}

std::string RequestHandler::responseBody() {
	return "";
}

std::string RequestHandler::responseHeaders(std::string const & body) {

	m_response_headers.push_back(Server());
	m_response_headers.push_back(Content_Length(body));
	m_response_headers.push_back(Content_Type());


	std::string	response_headers;

	std::vector<std::string>::iterator it = m_response_headers.begin();
	for (; it != m_response_headers.end(); ++it) {
		response_headers.append(*it);
	}
	return response_headers;
}

std::string RequestHandler::handleGET() {
	std::string status_line = statusLine();
	std::string response_body = responseBody();
	std::string	response_headers = responseHeaders(response_body);

	return status_line + response_headers + CRLF + response_body;
}

std::string RequestHandler::handleHEAD() {
	std::string status_line = statusLine();
	std::string response_body = responseBody();
	std::string	response_headers = responseHeaders(response_body);

	return status_line + response_headers + CRLF;
}

std::string RequestHandler::handlePOST() {
	std::string status_line = statusLine();
	std::string response_body = responseBody();
	std::string	response_headers = responseHeaders(response_body);

	return status_line + response_headers + CRLF + response_body;
}

std::string RequestHandler::handlePUT() {
	std::string status_line = statusLine();
	std::string response_body = responseBody();
	std::string	response_headers = responseHeaders(response_body);

	return status_line + response_headers + CRLF + response_body;
}

std::string RequestHandler::handleDELETE() {
	std::string status_line = statusLine();
	std::string response_body = responseBody();
	std::string	response_headers = responseHeaders(response_body);

	return status_line + response_headers + CRLF + response_body;
}

std::string	RequestHandler::generateErrorPage(int error) {
	std::string status_line = statusLine();

	std::string	error_response =
			"<html>" CRLF
			"<head><title>" + intToString(error) + ' ' + m_status_codes[error] + "</title></head>" CRLF
			"<body>" CRLF
			"<center><h1>" + intToString(error) + ' ' + m_status_codes[error] + "</h1></center>" CRLF
			;

	std::string	response_headers = responseHeaders(error_response);

	return status_line + response_headers + CRLF + error_response;
}

void	RequestHandler::handleMetadata(t_client &c) {
	if (c.m_request_data.m_error)
		return ; //don't need to do anything if an error has been detected in RequestParser
	std::cout<<"handling metadata.."<<std::endl;

	this->m_client = &c;

	try {
		//updating virtual server pointer based on client request's host header
		m_client->updateServerConf();
		c.m_request_data.m_location = c.m_v_server->getLocation(c.m_request_data);
		c.m_request_data.m_owner = &c;
		std::cout<<"-------FETCHED BLOCK-------\n\tLISTEN "
			<<c.m_v_server->m_configs.m_directives["listen"]
			<<"\n\tSERVER_NAME "<< m_client->m_v_server->m_configs.m_directives["server_name"]
			<<"\n\tLOCATION/ROUTE "<< m_client->m_request_data.m_location->first<<"\n-----------"<<std::endl;
		//maybe Request could have m_real_path, m_path_info and m_query_string to be updated by the cgi part of this code,
		//	and later fetched by Cgi to populate the map
		// subsitute route + rest of URI and update m_real_path
		std::string &real_path =  c.m_request_data.m_real_path;
		std::string	file = c.m_request_data.m_file;
		real_path = c.m_request_data.m_location->second["root"] + c.m_request_data.m_path; // for now
		std::cout<<"real_path: "<<real_path<<std::endl;
		size_t	prefix = 0;
		size_t	next_prefix = 0;
		// stat every /prefix/ until
		// 						found file
		// 						end of URI
		// 						stat returns -1, so throw error not found
		while (prefix < real_path.size()) {
			prefix = real_path.find('/', prefix);
			next_prefix = prefix == std::string::npos ? std::string::npos : real_path.find('/', prefix + 1);
			file = real_path.substr(0, next_prefix);
			std::cout<<"\tstat "<<file<<std::endl;
			if (stat(file.c_str(), &this->m_statbuf)) {
				throw HTTPError("RequestHandler::handleMetadata", "invalid full path", 404);
			}
			//also check permission
			if ((this->m_statbuf.st_mode & S_IFMT) == S_IFREG) // if file
				break ;
			if ((this->m_statbuf.st_mode & S_IFMT) != S_IFDIR
					&& (this->m_statbuf.st_mode & S_IFMT) != S_IFLNK) { // if something else than a directory/smlink
				throw HTTPError("RequestHandler::handleMetadata", "invalid full path, not a file/directory/symlink", 404);
			}
			if (next_prefix == std::string::npos)
				break ;
			prefix = next_prefix;
		}
		if (next_prefix == std::string::npos)
			c.m_request_data.m_file = real_path.substr(real_path.size() - file.size(), std::string::npos);
		else
			c.m_request_data.m_file = real_path.substr(prefix + 1, std::string::npos);
		// if we stopped at file
		if ((this->m_statbuf.st_mode & S_IFMT) == S_IFREG) {
			std::cout<<"m_file: "<<c.m_request_data.m_file<<std::endl;
			std::cout<<"file: "<<file<<std::endl;
			// 	extract extension
			size_t	extension = c.m_request_data.m_file.find_last_of('.', c.m_request_data.m_file.size());
			std::cout<<"extension index: "<<extension<<std::endl;
			// 	if CGI directives exist && extension == cgi directive && cgi_path is valid
			if (this->validCgi(c.m_request_data, extension))
			{
				std::cout<<"cgi detected"<<std::endl;
				this->handleCgiMetadata(c.m_request_data, file);
			}
			else
			{
				//	normal file;
				//	check extension against mime types;
				//	if there are additional entries after this file, we throw bad request
				if (next_prefix != std::string::npos) {
					throw HTTPError("RequestHandler::handleMetadata", "invalid full path", 404);
				}
			}
		} 
		//		see if autoindex on then flag it so handleRequest can call the appropriate method
		//		else return bad request?
		//		//we dont need to check if remaining entries, because loop on 141 doesn't stop at a directory unless it's the last prefix in uri
		else if (c.m_request_data.m_location->second["autoindex"] == "on")
		{
			std::cout<<"dir listing"<<std::endl;
			c.m_request_data.m_autoindex= true;
		}
		else
			throw HTTPError("RequestHandler::handleMetadata", "directory listing not enabled", 404);

		// Authorization / WWW-Authenticate
		// Allow
		//
	} catch (HTTPError & e) {
		std::cerr << e.what() << std::endl;
		m_client->m_request_data.m_error = e.HTTPStatusCode();
		m_client->m_request_data.m_done = true;
	}
}

void	RequestHandler::handleRequest(t_client &c) {
	m_client = &c;
	Request	&request = m_client->m_request_data;

	if (request.m_error != 0) {
		m_client->m_response_str = generateErrorPage(request.m_error);
	} else if (c.m_request_data.m_cgi) {
		this->m_cgi.run(c);
	} else {
		switch (request.m_method) {
			case GET:
				m_client->m_response_str = handleGET();
				break;
			case HEAD:
				m_client->m_response_str = handleHEAD();
				break;
			case POST:
				m_client->m_response_str = handlePOST();
				break;
			case PUT:
				m_client->m_response_str = handlePUT();
				break;
			case DELETE:
				m_client->m_response_str = handleDELETE();
				break;
		}
	}
}

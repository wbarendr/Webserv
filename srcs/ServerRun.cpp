#include "Server.hpp"
#include "Error.hpp"
#include "Client.hpp"
#include "RequestParser.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include "Logger.hpp"




void	Server::run(){
	
	struct timeval tv;
	t_client 	*c;

	for (;;) {//run
		this->m_read_fd = this->m_read_all;
		this->m_write_fd = this->m_write_all;
		tv.tv_sec = 1; //on linux, tv is modified so need to reset for every call
		tv.tv_usec = 500000;
		if (select(this->m_range_fd + 1, &this->m_read_fd, &this->m_write_fd, NULL, &tv) == -1)
			throw(serverError("select: ", strerror(errno)));
		for (int i =0; i <= this->m_range_fd ; ++i){
			if (FD_ISSET(i, &this->m_read_fd)) {
				if (this->accept(i) == SUCCESS)
					continue ;
				c = getClient(i);
				if (this->receive(c) > 0) {
				 	if (!c->m_request_data.m_metadata_parsed) {
				 		if (ft::fullMetaData(c->m_request_str) == std::string::npos)
				 			continue ;
				 		RequestParser::Parse(*c);
						RequestParser::Print(*c);
						if (c->m_request_data.m_status_code < 400)
				 			this->m_request_handler.handleMetadata(*c); 
				 	}
					else if (!c->m_request_data.m_done){
				 		RequestParser::GetBody(*c, false);
						RequestParser::Print(*c);
					}
				 	if (c->m_request_data.m_done)
					{
				 		this->m_request_handler.handleRequest(*c);
						FD_CLR(c->m_socket, &this->m_read_all);
						FD_SET(c->m_socket, &this->m_write_all);
						c->m_request_str.clear();
					}
				}
				else {
					this->removeClient(*c);
				}
			} // FD_ISSET(this->read_fd)
			else if (FD_ISSET(i, &this->m_write_fd)) {
				c = getClient(i);
				if (c->m_request_data.m_status_code < 400 && c->m_request_data.m_cgi && this->m_request_handler.handleCgi(*c) == CONTINUE)
					continue ;
				this->respond(*c);
			}
		} // for (i in range_fd)
	} // for(;;)
} //ServerRun

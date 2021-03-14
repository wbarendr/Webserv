#include "Client.hpp"
#include "Server.hpp"
#include "Error.hpp"
#include "WebServer.hpp"

Request::Request()
	: m_method(-1),
	m_path(""),
	m_protocol(-1),
	m_content_length(0),
	m_headers(0),
	m_if_body(false),
	m_body(""),
	m_metadata_parsed(false),
	m_done(false),
	m_chunked(false),
	m_error(200),
	m_start(0),
	m_location()
{
	for (int i = 0; i < 18; ++i)
		m_headers.push_back("");
}

Response::Response()
	: m_method(-1),
	m_path(""),
	m_protocol(-1),
	m_content_length(0),
	m_headers(0),
	m_if_body(false),
	m_body(""),
	m_error(200)
{
	for (int i = 0; i < 18; ++i)
		m_headers.push_back("");
}

Client::Client() 
	: m_request_str(""),
	m_response_str(""),
	m_request_data(),
	m_response_data(),
	m_v_server(0),
	m_socket(-1),
	m_sockaddr(),
	m_addrlen(sizeof(m_sockaddr))
{
}

bool	Client::fullMetaData() {
	return (!this->m_request_str.empty()
			&& this->m_request_str.find("\r\n\r\n") != std::string::npos);
}

Client::Client(Client const & src)
	: m_request_str(src.m_request_str),
	m_response_str(src.m_response_str),
	m_request_data(src.m_request_data),
	m_response_data(src.m_response_data),
	m_socket(src.m_socket),
	m_sockaddr(src.m_sockaddr),
	m_addrlen(src.m_addrlen)
{

}


// bool	Client::fullMetaData() {
// 	return (!this->m_request_str.empty()
// 			&& this->m_request_str.find("\r\n\r\n") != std::string::npos);
// }

void	Server::removeClient(int client_socket) {
	if (this->m_client_map.erase(client_socket) != 1)
		throw serverError("removeClient: ", "trying to remove unexisting client");
}

// <<<<<<< HEAD
// bool	Client::fullMetaData()
// {
// 	if (m_request_str.find("\n\n") != std::string::npos)
// 		return true;
// 	return false;
void	Client::updateServerConf()
{
	std::string host2 = this->m_request_data.m_headers[HOST];
	if (host2.empty()) {
		throw HTTPError("updateServerConf", "empty host header." , 400);
	}
	host2.resize(host2.size() - 1); // remove when problem fixed in RequestParser
	for (size_t i = 0; i < (*(this->m_v_server_blocks)).size(); ++i) {
		if ((*(this->m_v_server_blocks))[i].m_configs.m_directives["server_name"] == host2) {
			this->m_v_server = &(*(this->m_v_server_blocks))[i];//select server by server_name
			return ;
		}
	}
	this->m_v_server = &((*(this->m_v_server_blocks))[0]);//if not found, return the first added,default one
}

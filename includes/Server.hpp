#ifndef SERVER_HPP
# define SERVER_HPP

#include <vector>
#include <map>
#include <string>

#include "Conf.hpp"
#include "VirtualServer.hpp"
#include "RequestHandler.hpp"
#include "Client.hpp"
#include "Cgi.hpp"

class	Server
{
	public:

		typedef	s_v_server_conf							t_v_server_conf;
		typedef	Client									t_client;
		typedef	std::string								t_ip_port;
		typedef	VirtualServer							t_v_server;
		typedef	RequestHandler							t_request_handler;
		typedef	int										t_port;
		typedef	std::vector<t_v_server>					t_v_server_blocks;
		typedef	t_v_server_conf::t_directives			t_directives;
		typedef std::map<t_ip_port, t_v_server_blocks> 	t_v_server_map;
		typedef std::map<int, t_client> 				t_client_map;

		friend class RequestHandler;

		explicit Server();
		~Server();

		void	parse(char const *path);
		void	run();
		void	init();
		void	close();
		ssize_t	receive(t_client *c);
		void	respond(int client_socket);
		void	respond(t_client &c);
		int		accept(int socket);
		void	addClient();
		void	connectVirtualServer(t_v_server &v_server);
		void	closeClientConnection(t_client &c);
		void	removeClient(t_client &c);
		
		t_v_server_blocks	*getVirtualServer(int socket);
		t_client			*getClient(int client_socket);

		fd_set		m_read_all;
		fd_set		m_write_all;
		fd_set		m_read_fd;
		fd_set		m_write_fd;
		int			m_range_fd;

	private:
		t_v_server_map 		m_v_server_all;
		t_client_map		m_client_all;
		t_request_handler	m_request_handler;
};

#endif

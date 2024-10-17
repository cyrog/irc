#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <set>
#include <csignal>
#include "termios.h"
#include "client.hpp"
#include "channel.hpp"

#define MAX_CLIENTS 5

class Client;
class Channel;

class Server {
private:
	std::string			buffer;
	std::string			password;

	int					port;
	int					serverSocket;

	struct sockaddr_in	serverADDR;
	struct pollfd		fds[MAX_CLIENTS + 1];

	std::vector<Client*>		client_instances;
	std::vector<Channel>		channels;
	std::vector<std::string>	commands;
	std::vector<std::string>	parseCmd(const std::string &str);

	//connection
	void 		new_client();
	void		disconnect_client(int client_socket);
	void		register_client(int client_socket);

	//commands
	void		setTopic(int clientSocket, const std::string &channel, const std::string &topic);
	void		getTopic(int clientSocket, const std::string &channel);
	void		join_channel(int client_socket, const std::string& channel_name, const std::string& password);
	void		leave_channel(int client_socket, const std::string& channel_name);
	void		AddOpToChannel(int client_socket, const std::string& channel_name, const std::string& nick, const std::string& mode);
	void		inviteToJoin(int client_socket, const std::string& channel_name, const std::string& nickname);
	void		kickClient(int client_socket, const std::string& channel_name, const std::string& nickname, const std::string& comment);
	void		sendList(int client_socket, const std::string& channel_name);
	void		sendWhoList(int client_socket, const std::string& channel_name);
	void		display_message(int client_socket, const std::vector<std::string> &tokens);
	void		ChannelUserLimit(int client_socket, const std::string& mode, std::string nb, std::string channel_name);
	void		ChannelTopicRestriction(int client_socket, const std::string& channel_name, const std::string& mode);
	void		ChannelPassword(int client_socket, const std::string& channel_name, const std::string& mode, const std::string& password);
	void		ChannelInvite(int client_socket, const std::string& channel_name, const std::string& mode);

	//parsing
	void 		handle_client_inputs(int client_index);
	void 		processInput(int client_socket, const char* input, ssize_t length);
	void		handleCommand(int client_socket, const std::vector<std::string> &tokens);
	void		handleConnection(int client_socket, const std::vector<std::string> &tokens);
	void		parseMode(int client_socket, const std::vector<std::string> &tokens);


	//utils
	std::string	getNickBySocket(int client_socket);
	std::string	getfullname(int client_socket);
	Channel*	getChannelByName(const std::string& channel_name);
	bool		checkChannel(int client_socket, const std::string& channel_name);
	void		setnickname(int client_socket, std::string nickname);
	void		sendToClient(int client_socket, const std::string &message);
	int			getGudClientIndex(int client_socket);
	int			getSocketByName(const std::string& nickname);

public:
	Server(int port, std::string password);
	~Server();

	void	start();
	int		getServerSocket() const;
};

void	sighandler(int signal);
void	init_termios();

#endif
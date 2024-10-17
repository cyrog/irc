
#include "server.hpp"

volatile sig_atomic_t sig = 1;

int	Server::getServerSocket() const {
	return (this->serverSocket);
}

void	sighandler(int signal) {
	if (signal == SIGINT)
		sig = 0;
}

void	init_termios() {
	struct termios	termios;
	if ((tcgetattr(STDIN_FILENO, &termios)) == -1)
		exit(EXIT_FAILURE);
	termios.c_lflag &= ~(ECHOCTL);
	if ((tcsetattr(STDERR_FILENO, TCSANOW, &termios)) == -1)
		exit(EXIT_FAILURE);
}

Server::Server(int port, std::string password) : port(port), password(password) {
    client_instances.resize(MAX_CLIENTS, nullptr);
	this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1) {
		std::cerr << "\033[31mERROR while creating socket." << std::endl;
		exit(EXIT_FAILURE);
	}
	int opt = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		std::cerr << "\033[31mERROR while config socket." << std::endl;
		exit(EXIT_FAILURE);
	}
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
		std::cerr << "\033[31mERROR while config socket." << std::endl;
		exit(EXIT_FAILURE);
	}
	std::memset(&serverADDR, 0, sizeof(serverADDR));
	serverADDR.sin_family = AF_INET;
	serverADDR.sin_addr.s_addr = htonl(INADDR_ANY);
	serverADDR.sin_port = htons(this->port);
	if (bind(serverSocket, (struct sockaddr *)&serverADDR, sizeof(serverADDR)) == -1) {
		std::cerr << "\033[31mERROR while linking socket to address and port." << std::endl;
		exit(EXIT_FAILURE);
	}
	if (listen(serverSocket, SOMAXCONN) == -1) {
		std::cerr << "\033[31mERROR while starting to lisen on socket" << std::endl;
		exit(EXIT_FAILURE);
	}
	fds[0].fd = this->serverSocket;
	fds[0].events = POLLIN;
}

Server::~Server() {
	for (int i = 1; i < MAX_CLIENTS + 1; i++) {
		if (fds[i].fd > 0)
			close(fds[i].fd);
	}
	for (std::vector<Client*>::iterator it = client_instances.begin(); it != client_instances.end(); ++it) {
		delete *it;
	}
	client_instances.clear();
	close(serverSocket);
	std::cout << "\033[32mIRC server shut down successfully.\033[0m" << std::endl;
}

void	Server::start() {
	init_termios();
	signal(SIGINT, sighandler);
	std::cout << "\033[32mIRC server launched and waiting for connection on port: " << this->port << "\033[0m" <<  std::endl;
	while (sig) {
		int activity = poll(&fds[0], MAX_CLIENTS + 1,  -1);
		if ((activity < 0) && (errno != EINTR)) {
			std::cerr << "Poll error." << std::endl;
			break;
		}
		for(int i = 0; i < MAX_CLIENTS; i++) {
			if (fds[i].revents & POLLIN) {
				if (fds[i].fd == this->serverSocket)
					new_client();
				else
					handle_client_inputs(fds[i].fd); 
			}
		}
	}
}

void	Server::new_client() {
	int new_socket;
	struct sockaddr_in client_addr;
	int addrlen = sizeof(client_addr);

	if ((new_socket = accept(this->serverSocket, (struct sockaddr *)&client_addr, (socklen_t*)&addrlen)) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (client_instances[i] == nullptr) {
			fcntl(new_socket, F_SETFL, fcntl(new_socket, F_GETFL) | O_NONBLOCK);
			client_instances[i] = new Client(new_socket);
			fds[i + 1].fd = new_socket;
			fds[i + 1].events = POLLIN;
			std::cout << "\033[33mAdded client with socket " << new_socket << " stored in client_instances[" << i << "] and in fds[" << i + 1 << "]\033[37m" << std::endl;
			break;
		}
	}
}

std::vector<std::string> Server::parseCmd(const std::string &str) {
    std::vector<std::string> vec;
    std::istringstream stm(str);
    std::string token;

    while (stm >> token)
        vec.push_back(token);
    return vec;
}

void Server::processInput(int client_index, const char* input, ssize_t length) {
    buffer.append(input, length);
    size_t pos;
    while ((pos = buffer.find("\r\n")) != std::string::npos) {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 2);

		std::cout << "Receive from client: " << line << std::endl;

        std::vector<std::string> tokens = parseCmd(line);
		if (getGudClientIndex(client_index) == -1) {
			return;
		}
        if (!tokens.empty()) {
			if (!client_instances[getGudClientIndex(client_index)]->isOn())
				handleConnection(client_index, tokens);
			else
				handleCommand(client_index, tokens);
        }
    }
}

void Server::handleConnection(int client_socket, const std::vector<std::string> &tokens) {
	if (tokens[0] == "CAP") {
        if (tokens.size() > 1 && tokens[1] == "LS")
            sendToClient(client_socket, ":irc.example.net CAP * LS :multi-prefix\r\n");
		else if (tokens.size() > 1 && tokens[1] == "REQ")
            sendToClient(client_socket, ":irc.example.net CAP * ACK :multi-prefix\r\n");
    }
	else if (tokens[0] == "JOIN" && tokens[1] == ":")
    	sendToClient(client_socket, ":irc.example.net 451 * :Connection not registered\r\n");
	else if (tokens[0] == "PASS") {
		if (tokens[1] != this->password) {
			sendToClient(client_socket, ":server.name 464 * :Password incorrect\r\n");
			disconnect_client(client_socket);
			return;
		}
		else
			register_client(client_socket);
    }
	else if (tokens[0] == "NICK")
		setnickname(client_socket, tokens[1]);
	else if (tokens[0] == "USER" && tokens.size() > 1) {
		client_instances[getGudClientIndex(client_socket)]->setUser(tokens[1]);
		client_instances[getGudClientIndex(client_socket)]->setIp(tokens[3]);
		client_instances[getGudClientIndex(client_socket)]->setReal(tokens[4], tokens[5]);
	}
}

void Server::handle_client_inputs(int client_socket) {   
	char recv_buffer[1024];
    ssize_t valread;
    valread = recv(client_socket, recv_buffer, sizeof(recv_buffer) - 1, 0);
    recv_buffer[valread] = '\0';

    if (valread <= 0)
        disconnect_client(client_socket);
	else
		processInput(client_socket, recv_buffer, valread);
}

void Server::handleCommand(int client_socket, const std::vector<std::string> &tokens) {
	if (tokens[0] == "PING") {
        if (tokens.size() > 1)
            sendToClient(client_socket, "PONG " + tokens[1] + "\r\n");
    }
	else if (tokens[0] == "USER" && tokens.size() > 1) {
		client_instances[getGudClientIndex(client_socket)]->setUser(tokens[1]);
		client_instances[getGudClientIndex(client_socket)]->setIp(tokens[3]);
		client_instances[getGudClientIndex(client_socket)]->setReal(tokens[4], tokens[5]);
	}
	else if (tokens[0] == "JOIN" && tokens.size() == 3)
		join_channel(client_socket, tokens[1], tokens[2]);
	else if (tokens[0] == "JOIN" && tokens.size() == 2)
		join_channel(client_socket, tokens[1], "");
	else if (tokens[0] == "PART" && tokens.size() > 1)
		leave_channel(client_socket, tokens[1]);
	else if (tokens[0] == "WHO" && tokens.size() == 2)
		sendWhoList(client_socket, tokens[1]);
	else if (tokens[0] == "PRIVMSG" && tokens.size() > 2)
		display_message(client_socket, tokens);
	else if (tokens[0] == "MODE")
		parseMode(client_socket, tokens);
	else if (tokens[0] == "TOPIC" && tokens.size() > 1) {
		if (tokens.size() >= 2) {
			std::string topic;
			for (size_t i = 2; i < tokens.size(); ++i) {
				if (i > 2) {
					topic += " ";
				}
				topic += tokens[i];
			}
			setTopic(client_socket, tokens[1], topic);
		}
		else {
			getTopic(client_socket, tokens[1]);
		}
	}
	else if ((tokens[0] == "INVITE") && tokens.size() > 2)
		inviteToJoin(client_socket, tokens[1], tokens[2]);
	else if (tokens[0] == "KICK" && tokens.size() > 2) {
		if (tokens.size() < 2) {
			sendToClient(client_socket, ":irc.example.com 461 " + getfullname(client_socket) + " KICK :Not enough parameters\r\n");
        	return;
		}
		std::string comment;
		for (size_t i = 3; i < tokens.size(); ++i) {
			comment += tokens[i] + " ";
		}
		if (!comment.empty())
			comment.pop_back();
		if (comment.empty())
			comment += "No reason given";
		kickClient(client_socket, tokens[1], tokens[2], comment);
	}
	else
		return;
}

void Server::sendToClient(int client_socket, const std::string &message) {
    send(client_socket, message.c_str(), message.length(), 0);
    std::cout << "Sent to client: " << message << std::endl;
}

void Server::disconnect_client(int client_socket) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (fds[i + 1].fd == client_socket) {
			std::cout << "\033[33mclient with socket " << client_socket << " stored in client_instances[" << i << "] and in fds[" << i + 1 << "] disconnected...\033[37m" << std::endl;
			if (client_instances[i]) {
				delete client_instances[i];
				client_instances[i] = nullptr;
			}
			for (int j = i + 1; j < MAX_CLIENTS; ++j) {
				fds[j] = fds[j + 1];
			}
			fds[MAX_CLIENTS].fd = -1;
			fds[MAX_CLIENTS].events = 0;
			fds[MAX_CLIENTS].revents = 0;
			close(client_socket);
			return;
		}
	}
}

void Server::register_client(int client_socket) {
    for (int i = 0; i < client_instances.size(); i++) {
        if (client_instances[i] && client_instances[i]->getSocket() == client_socket) {
            client_instances[i]->setRegistered(true);
			if (client_instances[i]->getNick().empty())
				return;
            sendToClient(client_socket, ":irc.example.net 001 " + client_instances[i]->getNick() + " :Welcome to the Internet Relay Network\r\n");
            sendToClient(client_socket, ":irc.example.net 002 " + client_instances[i]->getNick() + " :Your host is irc.example.net, running version 1.0\r\n");
            sendToClient(client_socket, ":irc.example.net 003 " + client_instances[i]->getNick() + " :This server has been started today\r\n");
            sendToClient(client_socket, ":irc.example.net 004 " + client_instances[i]->getNick() + " irc.example.net 1.0 oiws\r\n");
            std::cout << "\033[33mClient with socket " << client_socket << " has been registered.\033[37m" << std::endl;
			client_instances[i]->setOn(true);
            return;
        }
    }
    std::cerr << "Client with socket " << client_socket << " not found." << std::endl;
}

void Server::setnickname(int client_socket, std::string nickname) {
	client_instances[getGudClientIndex(client_socket)]->setNick(nickname);
	if (client_instances[getGudClientIndex(client_socket)]->isRegistered() == true)
		register_client(client_socket);
	else {
		sendToClient(client_socket, ":server.name 464 * :Password incorrect\r\n");
		disconnect_client(client_socket);
	}
}

void	Server::join_channel(int client_socket, const std::string& channel_name, const std::string& password) {
	Channel* channel = getChannelByName(channel_name);
	if(!channel) {
		channels.push_back(Channel(channel_name));
		channel = &channels.back();
		channel->addOperator(getNickBySocket(client_socket));
	}
	if (channel->getInviteOnlyState() == true) {
		if (channel->isInvited(getNickBySocket(client_socket)) == true) {
			channel->addClient(client_socket);
			const std::vector<int>& clients = channel->getClients();
			for (size_t i = 0; i < clients.size(); ++i) {
        		sendToClient(clients[i], + ":" + getfullname(client_socket) + " JOIN :" + channel_name + "\r\n");
    		}
			client_instances[getGudClientIndex(client_socket)]->setChannelJoined(channel_name);
			sendList(client_socket, channel_name);
			return;
		}
		else {
			sendToClient(client_socket, ":irc.example.net 473 " + getfullname(client_socket) + " " + channel_name + " :Cannot join channel (+i)\r\n");
			return;
		}
	}
	if (channel->gethasPassword() == true && channel->getPassword() != password) {
		sendToClient(client_socket, ":irc.example.net 475 " + getfullname(client_socket) + " " + channel_name + " :Cannot join channel (+k)\r\n");
		return;
	}
	if (channel->getMaxUsers() != 0 && channel->returnNumberOfUsers() >= channel->getMaxUsers()) {
		sendToClient(client_socket, ":irc.example.net 471 " + getfullname(client_socket) + " " + channel_name + " :Cannot join channel (+l)\r\n");
		return;
	}
	channel->addClient(client_socket);
	const std::vector<int>& clients = channel->getClients();
	for (size_t i = 0; i < clients.size(); ++i) {
        sendToClient(clients[i], + ":" + getfullname(client_socket) + " JOIN :" + channel_name + "\r\n");
    }
	client_instances[getGudClientIndex(client_socket)]->setChannelJoined(channel_name);
	sendList(client_socket, channel_name);
}

void	Server::leave_channel(int client_socket, const std::string& channel_name) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	channel->removeClient(client_socket);
	sendToClient(client_socket, ":" + getfullname(client_socket) + " PART " + channel_name + "\r\n");
	const std::vector<int>& clients = channel->getClients();
	for (size_t j = 0; j < clients.size(); ++j) {
    	sendToClient(clients[j], ":" + getfullname(client_socket) + " PART " + channel_name + "\r\n");
    }
	client_instances[getGudClientIndex(client_socket)]->setChannelJoined("");
	channel->removeInvitedClient(getNickBySocket(client_socket));
}

void Server::display_message(int client_socket, const std::vector<std::string> &tokens) {
	std::string message;
	for(size_t q = 2; q < tokens.size(); ++q) {
		message += tokens[q];
		if (q != tokens.size() - 1)
			message += " ";
	}
	Channel* channel = getChannelByName(tokens[1]);
	if (channel) {
		const std::vector<int>& clients = channel->getClients();
		for(size_t j = 0; j < clients.size(); ++j) {
			if (clients[j] != client_socket)
				sendToClient(clients[j], ":" + getfullname(client_socket) + " PRIVMSG " + tokens[1] + " " + message + "\r\n");
		}
		return;
	}	
	for(size_t i = 0; i < client_instances.size(); i++) {
		if (tokens[1] == client_instances[i]->getNick()) {
			sendToClient(client_instances[i]->getSocket(), ":" + getfullname(client_socket) + " PRIVMSG " + tokens[1] + " " + message + "\r\n");
			return;
		}
	}
}

void	Server::AddOpToChannel(int client_socket, const std::string& channel_name, const std::string& nick, const std::string& mode) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	int target_socket = getSocketByName(nick);
	bool userfound = channel->isClientInChannel(target_socket);
	if (userfound == true && mode == "+o") {
		if (channel->isOperator(nick))
			return;
		channel->addOperator(nick);
        std::string operMessage = ":" + getNickBySocket(client_socket) + " MODE " + channel_name + " +o " + nick + "\r\n";
        for (size_t i = 0; i < channel->getClients().size(); ++i) {
            sendToClient(channel->getClients()[i], operMessage);
        }
	}
	else if (userfound == true && mode == "-o")
	{
		if (!channel->isOperator(nick))
			return;
		channel->removeOperator(nick);
		std::string operMessage = ":" + getNickBySocket(client_socket) + " MODE " + channel_name + " -o " + nick + "\r\n";
        for (size_t i = 0; i < channel->getClients().size(); ++i) {
            sendToClient(channel->getClients()[i], operMessage);
        }
	}
	else
		sendToClient(client_socket, ":irc.example.net 401 " + getfullname(client_socket) + " " + nick + " :No such user in this channel\r\n");
}

void Server::inviteToJoin(int client_socket, const std::string& channel_name, const std::string& nickname) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	int target_socket = getSocketByName(nickname);
	if (target_socket == -1) {
        sendToClient(client_socket, ":irc.example.net 401 :" + getfullname(client_socket) + " " + nickname + " :No such nick/channel\r\n");
        return;
    }
		channel->addInvitedClient(nickname);
		sendToClient(target_socket, ":" + getfullname(client_socket) + " INVITE " + nickname + " :" + channel_name + "\r\n");
		const std::vector<int>& clients = channel->getClients();
		for (size_t j = 0; j < clients.size(); ++j) {
        if (clients[j] != target_socket)
			sendToClient(clients[j], ":" + getfullname(client_socket) + " INVITE " + nickname + " :" + channel_name + "\r\n");
    }
}

void Server::kickClient(int client_socket, const std::string& channel_name, const std::string& nickname, const std::string& comment) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	int target_socket = getSocketByName(nickname);
	if (target_socket == -1 || !channel->isClientInChannel(target_socket)) {
        sendToClient(client_socket, ":irc.example.net 401 " + getfullname(client_socket) + " " + nickname + " :No such user in this channel\r\n");
        return;
    }
	channel->removeClient(target_socket);
	client_instances[getGudClientIndex(target_socket)]->setChannelJoined("");
	sendToClient(target_socket,  ":" + getNickBySocket(client_socket) + " KICK " + channel_name + " " + getNickBySocket(target_socket) + " " + comment + "\r\n");
	sendToClient(target_socket,  ":" + getNickBySocket(client_socket) + " NOTICE " + channel_name + " " + getNickBySocket(target_socket) + " You have been kicked from this channel (" + comment + ")\r\n");
    const std::vector<int>& clients = channel->getClients();
    for (size_t j = 0; j < clients.size(); ++j) {
        if (clients[j] != target_socket)
			sendToClient(clients[j], ":" + getNickBySocket(client_socket) + " KICK " + channel_name + " " + getNickBySocket(target_socket) + " " + comment + "\r\n");
    }
}

void Server::sendList(int client_socket, const std::string& channel_name) {
	std::string user_list;
	Channel* channel = getChannelByName(channel_name);
	if (channel) {
		const std::vector<int>& clients = channel->getClients();
		for (size_t i = 0; i < clients.size(); ++i) {
			std::string nick = getNickBySocket(clients[i]);
			if (channel->isOperator(nick))
				user_list += "@" + nick + " ";
			else
				user_list += nick + " ";
		}
		if (!user_list.empty())
			user_list.pop_back();
	}
	std::string modes = channel->getModes();
	sendToClient(client_socket, ":irc.example.net 332 " + getNickBySocket(client_socket) + " " + channel_name + " " + channel->getTopic() + "\r\n");
	sendToClient(client_socket, ":irc.example.net 353 " + getNickBySocket(client_socket) + " = " + channel_name + " :" + user_list + "\r\n");
	sendToClient(client_socket, ":irc.example.net 366 " + getfullname(client_socket) + " " + channel_name + " :End of /NAMES list.\r\n");
	sendToClient(client_socket, ":irc.example.net 324 " + getfullname(client_socket) + " " + channel_name + " +" + modes + "\r\n");
}

void	Server::parseMode(int client_socket, const std::vector<std::string> &tokens) {
	if (tokens.size() < 2) {
		sendToClient(client_socket, ":irc.example.com 461 :" + getfullname(client_socket) + " MODE :Not enough parameters\r\n");
		return;
	}
	Channel* channel = getChannelByName(tokens[1]);
	if (!channel) {
		if (tokens[1] == getNickBySocket(client_socket) && tokens.size() == 3)
			return;
		sendToClient(client_socket, ":irc.example.com 403 " + getfullname(client_socket) + " " + tokens[1] + " :No such channel\r\n");
		return;
	}
	if (tokens[0] == "MODE" && tokens.size() == 2) {
		const std::vector<int>& clients = channel->getClients();
		for (size_t i = 0; i < clients.size(); ++i) {
			if (clients[i] == client_socket) {
				std::string modes = channel->getModes();
				sendToClient(client_socket, ":irc.example.net 324 " + getfullname(client_socket) + " " + tokens[1] + " +" + modes + "\r\n");
				return;
			}
		}
		sendToClient(client_socket, ":irc.example.net 442 " + getfullname(client_socket) + " " + tokens[1] + " :You're not on that channel\r\n");
	}
	else if (tokens[0] == "MODE" && tokens.size() == 3 && tokens[2] == "b")
		sendToClient(client_socket, ":irc.example.net 368 " + getfullname(client_socket) + tokens[1] + ":End of channel ban list.\r\n");
	else if (tokens[0] == "MODE" && tokens.size() == 4 && (tokens[2] == "+o" || tokens[2] == "-o"))
		AddOpToChannel(client_socket, tokens[1], tokens[3], tokens[2]);
	else if (tokens[0] == "MODE" && tokens.size() == 4 && tokens[2] == "+l")
		ChannelUserLimit(client_socket, tokens[2], tokens[3], tokens[1]);
	else if (tokens[0] == "MODE" && tokens[2] == "-l" && tokens.size() == 3)
		ChannelUserLimit(client_socket, tokens[2], "null", tokens[1]);
	else if (tokens[0] == "MODE" && tokens.size() == 3 && (tokens[2] == "+t" || tokens[2] == "-t"))
		ChannelTopicRestriction(client_socket, tokens[1], tokens[2]);
	else if (tokens[0] == "MODE" && tokens.size() == 4 && (tokens[2] == "+k" || tokens[2] == "-k"))
		ChannelPassword(client_socket, tokens[1], tokens[2], tokens[3]);
	else if (tokens[0] == "MODE" && tokens.size() == 3 && (tokens[2] == "+i" || tokens[2] == "-i"))
		ChannelInvite(client_socket, tokens[1], tokens[2]);
}

Channel* Server::getChannelByName(const std::string& channel_name) {
    for (size_t i = 0; i < channels.size(); ++i) {
        if (channels[i].getChannelName() == channel_name) {
            return &channels[i];
        }
    }
    return nullptr;
}

int	Server::getGudClientIndex(int client_socket) {
	for (int i = 0; i < client_instances.size(); i++) {
		if (client_instances[i] && client_instances[i]->getSocket() == client_socket) {
			return (i);
		}
	}
	return (-1);
}

std::string Server::getfullname(int client_socket) {
	std::string fullname;
	fullname += getNickBySocket(client_socket) + "!~" + client_instances[getGudClientIndex(client_socket)]->getUser() + "@" + client_instances[getGudClientIndex(client_socket)]->getIp();
	return (fullname); 
}

void Server::sendWhoList(int client_socket, const std::string& channel_name) {
	Channel* channel = getChannelByName(channel_name);
	if (channel) {
		const std::vector<int>& clients = channel->getClients();
		for (size_t i = 0; i < clients.size(); ++i) {
			std::string nick = client_instances[getGudClientIndex(clients[i])]->getNick();
			std::string username = client_instances[getGudClientIndex(clients[i])]->getUser();
			std::string hostname = client_instances[getGudClientIndex(clients[i])]->getIp();
			std::string realname = client_instances[getGudClientIndex(clients[i])]->getReal();
			std::string op_status = channel->isOperator(nick) ? "H@" : "H";
			sendToClient(client_socket, "irc.example.net 352 " + getNickBySocket(client_socket) + " " +
				channel_name + " " + username + " " + hostname + " irc.example.net " + nick + " " + op_status + " :0 " + realname + "\r\n");
		}
		sendToClient(client_socket, ":irc.example.net 315 " + getfullname(client_socket) + " " + channel_name + " :End of /WHO list.\r\n");
	}
}

std::string Server::getNickBySocket(int client_socket) {
	return(client_instances[getGudClientIndex(client_socket)]->getNick());
}

void	Server::ChannelUserLimit(int client_socket, const std::string& mode, std::string nb, std::string channel_name) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	if (mode == "+l") {
		channel->setMaxUsers(std::stoi(nb));
		channel->setModes("+l");
	}
	else if (mode == "-l") {
		channel->setMaxUsers(0);
		channel->setModes("-l");
	}
	const std::vector<int>& clients = channel->getClients();
	for (size_t i = 0; i < clients.size(); ++i) {
		sendToClient(clients[i], ":" + getfullname(client_socket) + " MODE " + channel_name + " " + mode + "\r\n");
	}
}

void Server::setTopic(int client_socket, const std::string &channel_name, const std::string &topic) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->getTopicToOp() == true && channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
    channel->setTopic(topic);
	const std::vector<int>& clients = channel->getClients();
	for (int i = 0; i < clients.size(); ++i) {
		sendToClient(clients[i], ":" + getfullname(client_socket) + " TOPIC " + channel_name + " " + topic + "\r\n");	
	}
}

void Server::getTopic(int client_socket, const std::string &channel_name) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
    if (!channel->getTopic().empty()) {
        sendToClient(client_socket, ":irc.example.net 332 " + channel_name + " :" + channel->getTopic() + "\r\n");
    } 
	else {
        sendToClient(client_socket, ":irc.example.net 331 " + channel_name + " :No topic is set\r\n");
	}
}

int Server::getSocketByName(const std::string& nickname) {
	for (size_t i = 0; i < client_instances.size(); ++i) {
		if (client_instances[i] && client_instances[i]->getNick() == nickname)
			return (client_instances[i]->getSocket());
	}
	return (-1);
}

bool Server::checkChannel(int client_socket, const std::string& channel_name) {
	if (channel_name != client_instances[getGudClientIndex(client_socket)]->getChannelJoined()) {
		sendToClient(client_socket, ":irc.example.net 442 " + getfullname(client_socket) + " " + channel_name + " :You're not on that channel\r\n");
		return false;
	}
	Channel *channel = getChannelByName(channel_name);
	if (!channel) {
		sendToClient(client_socket, ":irc.example.com 403 " + getfullname(client_socket) + " " + channel_name + " :No such channel\r\n");
		return false;
	}
	return true;
}

void Server::ChannelTopicRestriction(int client_socket, const std::string& channel_name, const std::string& mode) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	if (mode == "+t") {
		if (channel->getTopicToOp() == true)
			return;
		channel->setModes("+t");
		channel->setTopicToOp(true);
	}
	else if (mode == "-t") {
		if (channel->getTopicToOp() == false)
			return ;
		channel->setModes("-t");
		channel->setTopicToOp(false);	
	}
	const std::vector<int>& clients = channel->getClients();
	for (size_t i = 0; i < clients.size(); ++i) {
		sendToClient(clients[i], ":" + getfullname(client_socket) + " MODE " + channel_name + " " + mode + "\r\n");
	}
}

void Server::ChannelPassword(int client_socket, const std::string& channel_name, const std::string& mode, const std::string& password) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	if (mode == "+k") {
		channel->setPassword(password);
		channel->setModes("+k");
		channel->SethasPassword(true);
	}
	else if (mode == "-k") {
		if (channel->gethasPassword() == false || password != channel->getPassword())
			return;
		channel->setModes("-k");
		channel->SethasPassword(false);
	}
	const std::vector<int>& clients = channel->getClients();
	for (size_t i = 0; i < clients.size(); ++i) {
		sendToClient(clients[i], ":" + getfullname(client_socket) + " MODE " + channel_name + " " + mode + "\r\n");
	}
}

void	Server::ChannelInvite(int client_socket, const std::string& channel_name, const std::string& mode) {
	if (checkChannel(client_socket, channel_name) == false)
		return;
	Channel* channel = getChannelByName(channel_name);
	if (channel->isOperator(getNickBySocket(client_socket)) == false) {
		sendToClient(client_socket, ":irc.example.net 482 " + getfullname(client_socket) + " " + channel_name + " :You're not channel operator\r\n");
		return;
	}
	if (mode == "+i") {
		if (channel->getInviteOnlyState() == true)
			return;
		channel->setModes("+i");
		channel->SetInviteOnly(true);
	}
	else if (mode == "-i") {
		if (channel->getInviteOnlyState() == false)
			return;
		channel->setModes("-i");
		channel->SetInviteOnly(false);
	}
	const std::vector<int>& clients = channel->getClients();
	for (size_t i = 0; i < clients.size(); ++i) {
		sendToClient(clients[i], ":" + getfullname(client_socket) + " MODE " + channel_name + " " + mode + "\r\n");
	}
}
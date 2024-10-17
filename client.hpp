#ifndef CLIENT_HPP
# define CLIENT_HPP

#include "server.hpp"

class Server;

class Client {
private:
	std::string	_pwd;
	std::string	_realName;
	std::string	_nickName;
	std::string	_hostName;
	std::string	_userName;
	std::string	_buffer;
	std::string channeljoined;
	std::string	_line;

	int socket;

	std::vector<std::string> _cmds;

	bool registered;
	bool on;

public:
	Client(int serverSocket);
	~Client();

	//setters
	void	setNick(const std::string& nick);
	void	setUser(const std::string &user);
	void	setIp(const std::string &host);
	void	setReal(std::string first, std::string last);
	void	setChannelJoined(std::string channel_name);
	void	setRegistered(bool reg);
	void	setOn(bool mode);

	//getters
	std::string	getChannelJoined() const;
	std::string getNick() const;
	std::string getUser() const;
	std::string getIp() const;
	std::string getReal() const;
	int 		getSocket() const;

	//is-something
	bool isRegistered() const;
	bool isOn() const;
};

#endif
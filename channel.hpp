
#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include "server.hpp"
#include "client.hpp"
#include "channel.hpp"
#include <unordered_map>

class Channel
{
private:
	std::string channel_name;

	std::vector<int> 			clients;
	std::vector<std::string> 	invitedClients;
	std::vector<std::string>	operators;

	std::unordered_map<char, bool>	modes;

	bool	onInviteOnly;
	bool	topicToOp;
	bool	hasPassword;
	int		maxUsers;

	std::string password;
	std::string	topic;

public:
	Channel(const std::string  &channel_name);
	~Channel();

	//add
	void	addClient(int client_fd);
	void	addOperator(const std::string& nickname);
	void	addInvitedClient(const std::string& nickname);
	void	addMode(char mode);

	//remove
	void	removeClient(int client_fd);
	void	removeOperator(const std::string& nickname);
	void	removeInvitedClient(const std::string& nickname);
	void	removeMode(char mode);

	//is-something
	bool 	isOperator(const std::string& nickname) const;
	bool	isClientInChannel(int client_socket) const;
	bool	isInvited(const std::string& nickname) const;


	//getters
	int								returnNumberOfUsers();
	const std::string& 				getChannelName() const;
	const std::vector<int>& 		getClients() const;
	const std::vector<std::string>& getOperators() const;
	const std::string				&getTopic(void) const;
	std::string 					getModes() const;
	std::string 					getPassword() const;
	int 							getMaxUsers() const;
	bool							getTopicToOp() const;
	bool							getInviteOnlyState() const;
	bool							gethasPassword() const;

	//setters
	void	setTopic(const std::string &topic);
	void	setModes(const std::string& newmode);
	void 	setMaxUsers(int maxusers);
	void	setTopicToOp(bool mode);
	void 	SetInviteOnly(bool mode);
	void 	SethasPassword(bool mode);
	void	setPassword(std::string new_password);
};

#endif
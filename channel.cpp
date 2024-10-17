
#include "channel.hpp"

Channel::Channel(const std::string &channel_name) : channel_name(channel_name) {
	this->onInviteOnly = false;
	this->maxUsers = 0;
	this->topicToOp = false;
	this->hasPassword = false;
	this->password = "";
}

Channel::~Channel() {}

void Channel::addClient(int client_fd) {
	clients.push_back(client_fd);
}

void Channel::removeClient(int client_fd) {
	clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
}

const std::string& Channel::getChannelName() const {
	return (this->channel_name);
}

const std::vector<int>& Channel::getClients() const {
	return (this->clients);
}

const std::vector<std::string>& Channel::getOperators() const {
	return (this->operators);
}

std::string Channel::getModes() const {
	std::string modeString;
	for (std::unordered_map<char, bool>::const_iterator it = modes.begin(); it != modes.end(); ++it) {
		if (it->second)
			modeString += it->first;
	}
	return (modeString);
}

void Channel::setModes(const std::string& newmode) {
	bool adding = true;
	for (size_t i = 0; i < newmode.size(); ++i)
	{
		if (newmode[i] == '+')
			adding = true;
		else if (newmode[i] == '-')
			adding = false;
		else
			modes[newmode[i]] = adding;
	}
}

void Channel::addMode(char mode) {
	modes[mode] = true;
}

void Channel::removeMode(char mode) {
	modes[mode] = false;
}

bool Channel::isOperator(const std::string& nickname) const {
	for (size_t i = 0; i < operators.size(); ++i)
	{
		if (operators[i] == nickname)
			return true;
	}
	return false;
}

void Channel::addOperator(const std::string& nickname) {
	if (!isOperator(nickname))
		operators.push_back(nickname);
}

void Channel::removeOperator(const std::string& nickname) {
	std::vector<std::string>::iterator it = operators.begin();
	while (it != operators.end()) {
		if (*it == nickname) {
			operators.erase(it);
			return;
		}
		++it;
	}
}

bool Channel::isClientInChannel(int client_socket) const {
	for (size_t i = 0; i < clients.size(); ++i) {
		if (clients[i] == client_socket)
			return (true);
	}
	return (false);
}

int Channel::getMaxUsers() const {
	return (this->maxUsers);
}

void Channel::setMaxUsers(int maxusers) {
	this->maxUsers = maxusers;
}

int Channel::returnNumberOfUsers() {
	return (this->clients.size());
}

const std::string	&Channel::getTopic(void) const {
	return this->topic;
}

void	Channel::setTopic(const std::string &topic) {
	this->topic = topic;
}

void	Channel::setTopicToOp(bool mode) {
	this->topicToOp = mode;
}

bool Channel::getTopicToOp() const {
	return (this->topicToOp);
}

void Channel::SetInviteOnly(bool mode) {
	this->onInviteOnly = mode;
}

bool Channel::getInviteOnlyState() const {
	return (this->onInviteOnly);
}

void Channel::setPassword(std::string new_password) {
	this->password = new_password;
}

std::string Channel::getPassword() const {
	return (this->password);
}

void Channel::SethasPassword(bool mode) {
	this->hasPassword = mode;
}

bool Channel::gethasPassword() const {
	return (this->hasPassword);
}

bool Channel::isInvited(const std::string& nickname) const {
	for (size_t i = 0; i < invitedClients.size(); ++i)
	{
		if (invitedClients[i] == nickname)
			return true;
	}
	return false;
}

void Channel::addInvitedClient(const std::string& nickname) {
	if (!isInvited(nickname))
		invitedClients.push_back(nickname);
}

void Channel::removeInvitedClient(const std::string& nickname) {
	std::vector<std::string>::iterator it = invitedClients.begin();
	while (it != invitedClients.end()) {
		if (*it == nickname) {
			invitedClients.erase(it);
			return;
		}
		++it;
	}
}
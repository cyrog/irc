
#include "client.hpp"

Client::Client(int socket) : registered(false), socket(socket), on(false) {}

Client::~Client() {
	close(this->socket);
}

std::string Client::getNick() const {
	return (this->_nickName);
}

void Client::setNick(const std::string& nick) {
	this->_nickName = nick;
}

void Client::setIp(const std::string &host) {
	this->_hostName = host;
}

bool Client::isRegistered() const {
	return (this->registered);
}

bool Client::isOn() const {
	return (this->on);
}

void Client::setUser(const std::string &user) {
	this->_userName = user;
}

void Client::setReal(std::string first, std::string last) {
    if (!first.empty() && first[0] == ':')
        first.erase(0, 1);
    this->_realName = first + " " + last;
}

void Client::setRegistered(bool reg) {
	this->registered = reg;
}

void Client::setOn(bool mode) {
	this->on = mode;
}

int Client::getSocket() const {
	return (this->socket);
}

std::string Client::getUser() const {
	return (this->_userName);
}

std::string Client::getIp() const {
	return (this->_hostName);
}

std::string Client::getReal() const {
	return (this->_realName);
}

void Client::setChannelJoined(std::string channel_name) {
	this->channeljoined = channel_name;
}

std::string Client::getChannelJoined() const {
	return (this->channeljoined);
}
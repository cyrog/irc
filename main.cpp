
#include "server.hpp"
#include "client.hpp"

int CheckArgs(char **argv)
{
	//check si l'argument correspond a un port valide
	int port = std::atoi(argv[1]);
	if (port < 0 || port > 65535)
	{
		std::cout << "ERROR: Invalid port number." << std::endl;
		return (1);
	}
	return (0);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "ERROR: Wrong number of arguments." << std::endl;
		return (1);
	}
	if (CheckArgs(argv) == 1)
		return (1);
	int porttmp = std::atoi(argv[1]);
	std::string passtmp = argv[2];
	Server server(porttmp, passtmp);
	server.start();
	return (0);
}


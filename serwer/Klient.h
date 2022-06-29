#include <iostream>
#include <vector>
#include <WinSock2.h>
#pragma once
class Klient
{
private:
	std::string login;
	std::string password;
	std::string chat_name;
	pollfd pollfd_data;

public:
	std::vector <char> buf;
	std::vector <char> login_answer;
	std::vector <char> name_answer;
	std::vector <char> send_answer;
	std::vector <char> list_chat;
	std::vector <char> client_new;
	std::vector <char> client_old;
	Klient() {};
	Klient(pollfd pollfd_data)
	{
		this->pollfd_data = pollfd_data;
	}
	
	//setters
	void setLogin(std::string login)
	{
		this->login = login;
	}
	void setPassword(std::string password)
	{
		this->password = password;
	}
	void setChat_name(std::string chat_name)
	{
		this->chat_name = chat_name;
	}
	void setPollfd_data(pollfd pollfd_data)
	{
		this->pollfd_data = pollfd_data;
	}
	//getters
	std::string getLogin()
	{
		return login;
	}
	std::string getPassword()
	{
		return password;
	}
	std::string getChat_name()
	{
		return chat_name;
	}
	pollfd getPollfd_data()
	{
		return pollfd_data;
	}
};


#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "Klient.h"
#pragma comment(lib, "ws2_32.lib")

int main()
{
    WSADATA wsaData;                                    //Struktura zawieraj¹ca dane Windows socket
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);  //Wersja winSock 2.2
    if (result != NO_ERROR)                             //Sprawdzanie, czy socket zosta³ utworzony
    {
        std::cout << "WSA_Startup failed: " << result << std::endl;
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);   //utworzenie gniazda serwera
    int cli_sock;
    struct sockaddr_in serv_addr, cli_addr;
    serv_addr.sin_port = htons(2000);
    serv_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, "127.0.0.1", &(serv_addr.sin_addr)) <= 0)    //konwersja na postaæ sieciow¹
    {
        std::cout << "Wrong address" << std::endl;
        return 1;
    }
    socklen_t serv_len = sizeof(serv_addr);
    socklen_t cli_len = sizeof(cli_addr);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)  //bindownanie portu
    {
        std::cout << "Socket assigning error" << std::endl;
        return 1;
    }
    else
    {
        if (listen(sockfd, 100) < 0)     //oczekiwanie na klientów (max 100)
        {
            std::cout << "Listen error" << std::endl;
            closesocket(sockfd);
            WSACleanup();
            return 1;
        }
        pollfd pollfd_data;
        pollfd_data.fd = sockfd;                        //przypisanie socketu do struktury
        pollfd_data.events = POLLRDNORM | POLLWRNORM;   //odbieranie i wysy³anie danych

        std::vector<pollfd> klienci;
        std::map<int, Klient*> dane_klient;
        klienci.push_back(pollfd_data);

        char buf[1024];
        memset(buf, 0, sizeof(buf));
        std::string received_client_data_string;

        while (1)   //pêtla g³ówna serwera
        {
            if (WSAPoll(&klienci[0], klienci.size(), 0) > 0)    //sprawdza, czy klienci maj¹ jakieœ dane
            {
                for (auto it = klienci.begin(); it != klienci.end(); it++)  //kto ma dane
                {
                    if (it->revents & (POLLRDNORM | POLLHUP))    //je¿eli mo¿na odebraæ dane od klienta
                    {
                        if (it == klienci.begin())  //je¿eli klient ³¹czy siê po raz pierwszy
                        {
                            cli_sock = accept(sockfd, (sockaddr*)&cli_addr, &cli_len);  //akceptowanie po³¹czenia z klientem
                            if (cli_sock <= 0)
                            {
                                std::cout << "accept error";
                            }
                            pollfd_data.fd = cli_sock;
                            pollfd_data.events = POLLRDNORM | POLLWRNORM;
                            klienci.push_back(pollfd_data);     //zapisanie parametrów o sockecie klienta w kliencie
                            it = klienci.begin();
                            Klient* kl1 = new Klient(pollfd_data);
                            dane_klient.insert(std::make_pair(cli_sock, kl1));    
                        }
                        else
                        {   
                            if (dane_klient.find(it->fd) != dane_klient.end())  //je¿eli klient jest w mapie
                            {
                                int data_size = recv(it->fd, (char*)buf, sizeof(buf), 0);
                                auto x = dane_klient.find(it->fd);
                                if (data_size >= 0)
                                {
                                    x->second->buf.insert(x->second->buf.end(), buf, (buf + data_size));
                                }
                                else if (data_size <= 0)
                                {
                                    closesocket(cli_sock);
                                    delete x->second;
                                    dane_klient.erase(x);
                                    continue;
                                }                            

                                received_client_data_string.clear();
                                received_client_data_string.insert(received_client_data_string.end(), x->second->buf.begin(), x->second->buf.end());
                                x->second->buf.clear();
                                if (received_client_data_string.find('\n') != std::string::npos)
                                {
                                    //Nag³ówek LOGIN
                                    if (received_client_data_string.find("LOGIN") != std::string::npos)
                                    {
                                        std::cout << "Naglowek to LOGIN" << std::endl;
                                        std::cout << received_client_data_string;

                                        //wy³uskanie danych z nag³ówka
                                        received_client_data_string.erase(0, 6);            //usuniêcie LOGIN:
                                        int where = received_client_data_string.find(':');  //znalezienie gdzie jest podzial login:password
                                        int str_len = received_client_data_string.length();

                                        //wyodrêbnienie login oraz password
                                        std::string login(std::string(received_client_data_string, 0, where));
                                        std::string password(std::string(received_client_data_string, where + 1, str_len));
                                        password.pop_back();

                                        //ustalenie odpowiedzi na podstawie login i password
                                        std::string ans = "LOGIN:WRONG\n";
                                        if ((login == "user1" && password == "12345") || (login == "user2" && password == "54321") || (login == "user3" && password == "12345") || (login == "user4" && password == "54321"))
                                        {
                                            ans = "LOGIN:OK\n";
                                        }
                                        for (auto i = dane_klient.begin(); i != dane_klient.end(); i++) //sprawdzenie, czy ktoœ ju¿ nie jest zalogowany z tego konta
                                        {
                                            if (login == i->second->getLogin())
                                            {
                                                ans = "LOGIN:WRONG\n";
                                                break;
                                            }
                                        }
                                        x->second->setLogin(login);                                //ustawienie login
                                        x->second->setPassword(password);                          //ustawnienie password
                                        x->second->login_answer.clear();
                                        x->second->login_answer.insert(x->second->login_answer.end(), ans.begin(), ans.end());    //zapisanie odpowiedzi w dane_klient
                                        //delete(received_client_data_string);
                                    }

                                    //Nag³ówek IMIE, do ustawienia chat_name
                                    else if (received_client_data_string.find("NAME") != std::string::npos)
                                    {
                                        std::cout << "Naglowek to NAME!!!" << std::endl;
                                        std::cout << received_client_data_string;

                                        //wy³uskanie danych z nag³ówka
                                        received_client_data_string.erase(0, 5);   //usuniêcie NAME:
                                        received_client_data_string.pop_back();    //usuniêcie \n

                                        bool cierpienie = true;
                                        for (auto i = dane_klient.begin(); i != dane_klient.end(); i++) //je¿eli ktoœ ju¿ ma takie imie to nie
                                        {
                                            if (i->second->getChat_name() == received_client_data_string)
                                            {
                                                //wyœlij do klienta, ¿e nie
                                                cierpienie = false;
                                                break;
                                            }
                                        }
                                        if (cierpienie == true) //je¿eli nikt nie ma takiego imienia, to ustaw to imie oraz wyœlij, ¿e taka osoba siê zalogowa³a
                                        {
                                            std::string ans = "NAME:OK\n";
                                            x->second->name_answer.clear();
                                            x->second->name_answer.insert(x->second->name_answer.end(), ans.begin(), ans.end());
                                            x->second->setChat_name(received_client_data_string);

                                            std::string _name = x->second->getChat_name();
                                            _name += " has logged in\n";
                                            for (auto i = dane_klient.begin(); i != dane_klient.end(); i++)
                                            {
                                                if ((i->second->getChat_name() != x->second->getChat_name()) && (!i->second->getChat_name().empty()))
                                                {
                                                    i->second->client_new.insert(i->second->client_new.end(), _name.begin(), _name.end());
                                                } 
                                            }
                                        }
                                        else if (cierpienie == false)   //je¿eli imie jest niepoprawne to wyœlij nie do klienta
                                        {
                                            std::string ans = "NAME:WRONG\n";
                                            x->second->name_answer.clear();
                                            x->second->name_answer.insert(x->second->name_answer.end(), ans.begin(), ans.end());
                                        }
                                    }
                                    //Nag³ówek SEND
                                    else if (received_client_data_string.find("SEND") != std::string::npos)
                                    {
                                        //wy³uskanie do kogo przes³aæ wiadomoœæ
                                        std::cout << received_client_data_string << std::endl;
                                        received_client_data_string.erase(0, 5);
                                        int where = received_client_data_string.find(':');
                                        std::string name, sender;   //name - do kogo, sender - od kogo + wiadomoœæ
                                        name.clear();
                                        sender.clear();
                                        if (where > 0)  //je¿eli jest : czyli wys³anie do jakiejœ osoby
                                        {
                                            std::string name(std::string(received_client_data_string, 0, where));
                                            std::string sender(std::string(received_client_data_string, where, received_client_data_string.length()));
                                            sender = x->second->getChat_name() + sender;

                                            bool istnienia = false;
                                            //znalezienie w dane_klient do kogo wpisac wiadomosc do odebrania
                                            for (auto y = dane_klient.begin(); y != dane_klient.end(); y++)
                                            {
                                                if (y->second->getChat_name() == name)
                                                {
                                                    istnienia = false;
                                                    y->second->send_answer.insert(y->second->send_answer.end(), sender.begin(), sender.end());
                                                    break;
                                                }
                                                else
                                                {
                                                    istnienia = true;
                                                }
                                            }
                                            if (istnienia == true)  //je¿eli osoba nie istnieje, to wpisz do zmiennej send_answer odpowiedz do osoby ktora wysylala wiadomosc
                                            {
                                                std::string error = "message send error\n";
                                                x->second->send_answer.insert(x->second->send_answer.end(), error.begin(), error.end());
                                            }
                                            sender.clear();
                                            name.clear();
                                        }
                                        if (received_client_data_string == "LIST\n")    //je¿eli przes³ane zostanie LIST
                                        {
                                            std::vector <char> chat_names;
                                            chat_names.clear();
                                            std::string _name;              //zmienna pomocnicza, gdy¿ nie mo¿na operowaæ na danych prywatnych
                                            
                                            for (auto i = dane_klient.begin(); i != dane_klient.end(); i++) //wpisz do zmiennej chat_names liste osób osoba1;osoba2;...
                                            {
                                                if (!i->second->getChat_name().empty())
                                                {
                                                    _name.clear();
                                                    _name = i->second->getChat_name();
                                                    //chat_names.assign(i->second->getChat_name().begin(), i->second->getChat_name().end());
                                                    chat_names.insert(chat_names.end(), _name.begin(), _name.end());
                                                    chat_names.push_back(';');
                                                }
                                            }
                                            chat_names.pop_back();
                                            chat_names.push_back('\n');
                                            x->second->list_chat = chat_names;  //wpisz do zmiennej osoby wysy³aj¹cej liste do wys³ania
                                        }
                                        else if (received_client_data_string == "LOGOUT\n") //je¿eli nag³ówek to logout
                                        {
                                            std::string _name = x->second->getChat_name();  //wpisz imie wylogowanego do zmiennej (nie da siê operowaæ na zmiennych prywatnych)
                                            _name += " has log out\n";
                                            for (auto i = dane_klient.begin(); i != dane_klient.end(); i++) //wpisanie do wszystkich zalogowanych, ¿e siê wylogowa³
                                            {
                                                i->second->client_old.insert(i->second->client_old.end(), _name.begin(), _name.end());
                                            }
                                            closesocket(it->fd);    //zamkniêcie socketu klienta
                                            delete x->second;       //usuniêcie klienta
                                            dane_klient.erase(x);   //usuniêcie klienta z mapy
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (it->revents & (POLLWRNORM))   //je¿eli mo¿na wys³aæ dane do klienta
                    {
                        if (dane_klient.find(it->fd) != dane_klient.end())  //znalezienie klienta z danymi
                        {
                            auto x = dane_klient.find(it->fd);     //zapisanie dane_klienta do x
                            if (!x->second->login_answer.empty())  //sprawdzenie, czy dane_klient ma dane do wys³ania do programu klienta
                            {
                                int length;
                                length = send(it->fd, (char*)&x->second->login_answer[0], x->second->login_answer.size(), 0); //wys³anie danych do klienta
                                if (length <= 0)
                                {
                                    closesocket(it->fd);    //zamkniêcie socketu klienta
                                }
                                if (length > 0)
                                {
                                    x->second->login_answer.erase(x->second->login_answer.begin(), x->second->login_answer.begin() + length);    //je¿eli s¹ jeszcze jakieœ dane do wys³ania, to usuniêcie tylu danych ile zosta³o ju¿ wys³anych
                                }
                            }
                            else if (!x->second->name_answer.empty())
                            {
                                int length;
                                length = send(it->fd, (char*)&x->second->name_answer[0], x->second->name_answer.size(), 0); //wys³anie danych do klienta
                                if (length <= 0)
                                {
                                    closesocket(it->fd);    //zamkniêcie socketu klienta
                                }
                                if (length > 0)
                                {
                                    x->second->name_answer.erase(x->second->name_answer.begin(), x->second->name_answer.begin() + length);    //je¿eli s¹ jeszcze jakieœ dane do wys³ania, to usuniêcie tylu danych ile zosta³o ju¿ wys³anych
                                }
                            }
                            else if (!x->second->list_chat.empty()) //wys³anie listy aktywnych klientów
                            {
                                std::cout << "LIST the chat" << std::endl;
                                
                                int length;
                                length = send(it->fd, (char*)&x->second->list_chat[0], x->second->list_chat.size(), 0); //wys³anie danych do klienta
                                if (length <= 0)
                                {
                                    closesocket(it->fd);    //zamkniêcie socketu klienta
                                }
                                if (length > 0)
                                {
                                    x->second->list_chat.erase(x->second->list_chat.begin(), x->second->list_chat.begin() + length);
                                }
                            }
                            else if (!x->second->send_answer.empty())   //przesy³anie wiadomoœci od jednego klienta do drugiego
                            {
                               
                                int length;
                                length = send(it->fd, (char*)&x->second->send_answer[0], x->second->send_answer.size(), 0); //wys³anie danych do klienta
                                if (length <= 0)
                                {
                                    closesocket(it->fd);    //zamkniêcie socketu klienta
                                }
                                if (length > 0)
                                {
                                    x->second->send_answer.erase(x->second->send_answer.begin(), x->second->send_answer.begin() + length);    //je¿eli s¹ jeszcze jakieœ dane do wys³ania, to usuniêcie tylu danych ile zosta³o ju¿ wys³anych
                                }
                            }
                            else if (!x->second->client_new.empty())    //informacja, ¿e nowy klient siê zalogowa³
                            {
                                std::cout << "New user";
                                int length;
                                length = send(it->fd, (char*)&x->second->client_new[0], x->second->client_new.size(), 0); //wys³anie danych do klienta
                                if (length <= 0)
                                {
                                    closesocket(it->fd);    //zamkniêcie socketu klienta
                                }
                                if (length > 0)
                                {
                                    x->second->client_new.erase(x->second->client_new.begin(), x->second->client_new.begin() + length);    //je¿eli s¹ jeszcze jakieœ dane do wys³ania, to usuniêcie tylu danych ile zosta³o ju¿ wys³anych
                                }
                            }
                            else if (!x->second->client_old.empty())    //informacja, ¿e jakiœ klient siê wylogowa³
                            {
                                std::cout << "logout";
                                int length;
                                length = send(it->fd, (char*)&x->second->client_old[0], x->second->client_old.size(), 0); //wys³anie danych do klienta
                                if (length <= 0)
                                {
                                    closesocket(it->fd);    //zamkniêcie socketu klienta
                                }
                                if (length > 0)
                                {
                                    x->second->client_old.erase(x->second->client_old.begin(), x->second->client_old.begin() + length);    //je¿eli s¹ jeszcze jakieœ dane do wys³ania, to usuniêcie tylu danych ile zosta³o ju¿ wys³anych
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    closesocket(sockfd);
    WSACleanup();
}
#include <chrono>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <conio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")

int data_send(int sockfd, char *lp_send);               //wysy³anie danych do serwera oraz zwrócenie odpowiedzi 1-poprawne dane, 0-niepoprawne dane
void logout(int sockfd);                                //funkcja do wylogowywania siê
void clear_line(int length);                            //funkcja do czyszczenia linii w konsoli

int main()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  //zmienna do obs³ugi kolorów w konsoli
    WSADATA wsaData;                                    //Struktura zawieraj¹ca dane Windows socket
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);  //Wersja winSock 2.2
    if (result != NO_ERROR)                             //Sprawdzanie, czy socket zosta³ utworzony
    {
        std::cout << "WSA_Startup failed: " << result << std::endl;
        return 1;
    }

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);       // tworzenie socketu IPv4 typu sock_stream
    struct addrinfo hints, * res;                       //… inicjalizacja struktury hints
    memset(&hints, 0, sizeof hints);                    // zapisanie struktury
    hints.ai_family = AF_INET;                          //IPv4
    hints.ai_socktype = SOCK_STREAM;                    //TCP

    getaddrinfo("127.0.0.1", "2000", &hints, &res);     //wpisanie adresu ip oraz portu serwera

    int nr_error = connect(sockfd, res->ai_addr, res->ai_addrlen);  //³¹czenie oraz obs³uga b³êdów
    if (nr_error != 0)
    {
        std::cout << "Connection error: " << nr_error;	            //W przypadku braku po³¹czenia wyjœcie z programu z b³êdem 2
        return 2;
    }
    if (nr_error == 0)
    {
        std::cout << "Connected to the server" << std::endl;
    }
    /*******************************************************/

    std::string login, password, chat_name; //zmienne do trzymania loginu, has³a, imienia na czacie dla innych uczestników
    char* lp_send;                          //zmienna do wys³ania powy¿szych danych wraz z nag³ówkiem do serwera
    int answer = 0;                         //imformacja zwrotna od serwera, je¿eli dane s¹ poprawne to kontynuuj, je¿eli nie to wprowadŸ jeszcze raz

    /*Pêtla do wprowadzenia loginu oraz has³a, przekazanie tych informacji do serwera 
    oraz otrzymanie informacji zwrotnej, czy dane s¹ poprawne*/
    do
    {
        std::cout << "Login: ";
        std::getline(std::cin, login);
        std::cout << "Password: ";
        std::getline(std::cin, password);

        //stworzenie danych do wys³ania do serwera (nag³ówek serwera do loginu oraz hasla to LOGIN)
        int size = sizeof("LOGIN:") + login.length() + sizeof(':') + password.length() + sizeof("\n\0") + 1;
        lp_send = new char[size];   
        strcpy_s(lp_send, size, ("LOGIN:" + login + ':' + password + "\n\0").c_str());

        //przes³anie danych do serwera
        answer = data_send(sockfd, lp_send);

        //Je¿eli dane do logowania nie s¹ poprawne, zapytaj o nie jeszcze raz
        if (answer == 0)
        {
            std::cout << "incorrect login or password";
            Sleep(1000);
            system("CLS");
        }
        //Je¿eli dane do logowania s¹ poprawne to przejdŸ do dalszej czêœci programu
        else if (answer == 1)
        {
            system("CLS");
            //break;
        }

    } while (answer != 1);
    //Pêtla do wprowadzenia chat_name, je¿eli imie ju¿ jest zajête, to wprowadŸ jeszcze raz      
    do
    {
        std::cout << "Chat name: ";
        std::getline(std::cin, chat_name);

        int size = sizeof("NAME:") + sizeof(chat_name) + sizeof("\n") + 1;
        lp_send = new char[size];
        strcpy_s(lp_send, size, ("NAME:" + chat_name + "\n").c_str());

        if ((chat_name.find(':') != std::string::npos) || (chat_name.find(';') != std::string::npos))   //imie nie mo¿e posiadaæ znaków :;
        {
            answer = 0;
            std::cout << "Do not use \":\", \";\" symbols";
            Sleep(1000);
            system("CLS");
        }
        else
        {
            answer = data_send(sockfd, lp_send);        //przesy³anie imienia do serwera oraz otrzymanie odpowiedzi
            if (answer == 0)                            //Je¿eli imie jest ju¿ zajête lub niepoprawe, zapytaj jeszcze raz
            {
                std::cout << "Chat name taken or incorrect";
                Sleep(1000);
                system("CLS");
            }
            else if (answer == 1)                       //Je¿eli imie mo¿e zostaæ wpisane to przejdŸ do dalszej czêœci programu
            {
                system("CLS");
            }
        }
    } while (answer != 1);
   
    //usuniêcie przycisku x, ¿eby ka¿dy siê wylogowywa³ (nie jest to idealna metoda, gdy¿ program mo¿na zamkn¹æ przez np menager zadañ)
    //lepsz¹ metod¹ by³oby obs³ugiwanie CTRL_CLOSE_EVENT, jednak z racji braku czasu (sesja) nie uda³o siê tego zrobiæ.
    HWND hwnd = GetConsoleWindow();
    HMENU hmenu = GetSystemMenu(hwnd, FALSE);
    EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);

    std::cout << "message structure chat_name:message          np. Adrian:Co slychac?" << std::endl;
    std::cout << "to see log in users type LIST" << std::endl;
    std::cout << "to log out type LOGOUT" << std::endl;
    
    std::string buf;        //bufor do osbs³ugi klawiatury (std::cout, scanf, _getline itp. blokowa³y terminal, nawet przy u¿yciu w¹tków)
    std::string chat_input; //zmienna, do której buf jest zapisywaniy, je¿eli przycisk enter zostanie naciœniêty
    chat_input.clear();     //wyczyszczenie pamiêci
    buf.clear();            //wyczyszczenie pamiêci

    //Pêtla chatu, odpowiada za wprowadzanie wiadomoœci, wysy³anie ich oraz odbieranie nowych wiadomoœci i komunikatów
    while (1)
    {
        SetConsoleTextAttribute(hConsole, 2);   //kolor tekstu w konsoli (zielony)
      
        if (_kbhit())                           //czytanie klawiatury, nieblokuj¹co. _kbhit() zwraca TRUE, je¿eli klawisz jest wciœniêty
        {                                       //je¿eli klawisz jest wciœniêty zapisywaniy jest do bufora. ENTER zatwierdza bufor i wpisuje go do chat_input
            char znak;                          //je¿eli backspace jest wciœniêty, usuwa wtedy znak z bufora, oraz ekranu. 
            znak = _getch();
            if (znak == 8)  //backspace
            {
                if (!buf.empty())               //zabezpieczenie, ¿eby do bufora nie zosta³o wpisane \b
                {
                    buf.pop_back();
                    std::cout << "\b \b";
                }
            }
            else if (znak == 13)                //enter, potwierdzenie bufora
            {
                if (!buf.empty())
                {
                    chat_input = buf;           //wpisanie wiadomoœci do chat_input
                    clear_line(buf.length());   //wyczyszczenie linii
                    int where = buf.find(':');
                    if (where > 0 || buf == "LIST" || buf == "LOGOUT")  //sprawdzenie, czy polecenie to LIST lub LOGOUT, pomimo braku :, czyli odbiorcy
                    {
                        if (buf == "LOGOUT")
                        {
                            logout(sockfd); //wylogowanie (na serwerze usuniêcie klienta), tutaj zamkniêcie socketu, oczyszczenie WSA
                            std::exit(0);   //wyjœcie z programu
                        }
                        std::cout << chat_name << "->" << buf;  //strza³ka, która informuje od kogo do kogo jest wiadomoœæ, wypisanie wys³anej wiadomoœci (sprawdzenie, czy wiadomoœæ zosta³a wys³ana jest póŸniej)
                        std::cout << '\n';
                    }
                    buf.clear();
                }
            }
            else    //wszystkie inne znaki, wpisanie do buf
            {
                buf.push_back(znak);
                std::cout << znak;
            }
        }

        //wysy³anie poleceñ do serwera
        std::vector <char> vec_chat;
        if (!chat_input.empty())    //wysy³aj dane, je¿eli s¹
        {
            chat_input = "SEND:" + chat_input + "\n\0";
            vec_chat.insert(vec_chat.end(), chat_input.begin(), chat_input.end());
            int length = 0;
            do
            {
                length = send(sockfd, reinterpret_cast<char*>(vec_chat.data()), vec_chat.size(), 0);
            } while (length <= 0);
            chat_input.erase(chat_input.begin(), chat_input.end());
        }

        int n = 0;
        char answer[500];
        std::string str;

        //sprawdzanie, czy s¹ jakieœ dane do odebrania z serwera oraz odbiór danych
        pollfd pollfd_data;
        pollfd_data.fd = sockfd;
        pollfd_data.events = POLLIN;

        if (WSAPoll(&pollfd_data, 1, 0) > 0)    //u¿ycie WSAPoll, ¿eby odbieranie wiadomoœci odbywa³o siê ca³y czas, nieblokuj¹co
        {
            if (pollfd_data.revents & POLLIN)   //je¿eli s¹ dane do odebrania
            {
                do
                {
                    n = recv(sockfd, answer, strlen(answer), 0);

                    if (n > 0)
                    {
                        str.append(answer, n);
                        if (str.find('\n') != std::string::npos)    //wpisywanie danych a¿ do znaku nowej linii
                        {
                            break;
                        }
                    }
                } while (n != 0);
                if (str.find(';') != std::string::npos) //szukanie ; w LIST, osoby s¹ tak oddzielane, zast¹pienie znakiem nowej linii do wypisania.
                {
                    for (int i = 0; i < str.length(); i++)
                    {
                        if (str[i] == ';')
                        {
                            str[i] = '\n';
                        }
                    }
                }
                else if (str == "message send error\n") //je¿eli serwer zwróci error, to usuñ liniê wys³anej wiadomoœci, wypisz komunikat na sekundê, wyczyœæ liniê
                {
                    str.clear();
                    std::cout << "\x1b[A";
                    clear_line(buf.length() * 2);
                    SetConsoleTextAttribute(hConsole, 4);   //kolor tekstu w konsoli (czerwony)
                    std::cout << "message send error, name not found";
                    Sleep(1000);
                    clear_line(sizeof("message send error, name not found"));
                }

                clear_line(buf.length());
                SetConsoleTextAttribute(hConsole, 7);   //kolor tekstu w konsoli (bia³y)
                std::cout << str;                       //wypisanie wiadomoœci, któr¹ klient otrzyma³
                SetConsoleTextAttribute(hConsole, 2);   //kolor tekstu w konsoli (zielony)
                std::cout << buf;                       //wypisanie tego, co pisaliœmy w tym kliencie
            }  
        }
    }
    
    closesocket(sockfd);    //zamkniêcie socketu
    WSACleanup();           //zamkniêcie winsock 2
}

//Funkcje
int data_send(int sockfd, char *lp_send)
{
    long n = 0;	                        //zmienna do sumowania iloœci wys³anych bitów (co iteracje)
    long total = 0;			            //zmienna do sumowania wys³anych bitów (³¹cznie)
    long bytesleft = strlen(lp_send);   //zmienna, z której co iteracje odejmowane s¹ wartoœci, u¿ywanie przy send, aby wys³a³ okreœlon¹ iloœæ bitów.
    while (total < strlen(lp_send))     //Przesy³anie, dopóki ca³kowita iloœæ bajtów nie bêdzie równa d³ugoœci pliku
    {
        n = send(sockfd, lp_send + total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;	    //sumowanie iloœci wys³anych bitów
        bytesleft -= n;	//odejmowanie bitów wys³anych od ca³oœciowej liczby pocz¹tkowej.
    }

    //odebranie od serwera odpowiedzi (1 - zgadza siê, 0 - nie zgadza siê)
    n = 0;
    char answer[20];
    std::string str;
    do
    {
        n = recv(sockfd, answer, strlen(answer), 0);
        if (n > 0)
        {
            str.append(answer, n);
            if (str.find('\n') != std::string::npos)
            {
                break;
            }
        }
    } while (n != 0);
    int wynik = 0;
    if ((str.find("LOGIN:OK\n") != std::string::npos) || (str.find("NAME:OK\n") != std::string::npos) || (str.find("SEND:OK\n") != std::string::npos))
    {
        wynik = 1;
    }
    if ((str.find("LOGIN:WRONG\n") != std::string::npos) || (str.find("NAME:WRONG\n") != std::string::npos) || (str.find("SEND:WRONG\n") != std::string::npos))
    {
        wynik = 0;
    }
    return wynik;
}
void clear_line(int length) 
{
    for (int i = 0; i < length; i++)        //czyszczenie linii: 
    {                                       //cofniêcie kursora w konsoli tyle razy ile jest znaków w buf
        for (int j = 0; j < 3; j++)         //wypisanie " " tyle razy ile jest znaków w buf
        {                                   //cofniêcie kursora w konsoli tyle razy ile jest znaków w buf
            if (j == 0 || j == 2)
            {
                std::cout << "\b";
            }
            if (j == 1)
            {
                std::cout << " ";
            }
        }
    }
}
void logout(int sockfd)
{
    int length;
    char string[] = "SEND:LOGOUT\n";
    do
    {
        length = send(sockfd, string, strlen(string), 0);
    } while (length <= 0);
    closesocket(sockfd);
    WSACleanup();
}

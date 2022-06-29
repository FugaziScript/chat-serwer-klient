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

int data_send(int sockfd, char *lp_send);               //wysy�anie danych do serwera oraz zwr�cenie odpowiedzi 1-poprawne dane, 0-niepoprawne dane
void logout(int sockfd);                                //funkcja do wylogowywania si�
void clear_line(int length);                            //funkcja do czyszczenia linii w konsoli

int main()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);  //zmienna do obs�ugi kolor�w w konsoli
    WSADATA wsaData;                                    //Struktura zawieraj�ca dane Windows socket
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);  //Wersja winSock 2.2
    if (result != NO_ERROR)                             //Sprawdzanie, czy socket zosta� utworzony
    {
        std::cout << "WSA_Startup failed: " << result << std::endl;
        return 1;
    }

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);       // tworzenie socketu IPv4 typu sock_stream
    struct addrinfo hints, * res;                       //� inicjalizacja struktury hints
    memset(&hints, 0, sizeof hints);                    // zapisanie struktury
    hints.ai_family = AF_INET;                          //IPv4
    hints.ai_socktype = SOCK_STREAM;                    //TCP

    getaddrinfo("127.0.0.1", "2000", &hints, &res);     //wpisanie adresu ip oraz portu serwera

    int nr_error = connect(sockfd, res->ai_addr, res->ai_addrlen);  //��czenie oraz obs�uga b��d�w
    if (nr_error != 0)
    {
        std::cout << "Connection error: " << nr_error;	            //W przypadku braku po��czenia wyj�cie z programu z b��dem 2
        return 2;
    }
    if (nr_error == 0)
    {
        std::cout << "Connected to the server" << std::endl;
    }
    /*******************************************************/

    std::string login, password, chat_name; //zmienne do trzymania loginu, has�a, imienia na czacie dla innych uczestnik�w
    char* lp_send;                          //zmienna do wys�ania powy�szych danych wraz z nag��wkiem do serwera
    int answer = 0;                         //imformacja zwrotna od serwera, je�eli dane s� poprawne to kontynuuj, je�eli nie to wprowad� jeszcze raz

    /*P�tla do wprowadzenia loginu oraz has�a, przekazanie tych informacji do serwera 
    oraz otrzymanie informacji zwrotnej, czy dane s� poprawne*/
    do
    {
        std::cout << "Login: ";
        std::getline(std::cin, login);
        std::cout << "Password: ";
        std::getline(std::cin, password);

        //stworzenie danych do wys�ania do serwera (nag��wek serwera do loginu oraz hasla to LOGIN)
        int size = sizeof("LOGIN:") + login.length() + sizeof(':') + password.length() + sizeof("\n\0") + 1;
        lp_send = new char[size];   
        strcpy_s(lp_send, size, ("LOGIN:" + login + ':' + password + "\n\0").c_str());

        //przes�anie danych do serwera
        answer = data_send(sockfd, lp_send);

        //Je�eli dane do logowania nie s� poprawne, zapytaj o nie jeszcze raz
        if (answer == 0)
        {
            std::cout << "incorrect login or password";
            Sleep(1000);
            system("CLS");
        }
        //Je�eli dane do logowania s� poprawne to przejd� do dalszej cz�ci programu
        else if (answer == 1)
        {
            system("CLS");
            //break;
        }

    } while (answer != 1);
    //P�tla do wprowadzenia chat_name, je�eli imie ju� jest zaj�te, to wprowad� jeszcze raz      
    do
    {
        std::cout << "Chat name: ";
        std::getline(std::cin, chat_name);

        int size = sizeof("NAME:") + sizeof(chat_name) + sizeof("\n") + 1;
        lp_send = new char[size];
        strcpy_s(lp_send, size, ("NAME:" + chat_name + "\n").c_str());

        if ((chat_name.find(':') != std::string::npos) || (chat_name.find(';') != std::string::npos))   //imie nie mo�e posiada� znak�w :;
        {
            answer = 0;
            std::cout << "Do not use \":\", \";\" symbols";
            Sleep(1000);
            system("CLS");
        }
        else
        {
            answer = data_send(sockfd, lp_send);        //przesy�anie imienia do serwera oraz otrzymanie odpowiedzi
            if (answer == 0)                            //Je�eli imie jest ju� zaj�te lub niepoprawe, zapytaj jeszcze raz
            {
                std::cout << "Chat name taken or incorrect";
                Sleep(1000);
                system("CLS");
            }
            else if (answer == 1)                       //Je�eli imie mo�e zosta� wpisane to przejd� do dalszej cz�ci programu
            {
                system("CLS");
            }
        }
    } while (answer != 1);
   
    //usuni�cie przycisku x, �eby ka�dy si� wylogowywa� (nie jest to idealna metoda, gdy� program mo�na zamkn�� przez np menager zada�)
    //lepsz� metod� by�oby obs�ugiwanie CTRL_CLOSE_EVENT, jednak z racji braku czasu (sesja) nie uda�o si� tego zrobi�.
    HWND hwnd = GetConsoleWindow();
    HMENU hmenu = GetSystemMenu(hwnd, FALSE);
    EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);

    std::cout << "message structure chat_name:message          np. Adrian:Co slychac?" << std::endl;
    std::cout << "to see log in users type LIST" << std::endl;
    std::cout << "to log out type LOGOUT" << std::endl;
    
    std::string buf;        //bufor do osbs�ugi klawiatury (std::cout, scanf, _getline itp. blokowa�y terminal, nawet przy u�yciu w�tk�w)
    std::string chat_input; //zmienna, do kt�rej buf jest zapisywaniy, je�eli przycisk enter zostanie naci�ni�ty
    chat_input.clear();     //wyczyszczenie pami�ci
    buf.clear();            //wyczyszczenie pami�ci

    //P�tla chatu, odpowiada za wprowadzanie wiadomo�ci, wysy�anie ich oraz odbieranie nowych wiadomo�ci i komunikat�w
    while (1)
    {
        SetConsoleTextAttribute(hConsole, 2);   //kolor tekstu w konsoli (zielony)
      
        if (_kbhit())                           //czytanie klawiatury, nieblokuj�co. _kbhit() zwraca TRUE, je�eli klawisz jest wci�ni�ty
        {                                       //je�eli klawisz jest wci�ni�ty zapisywaniy jest do bufora. ENTER zatwierdza bufor i wpisuje go do chat_input
            char znak;                          //je�eli backspace jest wci�ni�ty, usuwa wtedy znak z bufora, oraz ekranu. 
            znak = _getch();
            if (znak == 8)  //backspace
            {
                if (!buf.empty())               //zabezpieczenie, �eby do bufora nie zosta�o wpisane \b
                {
                    buf.pop_back();
                    std::cout << "\b \b";
                }
            }
            else if (znak == 13)                //enter, potwierdzenie bufora
            {
                if (!buf.empty())
                {
                    chat_input = buf;           //wpisanie wiadomo�ci do chat_input
                    clear_line(buf.length());   //wyczyszczenie linii
                    int where = buf.find(':');
                    if (where > 0 || buf == "LIST" || buf == "LOGOUT")  //sprawdzenie, czy polecenie to LIST lub LOGOUT, pomimo braku :, czyli odbiorcy
                    {
                        if (buf == "LOGOUT")
                        {
                            logout(sockfd); //wylogowanie (na serwerze usuni�cie klienta), tutaj zamkni�cie socketu, oczyszczenie WSA
                            std::exit(0);   //wyj�cie z programu
                        }
                        std::cout << chat_name << "->" << buf;  //strza�ka, kt�ra informuje od kogo do kogo jest wiadomo��, wypisanie wys�anej wiadomo�ci (sprawdzenie, czy wiadomo�� zosta�a wys�ana jest p�niej)
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

        //wysy�anie polece� do serwera
        std::vector <char> vec_chat;
        if (!chat_input.empty())    //wysy�aj dane, je�eli s�
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

        //sprawdzanie, czy s� jakie� dane do odebrania z serwera oraz odbi�r danych
        pollfd pollfd_data;
        pollfd_data.fd = sockfd;
        pollfd_data.events = POLLIN;

        if (WSAPoll(&pollfd_data, 1, 0) > 0)    //u�ycie WSAPoll, �eby odbieranie wiadomo�ci odbywa�o si� ca�y czas, nieblokuj�co
        {
            if (pollfd_data.revents & POLLIN)   //je�eli s� dane do odebrania
            {
                do
                {
                    n = recv(sockfd, answer, strlen(answer), 0);

                    if (n > 0)
                    {
                        str.append(answer, n);
                        if (str.find('\n') != std::string::npos)    //wpisywanie danych a� do znaku nowej linii
                        {
                            break;
                        }
                    }
                } while (n != 0);
                if (str.find(';') != std::string::npos) //szukanie ; w LIST, osoby s� tak oddzielane, zast�pienie znakiem nowej linii do wypisania.
                {
                    for (int i = 0; i < str.length(); i++)
                    {
                        if (str[i] == ';')
                        {
                            str[i] = '\n';
                        }
                    }
                }
                else if (str == "message send error\n") //je�eli serwer zwr�ci error, to usu� lini� wys�anej wiadomo�ci, wypisz komunikat na sekund�, wyczy�� lini�
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
                SetConsoleTextAttribute(hConsole, 7);   //kolor tekstu w konsoli (bia�y)
                std::cout << str;                       //wypisanie wiadomo�ci, kt�r� klient otrzyma�
                SetConsoleTextAttribute(hConsole, 2);   //kolor tekstu w konsoli (zielony)
                std::cout << buf;                       //wypisanie tego, co pisali�my w tym kliencie
            }  
        }
    }
    
    closesocket(sockfd);    //zamkni�cie socketu
    WSACleanup();           //zamkni�cie winsock 2
}

//Funkcje
int data_send(int sockfd, char *lp_send)
{
    long n = 0;	                        //zmienna do sumowania ilo�ci wys�anych bit�w (co iteracje)
    long total = 0;			            //zmienna do sumowania wys�anych bit�w (��cznie)
    long bytesleft = strlen(lp_send);   //zmienna, z kt�rej co iteracje odejmowane s� warto�ci, u�ywanie przy send, aby wys�a� okre�lon� ilo�� bit�w.
    while (total < strlen(lp_send))     //Przesy�anie, dop�ki ca�kowita ilo�� bajt�w nie b�dzie r�wna d�ugo�ci pliku
    {
        n = send(sockfd, lp_send + total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;	    //sumowanie ilo�ci wys�anych bit�w
        bytesleft -= n;	//odejmowanie bit�w wys�anych od ca�o�ciowej liczby pocz�tkowej.
    }

    //odebranie od serwera odpowiedzi (1 - zgadza si�, 0 - nie zgadza si�)
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
    {                                       //cofni�cie kursora w konsoli tyle razy ile jest znak�w w buf
        for (int j = 0; j < 3; j++)         //wypisanie " " tyle razy ile jest znak�w w buf
        {                                   //cofni�cie kursora w konsoli tyle razy ile jest znak�w w buf
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

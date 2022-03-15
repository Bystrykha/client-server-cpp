#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>

using namespace std;

bool checkData(char* s){
    if ((strlen(s) > 2) and (atoi(s) % 32 == 0)){
        cout << "Get: " << s << '\n';
        return false;
    } else if(string(s) == "-q") {
        return true;
    }else {
        cout << "Error." << "\n";
        return false;
    }
}

int main()
{
    int client, server;
    int portNum = 8080;
    bool isExit = false;
    char buffer[64] = {0};
    struct sockaddr_in server_addr{};
    socklen_t size;

    client = socket(AF_INET, SOCK_STREAM, 0);


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(portNum);

    if ((bind(client, (struct sockaddr*)&server_addr,sizeof(server_addr))) < 0) // bind - привязка сокета к адресу
    {
        cout << "=> Error binding connection, the socket has already been established..." << endl;
        return -1;
    }

    size = sizeof(server_addr);

    listen(client, 1);

    server = accept(client,(struct sockaddr *)&server_addr,&size);

    while (!isExit){
        recv(server, buffer, sizeof(buffer), 0);
        isExit = checkData(buffer);
    }

    close(server);
    cout << "\nGoodbye..." << endl;
    close(client);
    return 0;
}

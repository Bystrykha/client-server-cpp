#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <condition_variable>
#include <string>
#include <regex>

class threadSynchronizer{
public:
    std::condition_variable condition;
    std::mutex m;
    bool finish;
    bool clearBuffer;

    threadSynchronizer(){
        finish = false;
        clearBuffer = true;
    }

};

void sendData(int client, const std::string& s){
    char b[64];
    strcpy(b, s.c_str());
    send(client, b, sizeof(b), 0);
}

void clearingBuffer(threadSynchronizer &synchronizer){
    synchronizer.clearBuffer = true;
    std::ofstream buffer;
    buffer.open("shared_file.txt");
    buffer << "";
    buffer.close();
}

int getSum(std::string &s){
    int sum = 0;
    for (char c : s){
        if (c > 48 and  c <=57) sum += int(c) - 48;
    }
    return sum;
}

std::string evenReplace(const std::string& s){
    std::string res;
    for (char i : s){
        if (int(i) % 2 == 0) res += "KB";
        else res += i;
    }
    return res;
}

std::string getString(threadSynchronizer &synchronizer){
    std::string s;
    std::regex pattern("[0-9]+");
    while(true){
        std::getline(std::cin, s);
        if(((std::regex_match(s, pattern)) and s.size() <= 64)) break;
        else if (s =="-q") {
            synchronizer.finish = true;
            return s;
        }
    }
    std::sort(s.begin(), s.end());
    std::reverse(s.begin(), s.end());
    return evenReplace(s);
}

int pushToBuffer(threadSynchronizer &synchronizer){
    while (true){
        std::ofstream buffer;

        std::string s = getString(synchronizer);
        buffer.open("shared_file.txt", std::ios::app);

        {
            std::lock_guard<std::mutex> lg(synchronizer.m);
            buffer << s;
            buffer.close();
            synchronizer.clearBuffer = false;
            synchronizer.condition.notify_all();
        }
        synchronizer.m.unlock();
        if (synchronizer.finish) break;
    }
    return 0;
}

int popFromBuff(threadSynchronizer &synchronizer, int client){
    while(true){
        std::ifstream buffer;
        std::string s;
        buffer.open("shared_file.txt");
        bool *a = &synchronizer.clearBuffer;
        {
            std::unique_lock<std::mutex> ul(synchronizer.m);
            synchronizer.condition.wait(ul, [a]{
                return !*a;});
            std::getline(buffer, s);
            std::cout << s << '\n';
            clearingBuffer(synchronizer);
            ul.unlock();
        }
        buffer.close();
        if (synchronizer.finish) {
            sendData(client, "-q");
            break;
        }
        sendData(client, std::to_string(getSum(s)));
    }
    return 0;
}



int main(){
    int client;
    int portNum = 8080;
    std::string IP = "127.0.0.1";
    char ip[IP.size()];
    IP.copy(ip, IP.size());
    struct sockaddr_in server_addr{};

    client = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNum);

    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(client,(struct sockaddr*)&server_addr, sizeof(server_addr)) == 0)
        std::cout << "=> Connection to the server port number: " << portNum << '\n';

    threadSynchronizer synchronizer;
    std::thread thread_1(pushToBuffer, std::ref(synchronizer));
        std::thread thread_2(popFromBuff, std::ref(synchronizer), client);

    thread_2.join();
    thread_1.join();

    close(client);
    return 0;
}


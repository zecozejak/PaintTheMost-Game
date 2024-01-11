#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <thread>
#include <queue>
#include <mutex>
#include <sys/fcntl.h>
#include <SFML/Graphics.hpp>
#include <iomanip>
#include <time.h>

const sf::Time roundTime = sf::seconds(90.0f); //czas gry

int activePlayerCount = 0;
const int windowWidth = 800;
const int windowHeight = 600;
const int gridSize = 10;
const int gridWidth = windowWidth / 2;
const int gridHeight = windowHeight / 2;

//pozycja planszy
const int gridX = windowWidth / 4;
const int gridY = windowHeight / 4;

struct PlayerInfo {
    float x, y;  // Player position
    sf::Color color;
};

std::unordered_map<int, PlayerInfo> playersInGameMap; //map which contains info about Players
std::vector<std::vector<sf::Color> > visited(gridWidth / gridSize, std::vector<sf::Color>(gridHeight / gridSize, sf::Color::White));

std::queue<PlayerInfo> movementQueue;
std::mutex movementMutex;

void communicationFunction(int serverSocket) {
    while (1) {
        std::unique_lock<std::mutex> lock(movementMutex); //check if queue is not empty
        for (auto &[playerFD , Player] : playersInGameMap) {
            if (read(playerFD, ))
                if (errno = EWOULDBLOCK) continue;
        }
    }
}

void gameLogicThread() {

    while (1) {
        // time start -> sending information to all currently playing clients about time
        printf("%d", activePlayerCount);
        if(activePlayerCount == 4) {
            sf::Clock clock;
            sf::Time elapsedTime = clock.getElapsedTime();
            sf::Time remainingTime = roundTime - elapsedTime;
            if (remainingTime <= sf::Time::Zero) {
                remainingTime = sf::Time::Zero;  //unikanie wartosci ujemnych
            }
            std::cout << "\nPozostaly czas: " << std::setfill('0') << std::setw(2)
                  << static_cast<int>(remainingTime.asSeconds()) / 60 << ":"
                  << std::setfill('0') << std::setw(2) << static_cast<int>(remainingTime.asSeconds()) % 60
                  << std::flush;

            sf::Int32 millisecondsToSend = remainingTime.asMilliseconds();

            for (const auto &player: playersInGameMap) {
                int playerSocket = player.first;

                write(playerSocket, &millisecondsToSend, sizeof(millisecondsToSend));
            }
        }
    }
   // TODO: funkcja update otrzymująca 'x' i 'y' z kolejki wraz z numerem gracza, traktująca je jako współrzędne VISITED

}

void setReuseAddr(int sock){
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(res){
        perror("setsockopt failed");
        printf("Error code: %d\n", errno);
    }
}

int main(int argc, char ** argv) {

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Failed creating a socket");
        printf("Error code: %d\n", errno);
        return -1;
    }

    setReuseAddr(serverSocket);

    //signal(SIGINT, ctrl_c);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);

    int res = bind(serverSocket,(sockaddr*) &serverAddr, sizeof(serverAddr));
    if(res){
        perror("Bind failed.");
        printf("Error code: %d\n", errno);
    }
    res = listen(serverSocket,10);
    if (res){
        perror("Error listening for connections.");
        printf("Error code: %d\n", errno);
        close(serverSocket);
        return -1;
    }

    std::thread communicationThread(communicationFunction, serverSocket);
    std::thread gameThread(gameLogicThread);

    while(1) {
        if(activePlayerCount != 4) {
            int client = accept(serverSocket, nullptr, nullptr);
            fcntl(client, F_SETFL, O_NONBLOCK);
            std::cout << "New connection accepted" << std::endl;
            activePlayerCount += 1;
            // TODO: client disconnection == activePlayerCount -=1
            PlayerInfo newPlayer;
            // funkcja do iterowania przez graczy nadajaca im konkretny kolor i pozycje na mapie
            newPlayer.x = windowWidth / 3.0f;
            newPlayer.y = windowHeight / 2.0f;
            newPlayer.color = sf::Color::Red; // do zmiany
            playersInGameMap[client] = newPlayer;
        }
    }

};

void ctrl_c(){
    // TODO: client closing on ctrl + c
}


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
bool timeExpired = false;
char readyBuf[1];
int readyCount = 0;

//pozycja planszy
const int gridX = windowWidth / 4;
const int gridY = windowHeight / 4;

struct PlayerInfo {
    float x, y;  // Player position
    sf::Color color;
};

struct Coordinates {
    float x;
    float y;
};

std::unordered_map<int, PlayerInfo> playersInGameMap; //map which contains info about Players
std::vector<std::vector<sf::Color> > visited(gridWidth / gridSize, std::vector<sf::Color>(gridHeight / gridSize, sf::Color::White));
std::vector<sf::Color> colors;
std::vector<Coordinates> beginCords;

void communicationFunction(int serverSocket) {
    std::vector<int> toDisconnect;
    char movementBuf[8];
    while (1) {
        // taking care of all "t" from clients
        while (readyCount != 4) {
            for (auto &[playerFD, Player]: playersInGameMap) {
                ssize_t bytesRead = read(playerFD, &readyBuf, 1);
                if (bytesRead > 0 && readyBuf[0] == 't') {
                    readyCount++;
                }
            }
        }
        for (auto &[playerFD, Player]: playersInGameMap) {
            if (read(playerFD, &movementBuf, 8) == -1) {
                if (errno == EWOULDBLOCK) continue;
                toDisconnect.push_back(playerFD);
            }
            auto *stateInfo = reinterpret_cast<PlayerInfo *>(movementBuf);

//            if(stateInfo->x == Player.x && stateInfo->y == Player.y) {
                // updating position of every player
                if (stateInfo->x >= 0 && stateInfo->x < visited.size() && stateInfo->y >= 0 &&
                    stateInfo->y < visited[0].size()) {
                    Player.x = stateInfo->x;
                    Player.y = stateInfo->y;
                    visited[Player.x][Player.y] = Player.color;
                }

                for (const auto &player: playersInGameMap) {
                    int playerSocket = player.first;
                    char updateBuf[sizeof(PlayerInfo)];
                    // to powoduje segfolta (chyba niepoprawnie wyslany struct?)
                    PlayerInfo *updateInfo = reinterpret_cast<PlayerInfo *>(updateBuf);
                    updateInfo->x = Player.x;
                    updateInfo->y = Player.y;
                    updateInfo->color = Player.color;

                    //write(playerSocket, updateBuf, sizeof(PlayerInfo));
                    //printf("%s", updateBuf);
                }
//            }
        }
    }
}

float calculatePercentage(const std::vector<std::vector<sf::Color>>& visited, const sf::Color& color) {
    int totalGrids = 0;
    int coloredGrids = 0;

    for (int i = 0; i < visited.size(); ++i) {
        for (int j = 0; j < visited[i].size(); ++j) {
            if (visited[i][j] == color) {
                coloredGrids++;
            }
            totalGrids++;
        }
    }
    return static_cast<float>(coloredGrids) / totalGrids * 100.0f;
}

void gameLogicThread() {
    while(activePlayerCount != 4);
    while(readyCount != 4);
    while(readyCount == 4) {
        auto clock = new sf::Clock;

        // timer sending
        sf::Int32 millisecondsToSend = roundTime.asMilliseconds();
        for (const auto &player: playersInGameMap) {
            int playerSocket = player.first;
            write(playerSocket, &millisecondsToSend, sizeof(millisecondsToSend));
        }

        while (activePlayerCount == 4) {
            sf::Time elapsedTime = clock->getElapsedTime();
            // time start -> sending information to all currently playing clients about time
            sf::Time remainingTime = roundTime - elapsedTime;
            if (remainingTime <= sf::Time::Zero) {
                timeExpired = true;
                remainingTime = sf::Time::Zero;  //unikanie wartosci ujemnych
            }
            std::cout << "\rPozostaly czas: " << std::setfill('0') << std::setw(2)
                      << static_cast<int>(remainingTime.asSeconds()) / 60 << ":"
                      << std::setfill('0') << std::setw(2) << static_cast<int>(remainingTime.asSeconds()) % 60
                      << std::flush;

            if (timeExpired) {
                break;
            }
        }
        delete clock;
        std::cout << "\nKONIEC CZASU\n";
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
    colors.push_back(sf::Color::Red);
    colors.push_back(sf::Color::Blue);
    colors.push_back(sf::Color::Green);
    colors.push_back(sf::Color::Yellow);

    beginCords.push_back({windowWidth / 3.0f, windowHeight / 2.0f}); // position 0 for red player
    beginCords.push_back({2.0f * windowWidth / 3.0f, windowHeight / 2.0f}); // pos 0 for blue player
    beginCords.push_back({windowWidth / 3.0f, 2.0f * windowHeight / 3.0f}); // pos 0 for green player
    beginCords.push_back({2.0f * windowWidth / 3.0f, 2.0f * windowHeight / 3.0f}); // pos 0 for yellow player

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
            newPlayer.x = beginCords[activePlayerCount-1].x; // vector starts with 0
            newPlayer.y = beginCords[activePlayerCount-1].y; // vector starts with 0
            newPlayer.color = colors[activePlayerCount-1];
            playersInGameMap.insert({client, newPlayer});
            write(client, &newPlayer, sizeof(newPlayer));
        }
    }

};

void ctrl_c(){
    // TODO: client closing on ctrl + c
}


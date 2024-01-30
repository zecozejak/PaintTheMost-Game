#include "../shared/communication.h"
#include <SFML/Graphics.hpp>
#include <condition_variable>
#include <cstring>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

const sf::Time roundTime = sf::seconds(90.0f); // czas gry

int activePlayerCount = 0;
const int windowWidth = 800;
const int windowHeight = 600;
const int gridSize = 10;
const int gridWidth = windowWidth / 2;
const int gridHeight = windowHeight / 2;
bool timeExpired = false;
char readyBuf[1];
int readyCount = 0;

// pozycja planszy
const int gridX = windowWidth / 4;
const int gridY = windowHeight / 4;

#pragma pack(push, 1) // Set packing to 1 byte
struct PlayerInfo {
    float x, y; // Player position
    int intColor;
};
#pragma pack(pop) // Restore default packing

struct Coordinates {
    float x;
    float y;
};

std::condition_variable cvGameLogic;
std::condition_variable cvGameLogic2;
std::mutex mtx2;
std::mutex mtx3;

std::unordered_map<int, PlayerInfo>
        playersInGameMap; // map which contains info about Players
std::vector<std::vector<sf::Color>>
        visited(gridWidth / gridSize,
                std::vector<sf::Color>(gridHeight / gridSize, sf::Color::White));
std::vector<sf::Color> colors;
std::vector<Coordinates> beginCords;

void communicationFunction(int serverSocket) {
    std::unique_lock<std::mutex> lock(mtx2);
    cvGameLogic.wait(lock,
                     [] { return activePlayerCount == 4 && readyCount == 4; });

    std::vector<int> toDisconnect;
    char movementBuf[8];
    while (1) {
        for (auto &[playerFD, Player] : playersInGameMap) {
            if (read(playerFD, &movementBuf, 8) == -1) {
                if (errno == EWOULDBLOCK)
                    continue;
                toDisconnect.push_back(playerFD);
            }
            auto *stateInfo = reinterpret_cast<PlayerInfo *>(movementBuf);

            if (stateInfo->x == Player.x && stateInfo->y == Player.y) {
                continue;
            }
            std::cout << "jestem poza kontrolą" << std::endl;
            // updating position of every player
            if (stateInfo->x >= 0 && stateInfo->x < visited.size() &&
                stateInfo->y >= 0 && stateInfo->y < visited[0].size()) {
                Player.x = stateInfo->x;
                Player.y = stateInfo->y;
                visited[Player.x][Player.y] = colors[Player.intColor];
            }

            for (const auto &player : playersInGameMap) {
                int playerSocket = player.first;
                char updateBuf[12];
                auto *updateInfo = reinterpret_cast<PlayerInfo *>(updateBuf);
                updateInfo->x = Player.x;
                updateInfo->y = Player.y;
                updateInfo->intColor = Player.intColor;

                sendWithLength(playerSocket, updateBuf, 12);
            }
        }
    }
}

float calculatePercentage(const std::vector<std::vector<sf::Color>> &visited,
                          const sf::Color &color) {
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
    std::unique_lock<std::mutex> lock(mtx3);
    cvGameLogic2.wait(lock,
                      [] { return activePlayerCount == 4 && readyCount == 4; });
    std::cout << "uwbwubuwbuwbuwuw" << std::endl;
    char ready = 0xf;
    for (const auto &player : playersInGameMap) {
        int playerSocket = player.first;
        sendWithLength(playerSocket, &ready, 1);
        std::cout << ready << std::endl;
    }

    while (readyCount == 4) {

        auto clock = new sf::Clock;

        while (activePlayerCount == 4) {
            lock.unlock();
            sf::Time elapsedTime = clock->getElapsedTime();
            sf::Time remainingTime = roundTime - elapsedTime;
            if (remainingTime <= sf::Time::Zero) {
                timeExpired = true;
                remainingTime = sf::Time::Zero; // unikanie wartosci ujemnych
            }
            std::cout << "\rPozostaly czas: " << std::setfill('0') << std::setw(2)
                      << static_cast<int>(remainingTime.asSeconds()) / 60 << ":"
                      << std::setfill('0') << std::setw(2)
                      << static_cast<int>(remainingTime.asSeconds()) % 60
                      << std::flush;

            lock.lock();
            if (timeExpired) {
                break;
            }
        }
        delete clock;
        std::cout << "\nKONIEC CZASU\n";
    }
}

void setReuseAddr(int sock) {
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (res) {
        perror("setsockopt failed");
        printf("Error code: %d\n", errno);
    }
}

std::mutex mtx;
std::condition_variable cvStart;

int main(int argc, char **argv) {
    colors.push_back(sf::Color::Red);
    colors.push_back(sf::Color::Blue);
    colors.push_back(sf::Color::Green);
    colors.push_back(sf::Color::Yellow);

    beginCords.push_back(
            {windowWidth / 3.0f, windowHeight / 2.0f}); // position 0 for red player
    beginCords.push_back({2.0f * windowWidth / 3.0f,
                          windowHeight / 2.0f}); // pos 0 for blue player
    beginCords.push_back({windowWidth / 3.0f,
                          2.0f * windowHeight / 3.0f}); // pos 0 for green player
    beginCords.push_back({2.0f * windowWidth / 3.0f,
                          2.0f * windowHeight / 3.0f}); // pos 0 for yellow player

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Failed creating a socket");
        printf("Error code: %d\n", errno);
        return -1;
    }

    setReuseAddr(serverSocket);

    struct sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    int res = bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr));
    if (res) {
        perror("Bind failed.");
        printf("Error code: %d\n", errno);
    }
    res = listen(serverSocket, 10);
    if (res) {
        perror("Error listening for connections.");
        printf("Error code: %d\n", errno);
        close(serverSocket);
        return -1;
    }

    std::thread communicationThread(communicationFunction, serverSocket);
    std::thread gameThread(gameLogicThread);

    while (true) {
        int client = accept(serverSocket, nullptr, nullptr);
        int status = fcntl(client, F_SETFL, O_NONBLOCK);
        if (status == -1) {
            shutdown(serverSocket, SHUT_RDWR);
            close(serverSocket);
        }
        std::cout << "New connection accepted" << std::endl;

        std::unique_lock<std::mutex> lock(mtx);

        activePlayerCount += 1;

        PlayerInfo newPlayer;
        newPlayer.x = beginCords[activePlayerCount - 1].x; // vector starts with 0
        newPlayer.y = beginCords[activePlayerCount - 1].y; // vector starts with 0
        newPlayer.intColor = activePlayerCount - 1;
        playersInGameMap.insert({client, newPlayer});
        std::cout << "Size of MyStruct: " << sizeof(newPlayer) << " bytes"
                  << std::endl;
        char bytesSend[12];
        memcpy(bytesSend, &newPlayer, 12);
        sendWithLength(client, bytesSend, sizeof(bytesSend));

        if (activePlayerCount == 4) {
            // taking care of all "t" from clients
            while (readyCount != 4) {
                for (auto &[playerFD, Player] : playersInGameMap) {
                    ssize_t bytesRead = read(playerFD, &readyBuf, 1);
                    if (bytesRead == -1) {
                        if (errno == EWOULDBLOCK)
                            continue;
                        throw std::runtime_error("lllll");
                    }
                    if (bytesRead > 0 && readyBuf[0] == 't') {
                        readyCount++;
                        std::cout << "AAAAAAAAA" << std::endl;
                    }
                }
            }
            cvGameLogic.notify_one();
            cvGameLogic2.notify_one();
            cvStart.wait(lock, [] { return activePlayerCount < 4; });
        }

        lock.unlock();
    }
};

// obsluzyc rozlaczenia graczy

// toDisconnect pociagnac do konca

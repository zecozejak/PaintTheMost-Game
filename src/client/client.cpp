#include "../shared/communication.h"
#include <SFML/Graphics.hpp>
#include <arpa/inet.h>
#include <cmath>
#include <cstring>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <optional>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

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

const int gridSize = 10; // rozmiar siatki
const int windowWidth = 800;
const int windowHeight = 600;
const float squareSize = 10.0f;
const float outlineThickness = 1.0f;
sf::Time remainingTime; // czas od serwera
char ready;
const sf::Time roundTime = sf::seconds(15.0f); // czas gry

// ustawienia biala plansza co bedzie zamalowywana
const int gridWidth = windowWidth / 2;
const int gridHeight = windowHeight / 2;
// pozycja planszy
const int gridX = windowWidth / 4;
const int gridY = windowHeight / 4;
bool pleaseStart = false;
bool timeExpired = true;

std::vector<std::vector<sf::Color>>
        visited(gridWidth / gridSize,
                std::vector<sf::Color>(gridHeight / gridSize, sf::Color::White));
std::vector<sf::Color> colors;

#pragma pack(push, 1) // Set packing to 1 byte
struct PlayerInfo {
    float x, y; // Player position
    int intColor;
};
#pragma pack(pop) // Restore default packing

std::optional<PlayerInfo> readPlayerUpdate(int socketFD) {

    char bytesSend[12]{};
    ssize_t bytesRead = receive(socketFD, bytesSend);
    if (bytesRead == 0)
        return std::nullopt;
    PlayerInfo updateInfo{};
    memcpy(&updateInfo.x, bytesSend, sizeof(float));
    memcpy(&updateInfo.y, bytesSend + sizeof(float), sizeof(float));
    memcpy(&updateInfo.intColor, bytesSend + 2 * sizeof(float), sizeof(int));
    std::cout << sizeof(updateInfo) << std::endl;
    std::cout << bytesRead << std::endl;

    return updateInfo;
}

// simple function to send player current position to the server
void sendPlayerMovement(int socketFD, float x, float y) {
    PlayerInfo info = {x, y, 0};
    char toSend[12];
    memcpy(toSend, &info.x, sizeof(float));
    memcpy(toSend + sizeof(float), &info.y, sizeof(float));
    memcpy(toSend + 2 * sizeof(float), &info.intColor, sizeof(int));
    sendWithLength(socketFD, toSend, 12);
}

// function to sent readiness to server
void sendPlayerReadiness(int socketFD) {
    std::cout << "Czy jestes gotowy? [t] ";
    std::cin >> ready;
    sendWithLength(socketFD, &ready, 1);
};

int main(int argc, char **argv) {
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight),
                            "Paint the most game");

    colors.push_back(sf::Color::Red);
    colors.push_back(sf::Color::Blue);
    colors.push_back(sf::Color::Green);
    colors.push_back(sf::Color::Yellow);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Failed creating a socket");
        printf("Error code: %d\n", errno);
        return -1;
    }

    struct sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(8080);

    if (connect(fd, (sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        if (errno != EINPROGRESS) {
            perror("Connection failed");
            printf("Error code: %d\n", errno);
            close(fd);
            return -1;
        }
    }

    int status = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    if (status == -1) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    std::cout << "Connected to the server." << std::endl;
    std::optional<PlayerInfo> firstInfo{};

    while (true) {
        firstInfo = readPlayerUpdate(fd);
        if (firstInfo.has_value())
            break;
    }

    sf::RectangleShape player1(sf::Vector2f(squareSize, squareSize));
    sf::Color player1Color = colors[firstInfo->intColor];
    player1.setFillColor(player1Color);
    player1.setOutlineColor(sf::Color::Black);
    player1.setOutlineThickness(outlineThickness);
    player1.setPosition(firstInfo->x, firstInfo->y);

    sendPlayerReadiness(fd);
    sf::Time receivedTime = roundTime; // sf::milliseconds(receiveFirstTime(fd));
    timeExpired = false;
    char rBuff;
    while (!pleaseStart) {
        sleep(1);
        std::cout << "czekam" << std::endl;
        if (read(fd, &rBuff, 1) == -1) {
            if (errno == EWOULDBLOCK)
                continue;
            throw std::runtime_error(
                    "mamy problem w prosze startowac panie kapitanie");
        }
        if (rBuff == 0xf) {
            pleaseStart = true;
        }
        std::cout << rBuff << std::endl;
    }
    // Who let the clock out?
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // game start
        if (!timeExpired) {
            bool shouldMove = false;

            sf::Time elapsedTime = clock.getElapsedTime();
            // time start -> sending information to all currently playing clients
            // about time
            remainingTime = receivedTime - elapsedTime;
            if (remainingTime <= sf::Time::Zero) {
                timeExpired = true;
                remainingTime = sf::Time::Zero; // unikanie wartosci ujemnych
            }
            std::cout << "\rPozostaly czas: " << std::setfill('0') << std::setw(2)
                      << static_cast<int>(remainingTime.asSeconds()) / 60 << ":"
                      << std::setfill('0') << std::setw(2)
                      << static_cast<int>(remainingTime.asSeconds()) % 60
                      << std::endl;

            // movement keys
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) &&
                player1.getPosition().x > gridX && window.hasFocus()) {
                player1.move(-1.0f, 0.0f);
                shouldMove = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) &&
                player1.getPosition().x < gridX + gridWidth - player1.getSize().x &&
                window.hasFocus()) {
                player1.move(1.0f, 0.0f);
                shouldMove = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) &&
                player1.getPosition().y > gridY && window.hasFocus()) {
                player1.move(0.0f, -1.0f);
                shouldMove = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) &&
                player1.getPosition().y < gridY + gridHeight - player1.getSize().y &&
                window.hasFocus()) {
                player1.move(0.0f, 1.0f);
                shouldMove = true;
            }

            // sending position to server
            if (shouldMove) {
                float valueX = (player1.getPosition().x - gridX) / gridSize;
                float valueY = (player1.getPosition().y - gridY) / gridSize;
                std::cout << "WE ARRE MOVING AT MACH 3" << std::endl;
                std::cout << valueX << std::endl;
                std::cout << valueY << std::endl;
                std::cout << "Ending" << std::endl;
                sendPlayerMovement(
                        fd, valueX,
                        valueY);
            }

            std::optional<PlayerInfo> updatePlayer = readPlayerUpdate(
                    fd);
            if (updatePlayer.has_value()) {
                std::cout << "pronting" << std::endl;
                std::cout << updatePlayer->x << std::endl;
                std::cout << updatePlayer->y << std::endl;
                std::cout << updatePlayer->intColor << std::endl;
                std::cout << "Ended pronting" << std::endl;
                std::cout << "Visited is of size: " << visited.size() << " and "
                          << visited[0].size() << std::endl;
                visited[std::floor(updatePlayer->x)][std::floor(updatePlayer->y)] =
                        colors[updatePlayer->intColor];
            }
        } else {
            window.close();
            std::cout << "THE GAME HAS ENDED" << std::endl;
            std::cout << "Total percentage" << std::endl;
            for (const auto color : colors) {
                float percentage = calculatePercentage(visited, color);
                std::cout << "The percentage for color: ";
                if (color == sf::Color::Red) {
                    std::cout << "red: ";
                } else if (color == sf::Color::Blue) {
                    std::cout << "blue: ";
                } else if (color == sf::Color::Green) {
                    std::cout << "green: ";
                } else {
                    std::cout << "yellow: ";
                }

                std::cout << percentage << std::endl;
            }

            continue;
        }

        window.clear();

        // table drawing
        for (int i = 0; i < visited.size(); ++i) {
            for (int j = 0; j < visited[i].size(); ++j) {
                sf::RectangleShape gridSquare(sf::Vector2f(gridSize, gridSize));
                gridSquare.setPosition(gridX + i * gridSize, gridY + j * gridSize);
                gridSquare.setFillColor(visited[i][j]);
                window.draw(gridSquare);
            }
        }

        window.draw(player1);
        window.display();
    }

    shutdown(fd, SHUT_RDWR);
    close(fd);
    return 0;
}
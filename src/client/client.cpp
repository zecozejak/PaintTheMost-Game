#include <iostream>
#include <vector>
#include <map>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <SFML/Graphics.hpp>
#include <thread>
#include <time.h>
#include <iomanip>
#include <sys/fcntl.h>

const int gridSize = 10;  //rozmiar siatki
const int windowWidth = 800;
const int windowHeight = 600;
const float squareSize = 10.0f;
const float outlineThickness = 1.0f;
sf::Time remainingTime; // czas od serwera
char ready;

//ustawienia biala plansza co bedzie zamalowywana
const int gridWidth = windowWidth / 2;
const int gridHeight = windowHeight / 2;
//pozycja planszy
const int gridX = windowWidth / 4;
const int gridY = windowHeight / 4;
bool start = false;
bool timeExpired = true;

std::vector<std::vector<sf::Color> > visited(gridWidth / gridSize, std::vector<sf::Color>(gridHeight / gridSize, sf::Color::White));

struct PlayerInfo {
    float x, y;  // Player position
    sf::Color color;
};

struct ColorInfo {
    int r;
    int g;
    int b;
};
// function to receive starting position
PlayerInfo receivePosAndCol(int socketFD) {
    char beginningBuf[14];
    read(socketFD, &beginningBuf, 14);
    return *reinterpret_cast<PlayerInfo*>(beginningBuf);
}

// thread to receive all game states from the server
void receiveTimeState(int fd) {
    // TODO: loop z updatem tablicy
//    while(1) {
//        sf::Int32 receivedMilliseconds;
//        ssize_t receivedSize = read(fd, &receivedMilliseconds, sizeof(receivedMilliseconds));
//        if (receivedSize == -1) {
//            std::cerr << "Error receiving data" << std::endl;
//        } else {
//            timeExpired = false;
//            sf::Time receivedTime = sf::milliseconds(receivedMilliseconds);
//            float seconds = std::round(receivedTime.asSeconds());
//
//            std::cout << "\rPozostaly czas: " << std::setfill('0') << std::setw(2) << static_cast<int>(seconds) / 60 << ":"
//                      << std::setfill('0') << std::setw(2) << static_cast<int>(seconds) % 60 << "  " << std::flush;
//        }
//    }
    // TODO: read informacje o zwycięzcy
}

int receiveFirstTime(int fd){
    sf::Int32 receivedMilliseconds;
    ssize_t receivedSize = read(fd, &receivedMilliseconds, sizeof(receivedMilliseconds));
    if (receivedSize == -1) {
        std::cerr << "Error receiving data" << std::endl;
    }
    timeExpired = false;
    return receivedMilliseconds;
}
// std::optional<PlayerInfo> readPlayerUpdate(int socketFD) {
PlayerInfo readPlayerUpdate(int socketFD) {
    PlayerInfo updateInfo;
    ssize_t bytesRead = read(socketFD, &updateInfo, sizeof(updateInfo));
    printf("\n%f\n", updateInfo.x);
    printf("%f\n", updateInfo.y);
    printf("%d", updateInfo.color.b);
    if (bytesRead == -1) {
        perror("Error reading player update");
    }

    return updateInfo;
}

//simple function to send player current position to the server
void sendPlayerMovement(int socketFD, float x, float y) {
    PlayerInfo info = {x, y};
    write(socketFD, &info, 8);
}

//function to sent readiness to server
void sendPlayerReadiness(int socketFD) {
    std::cout << "Czy jestes gotowy? [t] ";
    std::cin >> ready;
    write(socketFD, &ready, 1);
};

int main(int argc, char ** argv){
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Paint the most game");

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Failed creating a socket");
        printf("Error code: %d\n", errno);
        return -1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);

    if(connect(fd,(sockaddr*) &serverAddr, sizeof(serverAddr)) == -1) {
        if (errno != EINPROGRESS) {
            perror("Connection failed");
            printf("Error code: %d\n", errno);
            close(fd);
            return -1;
        }
    }

    std::cout << "Connected to the server." << std::endl;

    //fcntl(fd, F_SETFL, O_NONBLOCK); insta segfault

    PlayerInfo firstInfo = receivePosAndCol(fd);

    sf::RectangleShape player1(sf::Vector2f(squareSize, squareSize));
    sf::Color player1Color = firstInfo.color;
    player1.setFillColor(player1Color);
    player1.setOutlineColor(sf::Color::Black);
    player1.setOutlineThickness(outlineThickness);
    player1.setPosition(firstInfo.x, firstInfo.y);

    sendPlayerReadiness(fd);
    sf::Time receivedTime = sf::milliseconds(receiveFirstTime(fd));
    timeExpired = false;

    //std::thread receiveThread(receiveTimeState, fd);

    // Who let the clock out?
    sf::Clock clock;
    int firstSendImportant = 0;

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
            // time start -> sending information to all currently playing clients about time
            remainingTime = receivedTime - elapsedTime;
            if (remainingTime <= sf::Time::Zero) {
                timeExpired = true;
                remainingTime = sf::Time::Zero;  //unikanie wartosci ujemnych
            }
            std::cout << "\rPozostaly czas: " << std::setfill('0') << std::setw(2)
                      << static_cast<int>(remainingTime.asSeconds()) / 60 << ":"
                      << std::setfill('0') << std::setw(2) << static_cast<int>(remainingTime.asSeconds()) % 60
                      << std::flush;

            // movement keys
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && player1.getPosition().x > gridX) {
                player1.move(-1.0f, 0.0f);
                shouldMove = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) &&
                player1.getPosition().x < gridX + gridWidth - player1.getSize().x) {
                player1.move(1.0f, 0.0f);
                shouldMove = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && player1.getPosition().y > gridY) {
                player1.move(0.0f, -1.0f);
                shouldMove = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) &&
                player1.getPosition().y < gridY + gridHeight - player1.getSize().y) {
                player1.move(0.0f, 1.0f);
                shouldMove = true;
            }

            // sending position to server
            float valueX = (player1.getPosition().x - gridX) / gridSize;
            float valueY = (player1.getPosition().y - gridY) / gridSize;
            sendPlayerMovement(fd, valueX,valueY);// to chyba powinno dzialac (nieprzetestowane bo nie potrafie zrobic zeby serwer wyslal dobre informacje do clienta zeby ten pomalowal sobie vector mapygry
            std::cout << "jebac mnie serio" << std::endl;
//            if(firstSendImportant == 0){
//                float valueX = (player1.getPosition().x - gridX) / gridSize;
//                float valueY = (player1.getPosition().y - gridY) / gridSize;
//                sendPlayerMovement(fd, valueX,valueY);
//                firstSendImportant = 1;
//            }
            PlayerInfo updatePlayer = readPlayerUpdate(fd);
            visited[(updatePlayer.x)][(updatePlayer.y)] = sf::Color::Cyan;

//            std::optional<PlayerInfo> updatePlayer = readPlayerUpdate(fd);// to rozpierdala gierke razem z czescia na serwerze
//            if(updatePlayer.has_value()) {
//                visited[std::floor(updatePlayer.value().x)][std::floor(updatePlayer.value().y)] = updatePlayer.value().color;
//            }
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

    close(fd);
    //receiveThread.join();  // Wait for the receive thread to finish

    return 0;
}
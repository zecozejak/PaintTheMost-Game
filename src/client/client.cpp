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

const int gridSize = 10;  //rozmiar siatki
const int windowWidth = 800;
const int windowHeight = 600;
const float squareSize = 10.0f;
const float outlineThickness = 1.0f;
bool timeExpired = false;
sf::Time remainingTime; // czas od serwera

//ustawienia biala plansza co bedzie zamalowywana
const int gridWidth = windowWidth / 2;
const int gridHeight = windowHeight / 2;
//pozycja planszy
const int gridX = windowWidth / 4;
const int gridY = windowHeight / 4;

std::vector<std::vector<sf::Color> > visited(gridWidth / gridSize, std::vector<sf::Color>(gridHeight / gridSize, sf::Color::White));

struct PlayerInfo {
    float x, y;  // Player position
    sf::Color color;
};

void receivePosAndCol(int socketFD) {
    // TODO: read kolor i pozycja startowa (lub numer gracza, obliczanie na tej podstawie pozycji)
}

void receiveGameState(int serverSocket) {
    // TODO: loop z updatem tablicy
    // TODO: otrzymywanie czasu z serwera
    while(1) {
        sf::Int32 receivedMilliseconds;
        ssize_t receivedSize = read(serverSocket, &receivedMilliseconds, sizeof(receivedMilliseconds));
        if (receivedSize == -1) {
            std::cerr << "Error receiving data" << std::endl;
        } else {
            sf::Time receivedTime = sf::milliseconds(receivedMilliseconds);

            std::cout << "Pozostały czas: " << receivedTime.asSeconds() << " sekund" << std::endl;
        }
    }
    // TODO: read informacje o zwycięzcy
}

void sendPlayerMovement(int socketFD, float x, float y) {
    PlayerInfo info = {x, y};
    write(socketFD, &info, 8);
}

void sendPlayerReadiness(int socketFD) {
    // TODO: informacja o gotowości
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
        perror("Connection failed");
        printf("Error code: %d\n", errno);
        close(fd);
        return -1;
    }
    std::cout << "Connected to the server." << std::endl;

    sf::RectangleShape player1(sf::Vector2f(squareSize, squareSize));
    sf::Color player1Color = sf::Color::Red; // TODO: zmienna z serwera
    player1.setFillColor(player1Color);
    player1.setOutlineColor(sf::Color::Black);
    player1.setOutlineThickness(outlineThickness);
    player1.setPosition(windowWidth / 3.0f, windowHeight / 2.0f); // TODO: informacja z serwera

    std::thread receiveThread(receiveGameState, fd);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        if (remainingTime <= sf::Time::Zero) {
            timeExpired = true;
            remainingTime = sf::Time::Zero;  //unikanie wartosci ujemnych
        }

        if (!timeExpired) { //jezeli czas sie nie skonczyl
            //sterowanie 1
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && player1.getPosition().x > gridX) {
                player1.move(-1.0f, 0.0f);
                sendPlayerMovement(fd, gridX, gridY); // to chyba powinno dzialac
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) &&
                player1.getPosition().x < gridX + gridWidth - player1.getSize().x) {
                player1.move(1.0f, 0.0f);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && player1.getPosition().y > gridY) {
                player1.move(0.0f, -1.0f);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) &&
                player1.getPosition().y < gridY + gridHeight - player1.getSize().y) {
                player1.move(0.0f, 1.0f);
            }
        }
        // Handle SFML events, player input, and draw the game
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
    receiveThread.join();  // Wait for the receive thread to finish

    return 0;
}
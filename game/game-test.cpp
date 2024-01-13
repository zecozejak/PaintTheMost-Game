//BOTH
#include <SFML/Graphics.hpp>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <time.h>
using namespace std;

//BOTH
const int gridSize = 10;  //rozmiar siatki
const int windowWidth = 800;
const int windowHeight = 600;
const float squareSize = 10.0f;  //rozmiar kwadracika/ikonki

//KLIENT
const float outlineThickness = 1.0f;  //grubosc obramowania kwardacika

//SERWER
const sf::Time roundTime = sf::seconds(90.0f); //czas gry

//SERWER
//ALE MUSI WYSYLAC DO KLIENTA ZAKTUALIZOWANY
//liczenie procentowego zamalowania planszy przez dany kolor
float calculatePercentage(const std::vector<std::vector<sf::Color> >& visited, const sf::Color& color) {
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

int main() {
    //KLIENT
    //okno wyskakujace
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Paint the most game");

    //KLIENT
    //WYSTARCZY TYLKO JEDEN AKAPIT (BO BEDZIE W KLIENCIE JEDEN GRACZ)
    //kwadrat/ikonka 1
    sf::RectangleShape player1(sf::Vector2f(squareSize, squareSize));
    sf::Color player1Color = sf::Color::Red;
    player1.setFillColor(player1Color);
    player1.setOutlineColor(sf::Color::Black);
    player1.setOutlineThickness(outlineThickness);
    player1.setPosition(windowWidth / 3.0f, windowHeight / 2.0f);

    //kwadrat/ikonka 2
    sf::RectangleShape player2(sf::Vector2f(squareSize, squareSize));
    sf::Color player2Color = sf::Color::Green;
    player2.setFillColor(player2Color);
    player2.setOutlineColor(sf::Color::Black);
    player2.setOutlineThickness(outlineThickness);
    player2.setPosition(2.0f * windowWidth / 3.0f, windowHeight / 2.0f);

    //kwadrat/ikonka 3
    sf::RectangleShape player3(sf::Vector2f(squareSize, squareSize));
    sf::Color player3Color = sf::Color::Blue;
    player3.setFillColor(player3Color);
    player3.setOutlineColor(sf::Color::Black);
    player3.setOutlineThickness(outlineThickness);
    player3.setPosition(windowWidth / 3.0f, 2.0f * windowHeight / 3.0f);

    //kwadrat/ikonka 4
    sf::RectangleShape player4(sf::Vector2f(squareSize, squareSize));
    sf::Color player4Color = sf::Color::Yellow;
    player4.setFillColor(player4Color);
    player4.setOutlineColor(sf::Color::Black);
    player4.setOutlineThickness(outlineThickness);
    player4.setPosition(2.0f * windowWidth / 3.0f, 2.0f * windowHeight / 3.0f);

    //BOTH
    //ustawienia biala plansza co bedzie zamalowywana
    const int gridWidth = windowWidth / 2;  
    const int gridHeight = windowHeight / 2;
    
    //BOTH
    //pozycja planszy
    const int gridX = windowWidth / 4;  
    const int gridY = windowHeight / 4; 

    //BOTH
    //siatka planszy
    std::vector<std::vector<sf::Color>> visited(gridWidth / gridSize, std::vector<sf::Color>(gridHeight / gridSize, sf::Color::White));

    //licznik czasu
    sf::Clock clock;
    bool timeExpired = false;
        //KLIENT
        //main petla
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }

            //SERWER
            //MUSI WYSYLAC INFO O CZASIE DO KLIENTA NA BIEZACO
            //obliczanie pozostalego czasu
            sf::Time elapsedTime = clock.getElapsedTime();
            sf::Time remainingTime = roundTime - elapsedTime;
            if (remainingTime <= sf::Time::Zero) {
                timeExpired = true;
                remainingTime = sf::Time::Zero;  //unikanie wartosci ujemnych
            }

            //KLIENT
            //DOSTAJE POZOSTALY CZAS OD SERWERA
            //wyswietlanie licznika w terminalu
            std::cout << "\rPozostaly czas: " << std::setfill('0') << std::setw(2) << static_cast<int>(remainingTime.asSeconds()) / 60 << ":"
                << std::setfill('0') << std::setw(2) << static_cast<int>(remainingTime.asSeconds()) % 60 << "  " << std::flush;

            //KLIENT
            //DOSTAJE WARTOSCI OD SERWERA 
            //wyswietlanie procentowego zamalowania przez dane kolory
            cout << " Czerwony " << std::setprecision(2) << calculatePercentage(visited, player1Color) << "%  ";
            cout << " Zielony " << std::setprecision(2) << calculatePercentage(visited, player2Color) << "%  ";
            cout << " Niebieski " << std::setprecision(2) << calculatePercentage(visited, player3Color) << "%  ";
            cout << " Zolty " << std::setprecision(2) << calculatePercentage(visited, player4Color) << "%  " << std::flush;

            //KLIENT
            //DOSTAJE INFO O TIMEEXPIRED OD SERWERA
            if (!timeExpired) { //jezeli czas sie nie skonczyl
                
                //KLIENT
                //WYSTRARCZY JEDEN AKAPIT (NA STEROWANIE STRZALKAMI NA KLAWIATURZE)
                //sterowanie 1
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && player1.getPosition().x > gridX) {
                    player1.move(-1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && player1.getPosition().x < gridX + gridWidth - player1.getSize().x) {
                    player1.move(1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && player1.getPosition().y > gridY) {
                    player1.move(0.0f, -1.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && player1.getPosition().y < gridY + gridHeight - player1.getSize().y) {
                    player1.move(0.0f, 1.0f);
                }

                //sterowanie 2
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && player2.getPosition().x > gridX) {
                    player2.move(-1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && player2.getPosition().x < gridX + gridWidth - player2.getSize().x) {
                    player2.move(1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && player2.getPosition().y > gridY) {
                    player2.move(0.0f, -1.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && player2.getPosition().y < gridY + gridHeight - player2.getSize().y) {
                    player2.move(0.0f, 1.0f);
                }

                //sterowanie 3
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1) && player3.getPosition().x > gridX) {
                    player3.move(-1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3) && player3.getPosition().x < gridX + gridWidth - player3.getSize().x) {
                    player3.move(1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2) && player3.getPosition().y > gridY) {
                    player3.move(0.0f, -1.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4) && player3.getPosition().y < gridY + gridHeight - player3.getSize().y) {
                    player3.move(0.0f, 1.0f);
                }

                //sterowanie 4
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::J) && player4.getPosition().x > gridX) {
                    player4.move(-1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::L) && player4.getPosition().x < gridX + gridWidth - player4.getSize().x) {
                    player4.move(1.0f, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::I) && player4.getPosition().y > gridY) {
                    player4.move(0.0f, -1.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::K) && player4.getPosition().y < gridY + gridHeight - player4.getSize().y) {
                    player4.move(0.0f, 1.0f);
                }

                //KLIENT
                //WYSYLA WSPOLRZEDNE X I Y DO SERWERA
                //WYSTARCZY JEDEN AKAPIT
                //oznacz odwiedzone pole
                int player1GridX = (player1.getPosition().x - gridX) / gridSize;
                int player1GridY = (player1.getPosition().y - gridY) / gridSize;

                int player2GridX = (player2.getPosition().x - gridX) / gridSize;
                int player2GridY = (player2.getPosition().y - gridY) / gridSize;

                int player3GridX = (player3.getPosition().x - gridX) / gridSize;
                int player3GridY = (player3.getPosition().y - gridY) / gridSize;

                int player4GridX = (player4.getPosition().x - gridX) / gridSize;
                int player4GridY = (player4.getPosition().y - gridY) / gridSize;

                //SERWER
                //AKTUALIZUJE TABLICE VISITED PO CZYM WYSYLA JA DO KLIENTA
                //ustalenie koloru na visited
                if (player1GridX >= 0 && player1GridX < visited.size() && player1GridY >= 0 && player1GridY < visited[0].size()) {
                    visited[player1GridX][player1GridY] = player1Color;
                }

                if (player2GridX >= 0 && player2GridX < visited.size() && player2GridY >= 0 && player2GridY < visited[0].size()) {
                    visited[player2GridX][player2GridY] = player2Color;
                }

                if (player3GridX >= 0 && player3GridX < visited.size() && player3GridY >= 0 && player3GridY < visited[0].size()) {
                    visited[player3GridX][player3GridY] = player3Color;
                }

                if (player4GridX >= 0 && player4GridX < visited.size() && player4GridY >= 0 && player4GridY < visited[0].size()) {
                    visited[player4GridX][player4GridY] = player4Color;
                }
            }

            //KLIENT
            window.clear();

            //KLIENT
            //RYSUJE NA PODSTAWIE TABLICY VISITED PRZESLANEJ PRZEZ SERWER
            //rysowanko 
            //plansza
            for (int i = 0; i < visited.size(); ++i) {
                for (int j = 0; j < visited[i].size(); ++j) {
                    sf::RectangleShape gridSquare(sf::Vector2f(gridSize, gridSize));
                    gridSquare.setPosition(gridX + i * gridSize, gridY + j * gridSize);
                    gridSquare.setFillColor(visited[i][j]);
                    window.draw(gridSquare);
                }
            }

            //KLIENT
            //WYSTARCZY JEDNA LINIJKA
            //playerzy
            window.draw(player1);
            window.draw(player2);
            window.draw(player3);
            window.draw(player4);

            //KLIENT
            //okno
            window.display();

            //KLIENT
            if (timeExpired) {
                cout << "\nKONIEC CZASU\n";
            }
        }
    }

    //KLIENT
    //anime dziefczymki na ending screen
    //sie doda potem

    return 0;
}

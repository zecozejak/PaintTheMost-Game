#include <SFML/Graphics.hpp>
#include <vector>
#include <sstream>

const int gridSize = 10;  //rozmiar siatki
const int windowWidth = 1000;
const int windowHeight = 800;
const float squareSize = 10.0f;  //rozmiar kwadracika/ikonki
const float outlineThickness = 1.0f;  //grubosc obramowania kwardacika

int main() {
    //okno wyskakujace
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Paint the most game");

    //kwadrat/ikonka
    sf::RectangleShape square(sf::Vector2f(squareSize, squareSize));
    square.setFillColor(sf::Color::Red);
    square.setOutlineColor(sf::Color::Black);  //Kolor obramowania
    square.setOutlineThickness(outlineThickness);  //Grubość obramowania
    square.setPosition(windowWidth / 2.0f, windowHeight / 2.0f);

    //ustawienia biala plansza co bedzie zamalowywana
    const int gridWidth = windowWidth / 2;  
    const int gridHeight = windowHeight / 2;
    //pozycja planszy
    const int gridX = windowWidth / 4;  
    const int gridY = windowHeight / 4; 

    //siatka planszy
    std::vector<std::vector<bool>> visited(gridWidth / gridSize, std::vector<bool>(gridHeight / gridSize, false));

    //slad
    sf::RectangleShape trail(sf::Vector2f(gridSize, gridSize));
    trail.setFillColor(sf::Color::Red);  

    //licznik czasu
    sf::Clock clock;

    //main petla
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        //sterowanie
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && square.getPosition().x > gridX) {
            square.move(-1.0f, 0.0f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && square.getPosition().x < gridX + gridWidth - square.getSize().x) {
            square.move(1.0f, 0.0f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && square.getPosition().y > gridY) {
            square.move(0.0f, -1.0f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && square.getPosition().y < gridY + gridHeight - square.getSize().y) {
            square.move(0.0f, 1.0f);
        }

        //oznacz odwiedzone pole
        int squareGridX = (square.getPosition().x - gridX) / gridSize;
        int squareGridY = (square.getPosition().y - gridY) / gridSize;

        if (!visited[squareGridX][squareGridY]) {
            visited[squareGridX][squareGridY] = true;
            //pozycja sladu
            trail.setPosition(gridX + squareGridX * gridSize, gridY + squareGridY * gridSize);
        }

        window.clear();

        //rysowanko 
        //plansza
        for (int i = 0; i < visited.size(); ++i) {
            for (int j = 0; j < visited[i].size(); ++j) {
                sf::RectangleShape gridSquare(sf::Vector2f(gridSize, gridSize));
                gridSquare.setPosition(gridX + i * gridSize, gridY + j * gridSize);
                if (visited[i][j]) {
                    gridSquare.setFillColor(sf::Color::Red);  //kolor odwiedzonego obszaru
                }
                else {
                    gridSquare.setFillColor(sf::Color::White);  //kolor nieodwiedzonego obszaru
                }
                window.draw(gridSquare);
            }
        }

        //slad
        window.draw(trail);

        //kwadracik/ikonka
        window.draw(square);

        //okno
        window.display();
    }

    return 0;
}

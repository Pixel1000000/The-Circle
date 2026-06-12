#include <SFML/Graphics.hpp>

#include <cmath>

int main()
{
    constexpr unsigned int windowWidth = 1280;
    constexpr unsigned int windowHeight = 720;

    sf::RenderWindow window(
        sf::VideoMode(windowWidth, windowHeight),
        "The Circle"
    );
    window.setFramerateLimit(60);

    sf::CircleShape player(24.0f);
    player.setFillColor(sf::Color(80, 220, 160));
    player.setOrigin(player.getRadius(), player.getRadius());
    player.setPosition(windowWidth * 0.5f, windowHeight * 0.5f);

    sf::Clock clock;

    while (window.isOpen())
    {
        sf::Event event{};
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        const float deltaTime = clock.restart().asSeconds();
        constexpr float playerSpeed = 280.0f;

        sf::Vector2f movement{0.0f, 0.0f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            movement.x -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            movement.x += 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        {
            movement.y -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        {
            movement.y += 1.0f;
        }

        if (movement.x != 0.0f || movement.y != 0.0f)
        {
            const float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
            movement /= length;
            player.move(movement * playerSpeed * deltaTime);
        }

        window.clear(sf::Color(18, 20, 24));
        window.draw(player);
        window.display();
    }

    return 0;
}

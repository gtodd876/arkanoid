#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <chrono>
#include <iostream>
#include <vector>

constexpr int window_width{800};
constexpr int window_height{600};
constexpr float ball_radius{10.f};
constexpr float ball_velocity{0.6f};
constexpr float paddle_width{60.f};
constexpr float paddle_height{20.f};
constexpr float paddle_velocity{0.8f};
constexpr float block_width{60.f};
constexpr float block_height{20.f};
constexpr int count_blocks_x{11};
constexpr int count_blocks_y{4};

constexpr float ft_step{1.f};
constexpr float ft_slice{1.f};

using FrameTime = float;
struct Rectangle
{
    sf::RectangleShape shape;
    float x() const noexcept { return shape.getPosition().x; }
    float y() const noexcept { return shape.getPosition().y; }
    float left() const noexcept { return shape.getPosition().x - shape.getSize().x / 2.f; }
    float right() const noexcept { return shape.getPosition().x + shape.getSize().x / 2.f; }
    float top() const noexcept { return shape.getPosition().y - shape.getSize().y / 2.f; }
    float bottom() const noexcept { return shape.getPosition().y + shape.getSize().y / 2.f; }
};

struct Brick : public Rectangle
{
    bool is_destroyed{false};
    Brick(const float m_x, const float m_y)
    {
        shape.setPosition(m_x, m_y);
        shape.setSize({block_width, block_height});
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin(block_width / 2.f, block_height / 2.f);
    }
};

struct Ball
{
    sf::CircleShape shape{};
    sf::Vector2f velocity{0.f, 0.f};
    bool reset{true};
    Ball(const float mX, const float mY)
    {
        shape.setPosition(mX, mY);
        shape.setRadius(ball_radius);
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(ball_radius, ball_radius);
    }
    void update(const FrameTime m_FT)
    {
        shape.move(velocity * m_FT);
        if (reset)
            velocity = {0.f, 0.f};
        else
        {
            if (left() < 0)
                velocity.x = ball_velocity;
            else if (right() > window_width)
                velocity.x = -ball_velocity;
            else if (top() < 0)
                velocity.y = ball_velocity;
            else if (bottom() > window_height)
                reset = true;
        }
    }

    float x() const noexcept { return shape.getPosition().x; }
    float y() const noexcept { return shape.getPosition().y; }
    float left() const noexcept { return shape.getPosition().x - shape.getRadius(); }
    float right() const noexcept { return shape.getPosition().x + shape.getRadius(); }
    float top() const noexcept { return shape.getPosition().y - shape.getRadius(); }
    float bottom() const noexcept { return shape.getPosition().y + shape.getRadius(); }
};

struct Paddle : public Rectangle
{
    sf::Vector2f velocity{};

    Paddle(const float mX, const float mY)
    {
        shape.setPosition(mX, mY);
        shape.setSize({paddle_width, paddle_height});
        shape.setFillColor(sf::Color::Blue);
        shape.setOrigin(paddle_width / 2.f, paddle_height / 2.f);
    }

    void update(const FrameTime m_FT)
    {
        shape.move(velocity * m_FT);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && left() > 0)
        {
            velocity.x = -paddle_velocity;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && right() < window_width)
        {
            velocity.x = paddle_velocity;
        }
        else
        {
            velocity.x = 0;
        }
    }
};

template<class T1, class T2>
bool isIntersecting(T1 &m_a, T2 &m_b)
{
    return m_a.right() >= m_b.left() && m_a.left() <= m_b.right() && m_a.bottom() >= m_b.top() &&
           m_a.top() <= m_b.bottom();
}

void testCollision(Paddle &m_paddle, Ball &m_ball)
{
    if (!isIntersecting(m_paddle, m_ball))
        return;
    m_ball.velocity.y = -ball_velocity;
    if (m_ball.x() < m_paddle.x())
        m_ball.velocity.x = -ball_velocity;
    else
    {
        m_ball.velocity.x = ball_velocity;
    }
}

void testCollision(Brick &m_brick, Ball &m_ball)
{
    if (!isIntersecting(m_brick, m_ball))
        return;

    m_brick.is_destroyed = true;

    const float overlap_left{m_ball.right() - m_brick.left()};
    const float overlap_right{m_brick.right() - m_ball.left()};
    const float overlap_top{m_ball.bottom() - m_brick.top()};
    const float overlap_bottom{m_brick.bottom() - m_ball.top()};

    const bool ballFromLeft(abs(overlap_left) < abs(overlap_right));
    const bool ballFromTop(abs(overlap_top) < abs(overlap_bottom));

    const float min_overlap_x{ballFromLeft ? overlap_left : overlap_right};
    // ReSharper disable once CppTooWideScopeInitStatement
    const float min_overlap_y{ballFromTop ? overlap_top : overlap_bottom};

    if (abs(min_overlap_x) < abs(min_overlap_y))
    {
        m_ball.velocity.x = ballFromLeft ? -ball_velocity : ball_velocity;
        m_ball.velocity.y = ballFromTop ? -ball_velocity : ball_velocity;
    }
}

struct Game
{
    sf::RenderWindow window{{window_width, window_height}, "Pipanoid"};
    FrameTime lastFT{0.f};
    FrameTime current_slice{0.f};
    sf::Font myFont;
    sf::Text score_text{};
    int score{0};
    Ball ball{static_cast<float>(window_width) / 2.f, static_cast<float>(window_height) - 100.f};
    Paddle paddle{static_cast<float>(window_width) / 2.f, static_cast<float>(window_height) - 50.f};
    std::vector<Brick> bricks{};

    Game()
    {
        if (!myFont.loadFromFile("fonts/JetBrainsMono-Regular.ttf"))
        {
            std::cerr << "Could not load font file" << '\n';
        }
        window.setFramerateLimit(60);
        for (int i = 0; i < count_blocks_x; ++i)
        {
            for (int j = 0; j < count_blocks_y; ++j)
            {
                bricks.emplace_back((static_cast<float>(i) + 1.f) * (block_width + 3) + 22,
                                    (static_cast<float>(j) + 2.f) * (block_height + 3));
            }
        }
    }
    void run()
    {
        while (window.isOpen())
        {
            // start our time interval
            auto time_point_1(std::chrono::high_resolution_clock::now());

            window.clear(sf::Color::Black);

            inputPhase();
            updatePhase();
            drawPhase();
            auto time_point_2(std::chrono::high_resolution_clock::now());
            auto time_elapsed(time_point_2 - time_point_1);
            const FrameTime ft{
                    std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(time_elapsed).count()};
            lastFT = ft;
            const auto ft_in_seconds(ft / 1000.f);
            const auto fps(1.f / ft_in_seconds);

            window.setTitle("FT: " + std::to_string(ft) + "\tPFS: " + std::to_string(fps));
        }
    }

    void inputPhase()
    {
        sf::Event event{};
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();
                if (event.key.code == sf::Keyboard::Space)
                {
                    ball.reset = false;
                }
            }
            if (event.type == sf::Event::Closed)
            {
                window.close();
                break;
            }
        }
    }

    void updatePhase()
    {
        current_slice += lastFT;
        for (; current_slice >= ft_slice; current_slice -= ft_slice)
        {
            ball.update(ft_step);
            paddle.update(ft_step);
            testCollision(paddle, ball);
            for (auto &brick: bricks)
            {
                testCollision(brick, ball);
            }
            bricks.erase(
                    std::ranges::remove_if(bricks, [](const Brick &m_brick) { return m_brick.is_destroyed; }).begin(),
                    std::end(bricks));
            score = 44 - static_cast<int>(bricks.size());
            if (ball.reset)
            {
                ball.velocity.x = 0;
                ball.shape.setPosition(paddle.x(), paddle.y() - 20.f);
                score = 0;
                bricks.clear();
                for (int i = 0; i < count_blocks_x; ++i)
                {
                    for (int j = 0; j < count_blocks_y; ++j)
                    {
                        bricks.emplace_back((static_cast<float>(i) + 1.f) * (block_width + 3) + 22,
                                            (static_cast<float>(j) + 2.f) * (block_height + 3));
                    }
                }
            }
            score_text = sf::Text("Score: " + std::to_string(score), myFont, 20);
            score_text.setFillColor(sf::Color::White);
            score_text.setPosition(window_width / 2.f, 15.f);
            score_text.setOrigin(score_text.getGlobalBounds().getSize().x / 2.f,
                                 score_text.getGlobalBounds().getSize().y / 2.f);
        }
    }
    void drawPhase()
    {
        window.draw(ball.shape);
        window.draw(paddle.shape);
        window.draw(score_text);
        for (auto &brick: bricks)
            window.draw(brick.shape);
        window.display();
    }
};


int main()
{
    Game{}.run();
    return 0;
}

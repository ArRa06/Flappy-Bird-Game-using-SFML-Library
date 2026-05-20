#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

constexpr unsigned WINDOW_WIDTH = 900;
constexpr unsigned WINDOW_HEIGHT = 640;
constexpr float GROUND_HEIGHT = 82.f;
constexpr float BIRD_X = 190.f;
constexpr float PI = 3.14159265359f;

enum class GameState
{
    Menu,
    Playing,
    Paused,
    GameOver
};

struct Difficulty
{
    std::string name;
    float pipeSpeed;
    float gapSize;
    float pipeSpacing;
    float verticalOscillation;
};

const std::array<Difficulty, 3> DIFFICULTIES{{
    {"Easy", 230.f, 210.f, 335.f, 0.f},
    {"Medium", 285.f, 170.f, 305.f, 28.f},
    {"Hard", 350.f, 135.f, 275.f, 55.f},
}};

fs::path findAsset(const std::string &filename)
{
    const std::array<fs::path, 5> candidates{
        fs::path("Assets") / filename,
        fs::path("../Assets") / filename,
        fs::path("../../Assets") / filename,
        fs::path("Assets_EndTermProj/Assets") / filename,
        fs::current_path() / "Assets" / filename,
    };

    for (const auto &path : candidates)
    {
        if (fs::exists(path))
        {
            return path;
        }
    }
    return fs::path("Assets") / filename;
}

std::optional<fs::path> findFont()
{
    const std::array<fs::path, 6> candidates{
        fs::path("Assets/arial.ttf"),
        fs::path("Assets/font.ttf"),
        fs::path("C:/Windows/Fonts/segoeui.ttf"),
        fs::path("C:/Windows/Fonts/arial.ttf"),
        fs::path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),
        fs::path("/System/Library/Fonts/Supplemental/Arial.ttf"),
    };

    for (const auto &path : candidates)
    {
        if (fs::exists(path))
        {
            return path;
        }
    }
    return std::nullopt;
}

class SoundManager
{
public:
    SoundManager()
    {
        flapBuffer = makeTone(620.f, 0.08f, 0.35f);
        scoreBuffer = makeTone(880.f, 0.12f, 0.28f);
        hitBuffer = makeTone(130.f, 0.22f, 0.45f);
        musicBuffer = makeMusicLoop();

        flap.setBuffer(flapBuffer);
        point.setBuffer(scoreBuffer);
        hit.setBuffer(hitBuffer);
        music.setBuffer(musicBuffer);
        music.setLoop(true);
        music.setVolume(13.f);
        flap.setVolume(45.f);
        point.setVolume(38.f);
        hit.setVolume(50.f);
    }

    void startMusic()
    {
        if (music.getStatus() != sf::Sound::Playing)
        {
            music.play();
        }
    }

    void stopMusic()
    {
        music.stop();
    }

    void playFlap() { flap.play(); }
    void playPoint() { point.play(); }
    void playHit() { hit.play(); }

private:
    static sf::SoundBuffer makeTone(float frequency, float seconds, float volume)
    {
        constexpr unsigned sampleRate = 44100;
        std::vector<sf::Int16> samples(static_cast<std::size_t>(sampleRate * seconds));
        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            const float envelope = 1.f - (static_cast<float>(i) / static_cast<float>(samples.size()));
            samples[i] = static_cast<sf::Int16>(std::sin(2.f * PI * frequency * t) * 30000.f * volume * envelope);
        }

        sf::SoundBuffer buffer;
        buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate);
        return buffer;
    }

    static sf::SoundBuffer makeMusicLoop()
    {
        constexpr unsigned sampleRate = 44100;
        constexpr float seconds = 2.4f;
        std::vector<sf::Int16> samples(static_cast<std::size_t>(sampleRate * seconds));
        const std::array<float, 4> notes{196.f, 246.94f, 293.66f, 246.94f};

        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            const auto noteIndex = static_cast<std::size_t>((t / seconds) * notes.size()) % notes.size();
            const float wave = std::sin(2.f * PI * notes[noteIndex] * t) + 0.35f * std::sin(2.f * PI * notes[noteIndex] * 2.f * t);
            samples[i] = static_cast<sf::Int16>(wave * 3200.f);
        }

        sf::SoundBuffer buffer;
        buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate);
        return buffer;
    }

    sf::SoundBuffer flapBuffer;
    sf::SoundBuffer scoreBuffer;
    sf::SoundBuffer hitBuffer;
    sf::SoundBuffer musicBuffer;
    sf::Sound flap;
    sf::Sound point;
    sf::Sound hit;
    sf::Sound music;
};

class Bird
{
public:
    explicit Bird(const sf::Texture &texture)
    {
        sprite.setTexture(texture);
        const auto bounds = sprite.getLocalBounds();
        sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        const float scale = 58.f / std::max(bounds.width, bounds.height);
        sprite.setScale(scale, scale);
        reset();
    }

    void reset()
    {
        position = {BIRD_X, WINDOW_HEIGHT * 0.46f};
        velocity = 0.f;
        sprite.setPosition(position);
        sprite.setRotation(0.f);
    }

    void flap()
    {
        velocity = -430.f;
    }

    void update(float dt)
    {
        velocity += 1120.f * dt;
        velocity = std::min(velocity, 720.f);
        position.y += velocity * dt;
        sprite.setPosition(position);
        sprite.setRotation(std::clamp(velocity * 0.065f, -25.f, 60.f));
    }

    void draw(sf::RenderWindow &window) const
    {
        window.draw(sprite);
    }

    sf::FloatRect collisionBounds() const
    {
        sf::FloatRect bounds = sprite.getGlobalBounds();
        bounds.left += bounds.width * 0.16f;
        bounds.top += bounds.height * 0.14f;
        bounds.width *= 0.68f;
        bounds.height *= 0.72f;
        return bounds;
    }

    float top() const { return collisionBounds().top; }
    float bottom() const
    {
        const auto bounds = collisionBounds();
        return bounds.top + bounds.height;
    }

private:
    sf::Sprite sprite;
    sf::Vector2f position;
    float velocity = 0.f;
};

class PipePair
{
public:
    PipePair(const sf::Texture &texture, float startX, float centerY, float gapSize, const Difficulty &difficulty)
        : gap(gapSize), baseCenter(centerY), oscillation(difficulty.verticalOscillation)
    {
        topPipe.setTexture(texture);
        bottomPipe.setTexture(texture);

        const auto bounds = topPipe.getLocalBounds();
        topPipe.setOrigin(bounds.width / 2.f, bounds.height);
        bottomPipe.setOrigin(bounds.width / 2.f, bounds.height);

        const float pipeWidth = 92.f;
        const float scaleX = pipeWidth / bounds.width;
        const float pipeHeight = 470.f;
        const float scaleY = pipeHeight / bounds.height;
        topPipe.setScale(scaleX, scaleY);
        bottomPipe.setScale(scaleX, -scaleY);

        x = startX;
        updateSprites(0.f);
    }

    void update(float dt, float speed, float elapsed)
    {
        x -= speed * dt;
        updateSprites(elapsed);
    }

    void draw(sf::RenderWindow &window) const
    {
        window.draw(topPipe);
        window.draw(bottomPipe);
    }

    bool isOffscreen() const
    {
        return x < -110.f;
    }

    bool passedBy(float birdX)
    {
        if (!scored && x + 46.f < birdX)
        {
            scored = true;
            return true;
        }
        return false;
    }

    bool collidesWith(const sf::FloatRect &birdBounds) const
    {
        return birdBounds.intersects(topPipe.getGlobalBounds()) || birdBounds.intersects(bottomPipe.getGlobalBounds());
    }

private:
    void updateSprites(float elapsed)
    {
        const float center = baseCenter + std::sin(elapsed * 2.2f + x * 0.015f) * oscillation;
        topPipe.setPosition(x, center - gap / 2.f);
        bottomPipe.setPosition(x, center + gap / 2.f);
    }

    sf::Sprite topPipe;
    sf::Sprite bottomPipe;
    float x = 0.f;
    float gap = 170.f;
    float baseCenter = 300.f;
    float oscillation = 0.f;
    bool scored = false;
};

class Game
{
public:
    Game()
        : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Flappy Bird Game", sf::Style::Titlebar | sf::Style::Close),
          bird(loadTexture(birdTexture, "Bird.png")),
          rng(std::random_device{}())
    {
        window.setFramerateLimit(120);
        pipeTexture.loadFromFile(findAsset("Pipes.png").string());

        if (const auto fontPath = findFont())
        {
            fontLoaded = font.loadFromFile(fontPath->string());
        }

        loadHighScore();
        resetWorld();
    }

    void run()
    {
        sf::Clock clock;
        while (window.isOpen())
        {
            const float dt = std::min(clock.restart().asSeconds(), 1.f / 30.f);
            handleEvents();
            update(dt);
            render();
        }
    }

private:
    static const sf::Texture &loadTexture(sf::Texture &texture, const std::string &filename)
    {
        if (!texture.loadFromFile(findAsset(filename).string()))
        {
            sf::Image fallback;
            fallback.create(64, 64, sf::Color::Yellow);
            texture.loadFromImage(fallback);
        }
        texture.setSmooth(true);
        return texture;
    }

    void handleEvents()
    {
        sf::Event event{};
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed)
            {
                const auto key = event.key.code;
                if (key == sf::Keyboard::Escape)
                {
                    window.close();
                }
                else if (state == GameState::Menu)
                {
                    handleMenuKey(key);
                }
                else if (state == GameState::Playing)
                {
                    if (key == sf::Keyboard::Space || key == sf::Keyboard::Up)
                    {
                        bird.flap();
                        sounds.playFlap();
                    }
                    else if (key == sf::Keyboard::P)
                    {
                        state = GameState::Paused;
                    }
                }
                else if (state == GameState::Paused)
                {
                    if (key == sf::Keyboard::P || key == sf::Keyboard::Space)
                    {
                        state = GameState::Playing;
                    }
                    else if (key == sf::Keyboard::R)
                    {
                        startGame();
                    }
                    else if (key == sf::Keyboard::M)
                    {
                        state = GameState::Menu;
                    }
                }
                else if (state == GameState::GameOver)
                {
                    if (key == sf::Keyboard::R || key == sf::Keyboard::Space)
                    {
                        startGame();
                    }
                    else if (key == sf::Keyboard::M)
                    {
                        state = GameState::Menu;
                        sounds.stopMusic();
                    }
                }
            }
        }
    }

    void handleMenuKey(sf::Keyboard::Key key)
    {
        if (key == sf::Keyboard::Num1 || key == sf::Keyboard::Numpad1)
        {
            selectedDifficulty = 0;
            startGame();
        }
        else if (key == sf::Keyboard::Num2 || key == sf::Keyboard::Numpad2)
        {
            selectedDifficulty = 1;
            startGame();
        }
        else if (key == sf::Keyboard::Num3 || key == sf::Keyboard::Numpad3)
        {
            selectedDifficulty = 2;
            startGame();
        }
        else if (key == sf::Keyboard::Left || key == sf::Keyboard::A)
        {
            selectedDifficulty = (selectedDifficulty + DIFFICULTIES.size() - 1) % DIFFICULTIES.size();
        }
        else if (key == sf::Keyboard::Right || key == sf::Keyboard::D)
        {
            selectedDifficulty = (selectedDifficulty + 1) % DIFFICULTIES.size();
        }
        else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space)
        {
            startGame();
        }
    }

    void resetWorld()
    {
        score = 0;
        elapsed = 0.f;
        bird.reset();
        pipes.clear();
        nextPipeX = WINDOW_WIDTH + 140.f;
        for (int i = 0; i < 4; ++i)
        {
            spawnPipe();
        }
    }

    void startGame()
    {
        resetWorld();
        state = GameState::Playing;
        sounds.startMusic();
    }

    void update(float dt)
    {
        if (state != GameState::Playing)
        {
            return;
        }

        elapsed += dt;
        const Difficulty &difficulty = DIFFICULTIES[selectedDifficulty];
        bird.update(dt);

        for (auto &pipe : pipes)
        {
            pipe.update(dt, difficulty.pipeSpeed, elapsed);
            if (pipe.passedBy(BIRD_X))
            {
                ++score;
                highScore = std::max(highScore, score);
                saveHighScore();
                sounds.playPoint();
            }
        }

        pipes.erase(std::remove_if(pipes.begin(), pipes.end(), [](const PipePair &pipe)
                                   { return pipe.isOffscreen(); }),
                    pipes.end());

        while (pipes.size() < 4)
        {
            spawnPipe();
        }

        checkCollisions();
    }

    void spawnPipe()
    {
        const Difficulty &difficulty = DIFFICULTIES[selectedDifficulty];
        std::uniform_real_distribution<float> gapDist(difficulty.gapSize * 0.88f, difficulty.gapSize * 1.12f);
        std::uniform_real_distribution<float> spacingDist(-34.f, 42.f);
        const float randomGap = gapDist(rng);
        // Pick the gap's top edge directly so openings clearly appear high, middle, or low.
        std::uniform_real_distribution<float> gapTopDist(70.f, WINDOW_HEIGHT - GROUND_HEIGHT - randomGap - 70.f);
        const float randomGapTop = gapTopDist(rng);
        const float randomCenter = randomGapTop + randomGap / 2.f;

        pipes.emplace_back(pipeTexture, nextPipeX, randomCenter, randomGap, difficulty);
        nextPipeX += difficulty.pipeSpacing + spacingDist(rng);
    }

    void checkCollisions()
    {
        const auto birdBounds = bird.collisionBounds();
        const bool hitBoundary = bird.top() < 0.f || bird.bottom() > WINDOW_HEIGHT - GROUND_HEIGHT;
        const bool hitPipe = std::any_of(pipes.begin(), pipes.end(), [&](const PipePair &pipe)
                                         { return pipe.collidesWith(birdBounds); });

        if (hitBoundary || hitPipe)
        {
            state = GameState::GameOver;
            sounds.playHit();
            sounds.stopMusic();
            saveHighScore();
        }
    }

    void render()
    {
        window.clear(sf::Color(95, 191, 232));
        drawBackground();

        for (const auto &pipe : pipes)
        {
            pipe.draw(window);
        }

        drawGround();
        bird.draw(window);
        drawHud();
        drawOverlay();
        window.display();
    }

    void drawBackground()
    {
        sf::RectangleShape sky({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
        sky.setFillColor(sf::Color(101, 195, 235));
        window.draw(sky);

        for (int i = 0; i < 5; ++i)
        {
            sf::CircleShape cloud(34.f + i * 3.f);
            cloud.setFillColor(sf::Color(245, 252, 255, 185));
            const float x = std::fmod(160.f + i * 210.f - elapsed * 18.f, WINDOW_WIDTH + 160.f) - 100.f;
            cloud.setPosition(x, 75.f + (i % 3) * 44.f);
            window.draw(cloud);
        }

        sf::ConvexShape hill;
        hill.setPointCount(3);
        hill.setPoint(0, {-80.f, WINDOW_HEIGHT - GROUND_HEIGHT});
        hill.setPoint(1, {260.f, 355.f});
        hill.setPoint(2, {590.f, WINDOW_HEIGHT - GROUND_HEIGHT});
        hill.setFillColor(sf::Color(84, 184, 119));
        window.draw(hill);

        hill.setPoint(0, {360.f, WINDOW_HEIGHT - GROUND_HEIGHT});
        hill.setPoint(1, {720.f, 338.f});
        hill.setPoint(2, {1040.f, WINDOW_HEIGHT - GROUND_HEIGHT});
        hill.setFillColor(sf::Color(65, 164, 106));
        window.draw(hill);
    }

    void drawGround()
    {
        sf::RectangleShape ground({static_cast<float>(WINDOW_WIDTH), GROUND_HEIGHT});
        ground.setPosition(0.f, WINDOW_HEIGHT - GROUND_HEIGHT);
        ground.setFillColor(sf::Color(222, 184, 103));
        window.draw(ground);

        sf::RectangleShape grass({static_cast<float>(WINDOW_WIDTH), 18.f});
        grass.setPosition(0.f, WINDOW_HEIGHT - GROUND_HEIGHT);
        grass.setFillColor(sf::Color(86, 190, 83));
        window.draw(grass);

        for (int i = 0; i < 24; ++i)
        {
            sf::RectangleShape stripe({24.f, 8.f});
            stripe.setPosition(static_cast<float>(i * 42), WINDOW_HEIGHT - 42.f);
            stripe.setFillColor(sf::Color(199, 154, 77));
            stripe.setRotation(-13.f);
            window.draw(stripe);
        }
    }

    void drawHud()
    {
        drawText("Score: " + std::to_string(score), 26, {24.f, 18.f}, sf::Color::White);
        drawText("Best: " + std::to_string(highScore), 22, {24.f, 52.f}, sf::Color(255, 246, 188));
        drawText(DIFFICULTIES[selectedDifficulty].name, 20, {WINDOW_WIDTH - 130.f, 20.f}, sf::Color::White);

        drawText("Regd No: 2341007072", 20, {20.f, WINDOW_HEIGHT - 35.f}, sf::Color::Black);
    }

    void drawOverlay()
    {
        if (state == GameState::Playing)
        {
            return;
        }

        sf::RectangleShape panel({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
        panel.setFillColor(sf::Color(0, 0, 0, state == GameState::Menu ? 95 : 135));
        window.draw(panel);

        if (state == GameState::Menu)
        {
            drawCentered("FLAPPY BIRD GAME", 54, 120.f, sf::Color(255, 242, 120));
            drawCentered("Choose difficulty with 1, 2, 3 or arrow keys", 24, 205.f, sf::Color::White);
            for (std::size_t i = 0; i < DIFFICULTIES.size(); ++i)
            {
                const bool selected = i == selectedDifficulty;
                const float y = 270.f + static_cast<float>(i) * 62.f;
                sf::RectangleShape option({330.f, 46.f});
                option.setOrigin(165.f, 23.f);
                option.setPosition(WINDOW_WIDTH / 2.f, y + 14.f);
                option.setFillColor(selected ? sf::Color(255, 213, 87, 230) : sf::Color(255, 255, 255, 65));
                option.setOutlineThickness(selected ? 4.f : 1.f);
                option.setOutlineColor(sf::Color::White);
                window.draw(option);
                drawCentered(std::to_string(i + 1) + " - " + DIFFICULTIES[i].name, 28, y, selected ? sf::Color(60, 42, 20) : sf::Color::White);
            }
            drawCentered("Space/Enter to start  |  Space or Up to flap  |  P to pause", 21, 515.f, sf::Color::White);
        }
        else if (state == GameState::Paused)
        {
            drawCentered("PAUSED", 58, 210.f, sf::Color(255, 242, 120));
            drawCentered("Press P or Space to resume", 27, 300.f, sf::Color::White);
            drawCentered("R restart  |  M menu  |  Esc quit", 22, 350.f, sf::Color(230, 240, 255));
        }
        else if (state == GameState::GameOver)
        {
            drawCentered("GAME OVER", 58, 190.f, sf::Color(255, 105, 86));
            drawCentered("Score: " + std::to_string(score) + "    Best: " + std::to_string(highScore), 30, 285.f, sf::Color::White);
            drawCentered("Press R or Space to restart", 25, 350.f, sf::Color(255, 242, 120));
            drawCentered("Press M for menu", 22, 392.f, sf::Color(230, 240, 255));
        }
    }

    void drawCentered(const std::string &text, unsigned size, float y, sf::Color color)
    {
        if (!fontLoaded)
        {
            return;
        }
        sf::Text drawable(text, font, size);
        drawable.setFillColor(color);
        drawable.setOutlineThickness(2.f);
        drawable.setOutlineColor(sf::Color(30, 55, 70, 180));
        const auto bounds = drawable.getLocalBounds();
        drawable.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
        drawable.setPosition(WINDOW_WIDTH / 2.f, y);
        window.draw(drawable);
    }

    void drawText(const std::string &text, unsigned size, sf::Vector2f position, sf::Color color)
    {
        if (!fontLoaded)
        {
            return;
        }
        sf::Text drawable(text, font, size);
        drawable.setPosition(position);
        drawable.setFillColor(color);
        drawable.setOutlineThickness(2.f);
        drawable.setOutlineColor(sf::Color(30, 55, 70, 170));
        window.draw(drawable);
    }

    void loadHighScore()
    {
        std::ifstream file("highscore.txt");
        if (file)
        {
            file >> highScore;
        }
    }

    void saveHighScore() const
    {
        std::ofstream file("highscore.txt", std::ios::trunc);
        file << highScore;
    }

    sf::RenderWindow window;
    sf::Texture birdTexture;
    sf::Texture pipeTexture;
    sf::Font font;
    bool fontLoaded = false;
    Bird bird;
    std::vector<PipePair> pipes;
    SoundManager sounds;
    std::mt19937 rng;
    GameState state = GameState::Menu;
    std::size_t selectedDifficulty = 1;
    int score = 0;
    int highScore = 0;
    float elapsed = 0.f;
    float nextPipeX = WINDOW_WIDTH + 120.f;
};

int main()
{
    Game game;
    game.run();
    return 0;
}

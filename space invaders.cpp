#include <SFML/Graphics.hpp>
#include <unordered_map>

sf::RenderWindow window(sf::VideoMode(800, 600), "(Space invaders)-like game");
std::unordered_map<std::string, sf::Texture> textures;

float deltaTime = (1.f / 120);

class Vector2f
{
public:
	Vector2f(float x, float y)
		:x(x), y(y)
	{ }

	Vector2f operator*(float a)
	{
		return Vector2f(x * a, y * a);
	}

	Vector2f operator+(Vector2f other)
	{
		return Vector2f(x + other.x, y + other.y);
	}

	Vector2f operator+=(Vector2f other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

public:
	float x, y;
};

enum class Direction
{
	UP,
	DOWN
};

class Bullet
{
#define BULLET_SPEED 200

public:
	Bullet(Vector2f position, Direction direction)
		:position_(position), direction_(direction)
	{
		if (direction == Direction::UP)
			sprite_.setTexture(textures["bullet_green"]);
		else
			sprite_.setTexture(textures["bullet_red"]);
	}

	void update()
	{
		// poruszanie się pocisku
		if (direction_ == Direction::DOWN)
			position_ += Vector2f(0, BULLET_SPEED) * deltaTime;
		else
			position_ += Vector2f(0, -BULLET_SPEED) * deltaTime;
	}

	sf::Sprite& getSprite()
	{
		return sprite_;
	}

	Vector2f getPosition()
	{
		return position_;
	}

	Direction getDirection()
	{
		return direction_;
	}

private:
	Vector2f position_;
	Direction direction_;
	sf::Sprite sprite_;
};
std::vector<Bullet> bullets;

class Spaceship
{
public:
	Spaceship(int hp, int speed, Vector2f position, float shootingSpeed)
		:hp_(hp), speed_(speed), shootingSpeed_(shootingSpeed), timeFromLastBullet_(0), position_(position)
	{ }

	virtual void update() = 0;

	sf::Sprite& getSprite()
	{
		return sprite_;
	}

	void setTexture(const std::string& texture)
	{
		sprite_.setTexture(textures[texture]);
	}

	Vector2f getPostion()
	{
		return position_;
	}

protected:
	int hp_;
	int speed_;
	float shootingSpeed_;
	float timeFromLastBullet_;
	Vector2f position_;
	sf::Sprite sprite_;
};

class Enemy : public Spaceship
{
public:
	Enemy(int hp, int speed, int startX, float shootingSpeed)
		:Spaceship(hp, speed, Vector2f(startX, -10), shootingSpeed)
	{ }

	void update() override
	{
		// poruszanie się statku
		position_ += Vector2f(speed_, 0) * deltaTime;

		// liczenie czasu od poprzedniego wystrzału i strzelenie jeśli upłynęło go wystarczająco dużo
		timeFromLastBullet_ += deltaTime;
		if (timeFromLastBullet_ >= shootingSpeed_)
		{
			timeFromLastBullet_ = 0;
			bullets.push_back(Bullet(position_, Direction::DOWN));
		}
	}
};
std::vector<Enemy> enemys;

class Player : public Spaceship
{
public:
	Player()
		:Spaceship(30, 200, Vector2f(400, 500), 0.8f)
	{ }

	void update() override
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			position_.x += speed_ * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			position_.x -= speed_ * deltaTime;

		timeFromLastBullet_ += deltaTime;
		if (timeFromLastBullet_ >= shootingSpeed_ && sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
		{
			timeFromLastBullet_ = 0;
			bullets.push_back(Bullet(position_, Direction::UP));
		}
	}
};
Player player;

#include<iostream>
void loadTexturesFromFiles()
{
	sf::Texture texture;
	texture.loadFromFile("img\\player.png");
	textures["player"] = texture;

	texture.loadFromFile("img\\bullet_green.png");
	textures["bullet_green"] = texture;

	texture.loadFromFile("img\\bullet_red.png");
	textures["bullet_red"] = texture;
}

void drawObject(sf::Sprite& sprite, Vector2f objectPosition)
{
	sf::Vector2u size = sprite.getTexture()->getSize();
	sprite.setPosition(sf::Vector2f(objectPosition.x - size.x / 2, objectPosition.y - size.y / 2));
	window.draw(sprite);
}

void updateBullets()
{
	for (int i = 0; i < bullets.size(); i++)
	{
		bullets[i].update();
		drawObject(bullets[i].getSprite(), bullets[i].getPosition());
	}
}

void updateEnemys()
{
	for (int i = 0; i < enemys.size(); i++)
	{
		enemys[i].update();
		drawObject(enemys[i].getSprite(), enemys[i].getPostion());
	}
}

void nextFrame()
{
	player.update();
	updateBullets();
	updateEnemys();
	drawObject(player.getSprite(), player.getPostion());
}

int main()
{
	loadTexturesFromFiles();
	player.setTexture("player");
	window.setFramerateLimit(120);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		window.clear();
		nextFrame();
		window.display();
	}

	return 0;
}
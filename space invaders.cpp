#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <queue>
#include <iostream>

sf::RenderWindow window(sf::VideoMode(1000, 700), "(Space invaders)-like game");
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
	
	Vector2f operator-(Vector2f other)
	{
		return Vector2f(x - other.x, y - other.y);
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
		:position_(position), direction_(direction), alive_(true)
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

	void setPosition(Vector2f position)
	{
		position_ = position;
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

	void kill()
	{
		alive_ = false;
	}

	bool isAlive()
	{
		return alive_;
	}

private:
	Vector2f position_;
	Direction direction_;
	sf::Sprite sprite_;
	bool alive_;
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

	void setPosition(Vector2f position)
	{
		position_ = position;
	}

	Vector2f getSize()
	{
		return Vector2f(getSprite().getTexture()->getSize().x, getSprite().getTexture()->getSize().x);
	}

	Vector2f getPostion()
	{
		return position_;
	}

	int getHp()
	{
		return hp_;
	}

	void takeDamage(int damage)
	{
		hp_ -= damage;
		if (hp_ <= 0)
			hp_ = 0;
	}

	void shoot(Direction direction)
	{
		auto size = getSize();
		auto bullet = Bullet(position_ + Vector2f(size.x, (direction == Direction::DOWN) ? size.y : -(float)size.y)*0.5, direction);
		auto bulletSize = bullet.getSprite().getTexture()->getSize();
		bullet.setPosition(bullet.getPosition() + Vector2f(-(float)bulletSize.x, 0)*0.5);
		bullets.push_back(bullet);
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
		position_ += Vector2f(0, speed_) * deltaTime;

		// liczenie czasu od poprzedniego wystrzału i strzelenie jeśli upłynęło go wystarczająco dużo
		timeFromLastBullet_ += deltaTime;
		if (timeFromLastBullet_ >= shootingSpeed_)
		{
			timeFromLastBullet_ = 0;
			shoot(Direction::DOWN);
		}
	}
};
std::vector<Enemy> enemys;

class Player: public Spaceship
{
public:
	Player()
		:Spaceship(30, 200, Vector2f(400, 620), 0.8f)
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
			shoot(Direction::UP);
		}
	}
};
Player player;

class EnemyBuilder
{
public:
	EnemyBuilder(int hp, int speed, float shootingSpeed, const std::string& texture)
		:hp_(hp), speed_(speed), shootingSpeed_(shootingSpeed), texture_(texture) 
	{ }
	
	Enemy create(int startX)
	{
		Enemy enemy = Enemy(hp_, speed_, startX, shootingSpeed_);
		enemy.setTexture(texture_);
		return enemy;
	}

private:
	int hp_;
	int speed_; 
	float shootingSpeed_;
	std::string texture_;
};
std::vector<EnemyBuilder> builders_;

void createEnemysBuilders()
{
	EnemyBuilder enemyNormal =   EnemyBuilder(30, 20, 3,   "enemy2");
	EnemyBuilder enemyFastShot = EnemyBuilder(30, 25, 1.5, "enemy4");
	EnemyBuilder enemyTank =     EnemyBuilder(60, 10, 4,   "enemy3");
	EnemyBuilder enemySpecial =  EnemyBuilder(45, 22, 2.5, "enemy1");
	EnemyBuilder enemyBoss =     EnemyBuilder(100, 8 , 3,   "enemy1");

	builders_ = { enemyNormal, enemyFastShot, enemyTank, enemySpecial, enemyBoss };
}

class LevelObjectInfo
{
public:
	LevelObjectInfo(EnemyBuilder& builder, int startX, float spawnTime)
		:builder(builder), startX(startX), spawnTime(spawnTime) 
	{ }
	
	EnemyBuilder& builder;
	int startX;
	float spawnTime;
};

class LevelManager
{
public:
	LevelManager()
		:objects_(), currentTime_(0) 
	{ }

	void addObject(LevelObjectInfo levelObjectInfo)
	{
		objects_.push(levelObjectInfo);
	}

	void clear()
	{
		while (objects_.empty() == false)
			objects_.pop();
		currentTime_ = 0;
	}

	void updateLevel()
	{
		currentTime_ += deltaTime;
		while (objects_.empty() == false && objects_.front().spawnTime <= currentTime_)
		{
			LevelObjectInfo info = objects_.front();
			Enemy enemy = info.builder.create(info.startX);
			enemy.setPosition(enemy.getPostion() - enemy.getSize()*0.5);
			enemys.push_back(enemy);
			objects_.pop();
		}
	}

private:
	std::queue<LevelObjectInfo> objects_;
	float currentTime_;
};

LevelManager levelManager;

bool areObjectsCollide(Spaceship& spaceship, Bullet& bullet)
{
	auto spaceshipSize = spaceship.getSize();
	auto bulletSize = bullet.getSprite().getTexture()->getSize();
	auto spaceshipPos = spaceship.getPostion();
	auto bulletPos = bullet.getPosition();

	return (spaceshipPos.x <= bulletPos.x + bulletSize.x && spaceshipPos.x + spaceshipSize.x >= bulletPos.x) &&
		(spaceshipPos.y <= bulletPos.y + bulletSize.y && spaceshipPos.y + spaceshipSize.y >= bulletPos.y);
}

void updateCollisions()
{
	// collisions with player
	for (auto& bullet : bullets)
	{
		if (bullet.getDirection() == Direction::DOWN && areObjectsCollide(player, bullet))
		{
			player.takeDamage(10);
			bullet.kill();
		}
	}

	// collisions with enemys
	for (auto& bullet : bullets)
	{
		for (auto& enemy : enemys)
		{
			if (bullet.getDirection() == Direction::UP && areObjectsCollide(enemy, bullet))
			{
				enemy.takeDamage(10);
				bullet.kill();
			}
		}
	}
}

#include<iostream>
void loadTexturesFromFiles()
{
	sf::Texture texture;
	texture.loadFromFile("img\\player.png");
	textures["player"] = texture;

	texture.loadFromFile("img\\green-bullet.png");
	textures["bullet_green"] = texture;

	texture.loadFromFile("img\\red-bullet.png");
	textures["bullet_red"] = texture;

	texture.loadFromFile("img\\enemy1.png");
	textures["enemy1"] = texture;

	texture.loadFromFile("img\\enemy2.png");
	textures["enemy2"] = texture;

	texture.loadFromFile("img\\enemy3.png");
	textures["enemy3"] = texture;

	texture.loadFromFile("img\\enemy4.png");
	textures["enemy4"] = texture;
}

void drawObject(sf::Sprite& sprite, Vector2f objectPosition)
{
	//sf::Vector2u size = sprite.getTexture()->getSize();
	sprite.setPosition(sf::Vector2f(objectPosition.x, objectPosition.y));
	window.draw(sprite);
}

void updateBullets()
{
	for (int i = 0; i < bullets.size(); i++)
	{
		if (!bullets[i].isAlive())
		{
			std::swap(bullets[i], bullets.back());
			bullets.pop_back();
			i--;
			continue;
		}
		bullets[i].update();
		drawObject(bullets[i].getSprite(), bullets[i].getPosition());
	}
}

void updateEnemys()
{
	for (int i = 0; i < enemys.size(); i++)
	{
		if (enemys[i].getHp() == 0)
		{
			std::swap(enemys[i], enemys.back());
			enemys.pop_back();
			i--;
			continue;
		}
		enemys[i].update();
		drawObject(enemys[i].getSprite(), enemys[i].getPostion());
	}
}

void updatePlayer()
{
	player.update();
	drawObject(player.getSprite(), player.getPostion());
}

void nextFrame()
{
	levelManager.updateLevel();
	updateCollisions();
	updatePlayer();
	updateBullets();
	updateEnemys();
}

void loadLevel1()
{
	levelManager.clear();
	/*levelManager.addObject(LevelObjectInfo(builders_[0], 200, 0));
	levelManager.addObject(LevelObjectInfo(builders_[1], 500, 5));
	levelManager.addObject(LevelObjectInfo(builders_[2], 150, 10));
	levelManager.addObject(LevelObjectInfo(builders_[3], 700, 15));*/

	levelManager.addObject(LevelObjectInfo(builders_[0], 100, 0));
	levelManager.addObject(LevelObjectInfo(builders_[0], 900, 0));
	
	levelManager.addObject(LevelObjectInfo(builders_[0], 250, 5));
	levelManager.addObject(LevelObjectInfo(builders_[0], 750, 5));
	
	levelManager.addObject(LevelObjectInfo(builders_[0], 400, 10));
	levelManager.addObject(LevelObjectInfo(builders_[0], 600, 10));

	levelManager.addObject(LevelObjectInfo(builders_[1], 500, 18));

	levelManager.addObject(LevelObjectInfo(builders_[2], 150, 25));
	levelManager.addObject(LevelObjectInfo(builders_[2], 850, 25));
}

int main()
{
	loadTexturesFromFiles();
	player.setTexture("player");
	createEnemysBuilders();
	window.setFramerateLimit(120);
	loadLevel1();

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

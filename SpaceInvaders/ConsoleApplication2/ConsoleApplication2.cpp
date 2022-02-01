#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
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

void drawObject(sf::Sprite& sprite, Vector2f objectPosition)
{
	//sf::Vector2u size = sprite.getTexture()->getSize();
	sprite.setPosition(sf::Vector2f(objectPosition.x, objectPosition.y));
	window.draw(sprite);
}

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
		:Spaceship(3, 200, Vector2f(400, 620), 0.8f)
	{ }

	void update() override
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && position_.x < 950)
			position_.x += speed_ * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && position_.x > 0)
			position_.x -= speed_ * deltaTime;

		timeFromLastBullet_ += deltaTime;
		if (timeFromLastBullet_ >= shootingSpeed_ && sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
		{
			timeFromLastBullet_ = 0;
			shoot(Direction::UP);
		}
	}

	void draw()
	{
		drawObject(getSprite(), getPostion());

		for (int i = 0; i < getHp(); i++)
			drawObject(hpSprite_, Vector2f(10 + 25 * i, 670));
	}

	void refillHp()
	{
		hp_ = 3;
	}

	void setHpTexture(const std::string& hpTexture)
	{
		hpSprite_.setTexture(textures[hpTexture]);
	}

private:
	sf::Sprite hpSprite_;
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
	EnemyBuilder enemyBoss =     EnemyBuilder(100, 8 , 3,   "enemy1-250");

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

bool isSpriteClicked(const sf::Sprite& sprite)
{
	sf::Mouse mouse;
	auto mousePosition = mouse.getPosition(window);
	auto spritePosition = sprite.getPosition();
	auto spriteSize = sprite.getTexture()->getSize();

	return ((mousePosition.x >= spritePosition.x && mousePosition.x <= spritePosition.x + spriteSize.x) &&
		(mousePosition.y >= spritePosition.y && mousePosition.y <= spritePosition.y + spriteSize.y));
}

class Button
{
public:
	Button(Vector2f position)
		:position_(position), sprite_(), isClicked_(false), prevClicked_(false)
	{ }

	void update()
	{
		sf::Mouse mouse;
		isClicked_ = isSpriteClicked(sprite_) && !mouse.isButtonPressed(sf::Mouse::Left) && prevClicked_;
		prevClicked_ = mouse.isButtonPressed(sf::Mouse::Left);
	}

	bool isClicked()
	{
		return isClicked_;
	}

	sf::Sprite& getSprite()
	{
		return sprite_;
	}

	void setTexture(const std::string& texture)
	{
		sprite_.setTexture(textures[texture]);
	}

	Vector2f getPosition()
	{
		return position_;
	}

private:

	Vector2f position_;
	sf::Sprite sprite_;
	bool isClicked_;
	bool prevClicked_;
};

enum class EMainMenuState
{
	START_MENU,
	LEVELS_MENU,
	GAME_OVER,
	LEVEL_PASSED,
	NO_MENU
};

void loadLevel1();
void loadLevel2();
void loadLevel3();

class MainMenu
{
public:
	MainMenu()
		:startButton_(Vector2f(350, 250)), exitButton_(Vector2f(350, 350)), level1Button_(Vector2f(350, 200)),
		level2Button_(Vector2f(350, 300)), level3Button_(Vector2f(350, 400)), exit2Button_(Vector2f(350, 500)), menuState_(EMainMenuState::NO_MENU)
	{ }

	void setButtonsTextures()
	{
		startButton_.setTexture("start_button");
		exitButton_.setTexture("exit_button");
		level1Button_.setTexture("level1_button");
		level2Button_.setTexture("level2_button");
		level3Button_.setTexture("level3_button");
		exit2Button_.setTexture("exit_button");
		levelPassed_.setTexture(textures["level_passed"]);
		gameOver_.setTexture(textures["game_over"]);
	}

	void update()
	{
		startButton_.update();
		exitButton_.update();
		level1Button_.update();
		level2Button_.update();
		level3Button_.update();
		exit2Button_.update();

		if (menuState_ == EMainMenuState::START_MENU)
		{
			if (startButton_.isClicked())
				menuState_ = EMainMenuState::LEVELS_MENU;
			if (exitButton_.isClicked())
				exit(0);
		}
		else if (menuState_ == EMainMenuState::LEVELS_MENU)
		{
			if (level1Button_.isClicked())
			{
				loadLevel1();
				menuState_ = EMainMenuState::NO_MENU;
			}
			else if (level2Button_.isClicked())
			{
				loadLevel2();
				menuState_ = EMainMenuState::NO_MENU;
			}
			else if (level3Button_.isClicked())
			{
				loadLevel3();
				menuState_ = EMainMenuState::NO_MENU;
			}
			else if (exit2Button_.isClicked())
			{
				menuState_ = EMainMenuState::START_MENU;
			}
		}
		else if (menuState_ == EMainMenuState::GAME_OVER || menuState_ == EMainMenuState::LEVEL_PASSED)
		{
			endSceeenTimer_ -= deltaTime;
			if (endSceeenTimer_ <= 0)
			{
				setMenuState(EMainMenuState::LEVELS_MENU);
			}
		}
	}

	void draw()
	{
		if (menuState_ == EMainMenuState::START_MENU)
		{
			drawObject(startButton_.getSprite(), startButton_.getPosition());
			drawObject(exitButton_.getSprite(), exitButton_.getPosition());
		}
		else if (menuState_ == EMainMenuState::LEVELS_MENU)
		{
			drawObject(level1Button_.getSprite(), level1Button_.getPosition());
			drawObject(level2Button_.getSprite(), level2Button_.getPosition());
			drawObject(level3Button_.getSprite(), level3Button_.getPosition());
			drawObject(exit2Button_.getSprite(), exit2Button_.getPosition());
		}
		else if (menuState_ == EMainMenuState::GAME_OVER)
		{
			drawObject(gameOver_, Vector2f(0, 0));
		}
		else if (menuState_ == EMainMenuState::LEVEL_PASSED)
		{
			drawObject(levelPassed_, Vector2f(0, 0));
		}
	}

	EMainMenuState getMenuState()
	{
		return menuState_;
	}

	void setMenuState(EMainMenuState type)
	{
		menuState_ = type;
		endSceeenTimer_ = 5;
	}

private:
	EMainMenuState menuState_;

	Button startButton_;
	Button exitButton_;

	Button level1Button_;
	Button level2Button_;
	Button level3Button_;
	Button exit2Button_;

	sf::Sprite gameOver_;
	sf::Sprite levelPassed_;

	float endSceeenTimer_;
};

MainMenu mainMenu;

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

		enemys.clear();
		bullets.clear();

		currentTime_ = 0;
	}

	void updateLevel()
	{
		if (player.getHp() <= 0)
		{
			mainMenu.setMenuState(EMainMenuState::GAME_OVER);
			player.refillHp();
		}
		else if (enemys.size() == 0 && objects_.size() == 0)
		{
			mainMenu.setMenuState(EMainMenuState::LEVEL_PASSED);
			player.refillHp();
		}

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
			player.takeDamage(1);
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

	texture.loadFromFile("img\\enemy1-250.png");
	textures["enemy1-250"] = texture;

	texture.loadFromFile("img\\enemy2.png");
	textures["enemy2"] = texture;

	texture.loadFromFile("img\\enemy3.png");
	textures["enemy3"] = texture;

	texture.loadFromFile("img\\enemy4.png");
	textures["enemy4"] = texture;

	texture.loadFromFile("img\\start.png");
	textures["start_button"] = texture;

	texture.loadFromFile("img\\exit.png");
	textures["exit_button"] = texture;

	texture.loadFromFile("img\\level1.png");
	textures["level1_button"] = texture;

	texture.loadFromFile("img\\level2.png");
	textures["level2_button"] = texture;

	texture.loadFromFile("img\\level3.png");
	textures["level3_button"] = texture;

	texture.loadFromFile("img\\level_passed.png"); //tmp
	textures["level_passed"] = texture;

	texture.loadFromFile("img\\game_over.png"); // tmp
	textures["game_over"] = texture;

	texture.loadFromFile("img\\heart.png");
	textures["heart"] = texture;

	texture.loadFromFile("img\\bg_fin.png");
	textures["bg"] = texture;
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
		if (enemys[i].getPostion().y >= 700)
		{
			player.takeDamage(1);
			std::swap(enemys[i], enemys.back());
			enemys.pop_back();
		}
	}
}

void updatePlayer()
{
	player.update();
	player.draw();
}

sf::SoundBuffer soundBuffer;
sf::Sound sound;
void updateBacgroundMusic()
{
	if (sound.getStatus() != sf::SoundSource::Status::Playing)
		sound.play();
}

sf::Sprite backgroundSprite;
void nextFrame()
{
	updateBacgroundMusic();
	window.draw(backgroundSprite);
	if (mainMenu.getMenuState() == EMainMenuState::NO_MENU)
	{
		levelManager.updateLevel();
		updateCollisions();
		updatePlayer();
		updateBullets();
		updateEnemys();
	}
	else
	{
		mainMenu.update();
		mainMenu.draw();
	}
}

void loadLevel1()
{
	levelManager.clear();
	levelManager.addObject(LevelObjectInfo(builders_[0], 200, 0));
	levelManager.addObject(LevelObjectInfo(builders_[0], 400, 0));
	levelManager.addObject(LevelObjectInfo(builders_[0], 600, 0));
	levelManager.addObject(LevelObjectInfo(builders_[0], 800, 0));

	levelManager.addObject(LevelObjectInfo(builders_[1], 300, 20));
	levelManager.addObject(LevelObjectInfo(builders_[1], 500, 20));
	levelManager.addObject(LevelObjectInfo(builders_[1], 700, 20));

	levelManager.addObject(LevelObjectInfo(builders_[2], 400, 35));
	levelManager.addObject(LevelObjectInfo(builders_[2], 600, 35));

	levelManager.addObject(LevelObjectInfo(builders_[3], 500, 50));
	levelManager.addObject(LevelObjectInfo(builders_[3], 500, 55));
	levelManager.addObject(LevelObjectInfo(builders_[4], 500, 60));
		
}

void loadLevel2()
{
	levelManager.clear();

	levelManager.addObject(LevelObjectInfo(builders_[0], 100, 0));
	levelManager.addObject(LevelObjectInfo(builders_[0], 900, 0));

	levelManager.addObject(LevelObjectInfo(builders_[0], 250, 5));
	levelManager.addObject(LevelObjectInfo(builders_[0], 750, 5));

	levelManager.addObject(LevelObjectInfo(builders_[0], 400, 10));
	levelManager.addObject(LevelObjectInfo(builders_[0], 600, 10));

	levelManager.addObject(LevelObjectInfo(builders_[1], 500, 18));

	levelManager.addObject(LevelObjectInfo(builders_[2], 150, 25));
	levelManager.addObject(LevelObjectInfo(builders_[2], 850, 25));

	levelManager.addObject(LevelObjectInfo(builders_[3], 500, 40));
	levelManager.addObject(LevelObjectInfo(builders_[3], 200, 45));
	levelManager.addObject(LevelObjectInfo(builders_[3], 800, 45));

	levelManager.addObject(LevelObjectInfo(builders_[3], 300, 70));
	levelManager.addObject(LevelObjectInfo(builders_[3], 700, 70));
	levelManager.addObject(LevelObjectInfo(builders_[3], 500, 75));

	levelManager.addObject(LevelObjectInfo(builders_[4], 500, 85));


}

void loadLevel3()
{
	levelManager.clear();
	for (int i = 0; i < 9; i++)
	{
		levelManager.addObject(LevelObjectInfo(builders_[0], (300 * i)%1000 + 50, i * 10));
		levelManager.addObject(LevelObjectInfo(builders_[0], 1000 - ((300 * i) % 1000) - 50, i*10));
	}

	levelManager.addObject(LevelObjectInfo(builders_[3], 100, 95));
	levelManager.addObject(LevelObjectInfo(builders_[3], 900, 95));
	levelManager.addObject(LevelObjectInfo(builders_[3], 300, 100));
	levelManager.addObject(LevelObjectInfo(builders_[3], 700, 100));

	for (int i = 0; i < 5; i++)
		levelManager.addObject(LevelObjectInfo(builders_[1], 150 * (i + 1) + 50, 120 +  i * 4));

	for (int i = 0; i < 5; i++)
		levelManager.addObject(LevelObjectInfo(builders_[1], 1000 - (150 * (i + 1) + 50), 140 + i * 4));

	levelManager.addObject(LevelObjectInfo(builders_[4], 500, 150));
}

int main()
{
	loadTexturesFromFiles();
	backgroundSprite.setTexture(textures["bg"]);
	soundBuffer.loadFromFile("music/muzyka.wav");
	sound.setBuffer(soundBuffer);
	player.setTexture("player");
	player.setHpTexture("heart");
	createEnemysBuilders();
	mainMenu.setButtonsTextures();
	window.setFramerateLimit(120);
	mainMenu.setMenuState(EMainMenuState::START_MENU);
	//loadLevel1();

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
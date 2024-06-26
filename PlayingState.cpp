#include "PlayingState.h"

#include "State.h"
#include "StateMachine.h"
class StateMachine;

#include "MoreInfo.h"

PlayingState::PlayingState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace },
	versionText(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 30, 25, ARIAL_FONT, VERSION_STATE, sf::Color(255, 255, 0)),
	scoreText(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0 + 30, 25, SPACEINVADERS_FONT, sf::Color(255, 255, 255)),
	ccText(SCREEN_WIDTH - 200, 10, 15, ARIAL_FONT, "Created By ShangXian Ruan", sf::Color(128, 128, 128)) {
	
	//Score and lives information
	std::ifstream ifs("Information/SpaceInvaders.txt");
	if (ifs.is_open()) {
		ifs >> playerScore; //Score
		ifs >> playerLives; //Lives
	}


	//Player information
	playerTexture.loadFromFile(PLAYER_T);
	pBulletTexture.loadFromFile(PLAYER_BULLET_T);
	player = std::make_unique<Player>(playerTexture, PLAYER_SPEED);
	player->setPosition(sf::Vector2<float>(SCREEN_WIDTH / 10, GROUND_HEIGHT));
	
	pBullet = std::make_unique<PlayerBullet>(pBulletTexture);
	pBullet->setPosition(sf::Vector2<float>(PLAYER_BULLET_ORIGIN, PLAYER_BULLET_ORIGIN));

	
	//Shield information
	shieldTexture.loadFromFile(SHIELD_T);

	for (size_t x = 0; x < shieldCount; x++) { shieldVector.emplace_back(new Shield(shieldTexture)); } //320
	shieldVector[0]->setPosition(sf::Vector2<float>(SCREEN_WIDTH / 2 - 450, SCREEN_HEIGHT - 200));
	shieldVector[1]->setPosition(sf::Vector2<float>(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 200));
	shieldVector[2]->setPosition(sf::Vector2<float>(SCREEN_WIDTH / 2 + 150, SCREEN_HEIGHT - 200));
	shieldVector[3]->setPosition(sf::Vector2<float>(SCREEN_WIDTH / 2 + 450, SCREEN_HEIGHT - 200));


	//UFO information
	ufoTexture.loadFromFile(UFO_T);

	ufo = std::make_unique<UFO>(ufoTexture, UFO_SPEED);
	ufo->setPosition(sf::Vector2<float>(SCREEN_WIDTH + 50, SKY_HEIGHT));

	ufoTimerSave = random[0].getInt(30, 60);


	//Sound information
	//0 = Invader Killed Effect
	//1 = Player Death Effect
}

void PlayingState::updateKeyboardInputs(sf::Keyboard::Key key, bool isPressed) {
	if (key == sf::Keyboard::Escape) { machine.run(machine.buildState<MainMenuState>(machine, window, true)); invaderManager.stopSounds(); }
	if (key == sf::Keyboard::Space)  { playerShooting = isPressed; }
}

void PlayingState::updateEvents() {
	while (window.pollEvent(sfEvent)) {
		switch (sfEvent.type) {
			case sf::Event::Closed:
				machine.quit();
				break;

			case sf::Event::KeyPressed:
				updateKeyboardInputs(sfEvent.key.code, true);
				player->keyboardInputs(sfEvent.key.code, true);
				break;

			case sf::Event::KeyReleased:
				updateKeyboardInputs(sfEvent.key.code, false);
				player->keyboardInputs(sfEvent.key.code, false);
				break;
		}
	}
}

void PlayingState::update() {
	fpsCounter.updateCounter();
	scoreText.updateOText("SCORE<1>\n\t\t", playerScore);
	
	player->updateBorderBounds();
	player->updatePlayer();
	player->updateLives(playerLives);
	pBullet->update(playerShooting, PLAYER_BULLET_SPEED, player->getX(), player->getY());

	
	invaderManager.update();

	//Collision detection
	auto PLAYER_TO_INVADER = invaderManager.invaderCollision(player->getGlobalBounds());
	if (PLAYER_TO_INVADER) {
		playerLives = 0;
	}

	//Player bullet with invader
	auto INVADER_TO_PBULLET = invaderManager.invaderCollision(pBullet->getGlobalBounds());
	if (INVADER_TO_PBULLET) {
		playerScore += INVADER_TO_PBULLET->returnPoints();
		INVADER_TO_PBULLET->setPosition(sf::Vector2<float>(INVADER_ORIGIN, INVADER_ORIGIN));
		resetPBulletPosition();
		sound[0].setSound(INVADER_KILLED_FX, 20, false);
	}
	
	//Player bullet with invader bullet
	auto IBULLET_TO_PBULLET = invaderManager.iBulletCollision(pBullet->getGlobalBounds());
	if (IBULLET_TO_PBULLET) {
		resetPBulletPosition();
		IBULLET_TO_PBULLET->setPosition(sf::Vector2<float>(INVADER_BULLET_ORIGIN, INVADER_BULLET_ORIGIN));
	}

	//Invader bullet collides with player
	auto IBULLET_TO_PLAYER = invaderManager.iBulletCollision(player->getGlobalBounds());
	if (IBULLET_TO_PLAYER) {
		IBULLET_TO_PLAYER->setPosition(sf::Vector2<float>(INVADER_BULLET_ORIGIN, INVADER_BULLET_ORIGIN));
		player->setPosition(sf::Vector2<float>(SCREEN_WIDTH / 10, GROUND_HEIGHT));
		playerLives--;
		sound[1].setSound(EXPLOSION_FX, 50, false);
	}

	
	//Shield information
	for (auto& shield : shieldVector) {
		//Collision
		auto IBULLET_TO_SHIELD = invaderManager.iBulletCollision(shield->getGlobalBounds());
		auto PBULLET_TO_SHIELD = shield->getGlobalBounds().intersects(pBullet->getGlobalBounds());

		if (PBULLET_TO_SHIELD || IBULLET_TO_SHIELD) {
			if (PBULLET_TO_SHIELD) {
				resetPBulletPosition();
				shield->shieldProtection();
			}

			//Collision with invader bullets
			if (IBULLET_TO_SHIELD) {
				IBULLET_TO_SHIELD->setPosition(sf::Vector2<float>(INVADER_BULLET_ORIGIN, INVADER_BULLET_ORIGIN));
				shield->shieldProtection();
			}


			//Checking for shield damage
			if (shield->shieldProtectionNumber() <= 0) { shield->setPosition(sf::Vector2<float>(SHIELD_ORIGIN, SHIELD_ORIGIN)); }
		}
	}
	
	//UFO Information
	ufo->update(ufoTimerSave);
	auto PBULLET_TO_UFO = pBullet->getGlobalBounds().intersects(ufo->getGlobalBounds());

	if (PBULLET_TO_UFO) {
		resetPBulletPosition();
		resetUFOPosition();
		ufo->ufoCollisionSound();

		auto randomPoints = random[1].getInt(1, 3);
		auto ufoPoints = 0;

		switch (randomPoints) {
			case 1: ufoPoints = 050; break;
			case 2: ufoPoints = 100; break;
			case 3: ufoPoints = 150; break;
		}
		playerScore += ufoPoints;
	}

	
	auto shieldEnd = std::remove_if(shieldVector.begin(), shieldVector.end(), [](std::unique_ptr<Shield> & shield) {
		return !shield->isOnScreen();
	});
	shieldVector.erase(shieldEnd, shieldVector.end());
	
	std::ofstream ofs("Information/SpaceInvaders.txt", std::ios::out | std::ios::trunc);

	//Win
	if (invaderManager.invaderVectorSize() == 0) {
		ofs << playerScore << std::endl; //Score
		ofs << playerLives << std::endl; //Lives

		invaderManager.stopSounds();
		machine.run(machine.buildState<PlayingState>(machine, window, true));
	}

	//Lose
	if (playerLives <= 0) {
		ofs << 0; //Score
		ofs << 0; //Lives

		invaderManager.stopSounds();
		machine.run(machine.buildState<MainMenuState>(machine, window, true));
	}
}

void PlayingState::render() {
	window.clear();

	//Render items
	fpsCounter.renderTo(window);
	versionText.renderTo(window);
	ccText.renderTo(window);

	player->extraRenderTo(window);
	pBullet->renderTo(window);

	invaderManager.renderTo(window);
	scoreText.renderTo(window);

	ufo->renderTo(window);

	for (auto& shield : shieldVector) { shield->renderTo(window); }

	window.display();
}


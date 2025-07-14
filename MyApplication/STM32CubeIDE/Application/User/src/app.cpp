/*
 * app.cpp
 */
#include "app.hpp"
#include <cmsis_os2.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stack_macros.h"
#include "main.h"
#include "cmsis_os2.h"

// Khởi tạo đối tượng trò chơi và các mảng đạn và kẻ địch
Game gameInstance;
Bullet shipBullet[MAX_BULLET];
Bullet enemyBullet[MAX_BULLET];
Enemy enemy[MAX_ENEMY];

// Các hằng số và biến dùng để quản lý tốc độ xuất hiện của kẻ địch
#define MAX_SPAWN_RATE 10000
int spawnRate = MAX_SPAWN_RATE;
int spawnSeed = 0;

int currentRound = 1;
bool isRoundTransition = false;
int enemyBulletSpeed = 5;
const int ENEMY_BULLET_SPEED_MAX = 100;

volatile bool isGameTaskTerminated = false;

// Biến để lưu trữ vị trí của tàu và các cờ để điều khiển trạng thái trò chơi
uint16_t sx, sy;
bool shouldEndGame;
bool shouldStopTask;

// Các hàm cập nhật vị trí của kẻ địch, đạn tàu và đạn kẻ địch
void updateEnemy(uint8_t dt);
void updateShipBullet(uint8_t dt);
void updateEnemyBullet(uint8_t dt);
void resetGameObjectsForNextRound();

// Biến lưu thời gian đã trôi qua
uint8_t dt = 0;

extern osMessageQueueId_t Queue5Handle;

// Nhiệm vụ của task game
void gameTask(void *argument) {
    shouldEndGame = false;
    shouldStopTask = false;
	int loopCount = 0;

	// Khởi tạo vị trí và trạng thái ban đầu cho đạn và kẻ địch
	for (int i = 0; i < MAX_BULLET; i++) {
		shipBullet[i].updateCoordinate(-50, -50);
		enemyBullet[i].updateCoordinate(-50, -50);
		shipBullet[i].updateVelocity(0, -5);
		enemyBullet[i].updateVelocity(0, 5);
		shipBullet[i].updateDisplayStatus(IS_HIDDEN);
		enemyBullet[i].updateDisplayStatus(IS_HIDDEN);
	}

	for (int i = 0; i < MAX_ENEMY; i++) {
		enemy[i].updateCoordinate(-50, -50);
		enemy[i].updateDisplayStatus(IS_HIDDEN);
	}

//	uint32_t previousTick = 0, currentTick = 0;
	for (;;) {
		loopCount++;
		// Kiểm tra nút PG13 mỗi vòng lặp để tăng tốc độ đạn quái khi nhấn
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
			if (enemyBulletSpeed < ENEMY_BULLET_SPEED_MAX-1) {
				enemyBulletSpeed=enemyBulletSpeed+2;
				osDelay(200); // Chống tăng liên tục khi giữ nút
			}
		}
		// Nếu cờ kết thúc trò chơi và dừng task được đặt, thoát khỏi vòng lặp
		if (shouldEndGame == true && shouldStopTask == true) {
			break;
		}

		// Nếu cờ kết thúc trò chơi đã được đặt, tiếp tục vòng lặp
		else if (shouldEndGame)
			continue;

		// Xác định thời gian đã trôi qua từ lần cập nhật trước đó
		if (loopCount >= 2) {
			dt = 1;
			loopCount = 0;
		} else {
			dt = 0;
		}
		/*  UPDATE POSITIONS */

		// Cập nhật vị trí tàu và các đối tượng trong trò chơi
		sx = gameInstance.ship.coordinateX;
		sy = gameInstance.ship.coordinateY;

		// update enemy
		updateEnemy(dt);

		// update ship position
		gameInstance.ship.update(dt);

		// update ship bullet position
		updateShipBullet(dt);

		// update enemy bullet position
		updateEnemyBullet(dt);

		/*  CHECK COLLISIONS */

		// check enemy and ship collision
		// Kiểm tra va chạm giữa các đối tượng
		// Kiểm tra va chạm giữa tàu và kẻ địch
		for (int i = 0; i < MAX_ENEMY; i++) {
			if (enemy[i].displayStatus != IS_SHOWN)
				continue;
			if (Entity::isCollide(enemy[i], gameInstance.ship)) {
				gameInstance.ship.updateShipHp(1);
				enemy[i].updateDisplayStatus(SHOULD_HIDE);
				gameInstance.ship.updateCoordinate(104, 260);
				break;
			}
		}

		// Kiểm tra va chạm giữa kẻ địch và đạn của tàu
		for (int i = 0; i < MAX_ENEMY; i++) {
			if (enemy[i].displayStatus != IS_SHOWN)
				continue;
			for (int j = 0; j < MAX_BULLET; j++) {
				if (shipBullet[j].displayStatus != IS_SHOWN)
					continue;
				if (Entity::isCollide(enemy[i], shipBullet[j])) {
					enemy[i].updateDisplayStatus(SHOULD_HIDE);
					shipBullet[j].updateDisplayStatus(SHOULD_HIDE);
					gameInstance.updateScore(5);
					HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
					osDelay(50);
					HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
					break;
				}
			}
		}

		

		// Kiểm tra va chạm giữa tàu và đạn của kẻ địch
		for (int i = 0; i < MAX_BULLET; i++) {
			if (enemyBullet[i].displayStatus != IS_SHOWN)
				continue;
			if (Entity::isCollide(enemyBullet[i], gameInstance.ship)) {
				gameInstance.ship.updateShipHp(1);
				enemyBullet[i].updateDisplayStatus(SHOULD_HIDE);
				gameInstance.ship.updateCoordinate(104, 260);
				break;
			}
		}

		// Nếu tàu hết máu, đặt cờ kết thúc trò chơi
		uint8_t msg;
		if (gameInstance.ship.lives <= 0) {
			shouldEndGame = true;
			while (!shouldStopTask) {
				msg = 1;
				osMessageQueuePut(Queue5Handle, &msg, 0, 0);
				osDelay(50); // Small delay to yield CPU
			}
			break;
		} 

//		else if (currentRound == 1 && gameInstance.score >= 30) {
//			shouldEndGame = true;
//			isRoundTransition = true;
//			// Gửi message đặc biệt để UI biết là chuyển round
//			uint8_t msg = 2;
//			osMessageQueuePut(Queue5Handle, &msg, 0, 0);
//					// Chờ UI gửi tín hiệu continue (ví dụ đặt shouldEndGame = false từ ngoài)
//			while (shouldEndGame && isRoundTransition) {
//				osDelay(10);  // Đợi đến khi người dùng nhấn “continue”
//			}
//
//			// Reset lại các đối tượng và tiếp tục vòng mới
//			currentRound = 2;
//			shouldEndGame = false;  // Cho phép game tiếp tục
//			// while (!shouldStopTask) {
//			// 	osDelay(10);
//			// }
//			// break;
//		}
		else {
			msg = '0';
			osMessageQueuePut(Queue5Handle, &msg, 0, 0);
		}
	}
	isGameTaskTerminated = true;
	return;

}

// Cập nhật vị trí của kẻ địch
void updateEnemy(uint8_t dt) {
	spawnRate -= dt;
	for (int i = 0; i < MAX_ENEMY; i++) {
		// Nếu kẻ địch đang hiển thị, tiếp tục cập nhật vị trí
		if (enemy[i].displayStatus != IS_HIDDEN) {
			enemy[i].update(dt);
			continue;
		}

		if (spawnRate > 0)
			continue;

		uint16_t ex = 0;
		uint16_t ey = 0;
		uint16_t vx;
		uint16_t vy = 0;

		// --- Use hardware RNG for randomness ---
		uint32_t randValue = 0;
		if (HAL_RNG_GenerateRandomNumber(&hrng, &randValue) != HAL_OK) {
			randValue = 0; // fallback if RNG fails
		}
		uint8_t randSpawn = randValue % 4; // 0..3
		uint8_t randSide = (randValue >> 8) & 0x01; // 0 or 1

		if (randSide) {
			ex = -32;
			vx = 1;
		} else {
			ex = 240;
			vx = -1;
		}

		switch (randSpawn) {
		case 0:
			ey = 42;
			break;
		case 1:
			ey = 80;
			break;
		case 2:
			ey = 120;
			break;
		case 3:
			ey = 160;
			break;
		default:
			ey = 42;
			break;
		}

		// Cập nhật thông tin của kẻ địch và hiển thị lên màn hình
		enemy[i].updateCoordinate(ex, ey);
		enemy[i].updateVelocity(vx, vy);
		enemy[i].updateDisplayStatus(SHOULD_SHOW);

		// Đặt lại spawnRate để đợi cho lần xuất hiện kế tiếp
		spawnRate = MAX_SPAWN_RATE;
	}
}

// Cập nhật vị trí của đạn từ tàu
void updateShipBullet(uint8_t dt) {
	// kiem tra tau con het dan chua
	bool shouldFire = gameInstance.ship.updateBullet(dt);
	for (int i = 0; i < MAX_BULLET; i++) {
		if (shipBullet[i].displayStatus != IS_HIDDEN) {
			shipBullet[i].update(dt);
			continue;
		}

		if (shouldFire) {
			shipBullet[i].updateCoordinate(sx, sy - 16);
			shipBullet[i].updateDisplayStatus(SHOULD_SHOW);
			shouldFire = false;
		}
	}
}

// Cập nhật vị trí của đạn từ kẻ địch

void updateEnemyBullet(uint8_t dt) {
	// Đặt tốc độ đạn enemy theo round
	int bulletSpeed = enemyBulletSpeed;
	if (currentRound == 2) bulletSpeed += 1;
	if (bulletSpeed > ENEMY_BULLET_SPEED_MAX) bulletSpeed = ENEMY_BULLET_SPEED_MAX;

	for (int j = 0; j < MAX_ENEMY; j++) {
		if (enemy[j].displayStatus != IS_SHOWN)
			continue;

		// kiem tra ke dich con dan khong
		if (enemy[j].updateBullet(dt) == false)
			continue;

		for (int i = 0; i < MAX_BULLET; i++) {
			if (enemyBullet[i].displayStatus != IS_HIDDEN)
				continue;

			// Tạo mới đạn từ kẻ địch và hiển thị lên màn hình
			enemyBullet[i].updateCoordinate(enemy[j].coordinateX, enemy[j].coordinateY + 16);
			enemyBullet[i].updateVelocity(0, bulletSpeed); // Sử dụng tốc độ mới
			enemyBullet[i].updateDisplayStatus(SHOULD_SHOW);
			break;
		}
	}

	// Cập nhật vị trí của các đạn từ kẻ địch đã xuất hiện
	for (int i = 0; i < MAX_BULLET; i++) {
		if (enemyBullet[i].displayStatus != IS_SHOWN)
			continue;
		enemyBullet[i].update(dt);
	}

}

// Đặt lại trạng thái của các đối tượng trong trò chơi cho vòng chơi tiếp theo
void resetGameObjectsForNextRound() {
    // Reset enemy
    for (int i = 0; i < MAX_ENEMY; i++) {
        enemy[i].updateCoordinate(-50, -50);
        enemy[i].updateDisplayStatus(IS_HIDDEN);
    }
    // Reset bullets
    for (int i = 0; i < MAX_BULLET; i++) {
        shipBullet[i].updateCoordinate(-50, -50);
        shipBullet[i].updateDisplayStatus(IS_HIDDEN);
        enemyBullet[i].updateCoordinate(-50, -50);
        enemyBullet[i].updateDisplayStatus(IS_HIDDEN);
    }
    // Reset tàu về vị trí ban đầu (giữ nguyên số mạng và điểm)
    gameInstance.ship.coordinateX = 100;
    gameInstance.ship.coordinateY = 200;
    gameInstance.ship.updateVelocityX(0);
    gameInstance.ship.updateVelocityY(0);
    // Đảm bảo các biến trạng thái cho phép tàu di chuyển lại bình thường
    // Nếu có biến nào khác kiểm soát input/di chuyển, reset tại đây
    // spawnRate = MAX_SPAWN_RATE;
    spawnRate = 0; // Cho phép spawn quái ngay lập tức khi vào round mới
    shouldEndGame = false;
    shouldStopTask = false;
    // isGameTaskTerminated = false;
}

/*
 * Bullet.h
 */

#ifndef APPLICATION_USER_SRC_BULLET_HPP_
#define APPLICATION_USER_SRC_BULLET_HPP_
#include "Entity.hpp"

const uint8_t MAX_BULLET = 10;

class Bullet: public Entity {
public:
	int baseX;        // Tọa độ X ban đầu (trung tâm sóng)
	float tick;       // Giá trị "thời gian" cho hàm sin
	bool waveMode;    // Có bật chế độ đạn lượn sóng không

	Bullet();
	const int MOVE_RATE = 400;
	virtual ~Bullet();
	bool update(uint8_t dt);
};

#endif /* APPLICATION_USER_SRC_BULLET_HPP_ */

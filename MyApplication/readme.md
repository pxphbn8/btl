# btl_Nhung

Trò chơi Space Invaders là trò chời dạng shoot-em-up, trong đó người chơi điều khiển nhân vật di chuyển để tránh hoả lực của kẻ thù và tiêu diệt các mục tiêu.  
Trò chơi sẽ được xây dựng trên kit STM32F429I-DISC1 kết hợp với thư viện đồ hoạ TouchGFX. Bộ 32F429IDISCOVERY tận dụng khả năng của các vi điều khiển hiệu suất cao STM32F429, cho phép người dùng dễ dàng phát triển các ứng dụng phong phú với giao diện người dùng đồ họa tiên tiến.


## GIỚI THIỆU

__Đề bài__: _SpaceInvaders._

__Sản phẩm:__
1. Tính năng: Giao diện đồ họa trực quan (LCD)
- Trò chơi hiển thị trên màn hình LCD cảm ứng hoặc TFT.
- Giao diện sử dụng đồ họa từ TouchGFX, giúp hiển thị sinh động tàu vũ trụ, đạn và kẻ địch.
- Dùng LTDC, DMA2D và SDRAM để tối ưu hiệu suất đồ họa.
2. Tính năng: Điều khiển bằng Joystick
- Người chơi điều khiển tàu vũ trụ sang trái/phải bằng joystick.
- Joystick kết nối qua ADC1 (PA0, PA1) để đọc tọa độ analog X/Y.
- Phản hồi nhanh, mượt mà và có xử lý hướng di chuyển qua FreeRTOS Queue.
3. Tính năng: Gameplay tương tác
- Người chơi có thể bắn đạn để tiêu diệt các tàu địch, hiển thị điểm số, mạng chơi, Game Over.
- Tàu địch di chuyển theo hàng lượn sóng, bắn trả (nếu mở rộng).
- Có thể nâng cấp thêm hiệu ứng âm thanh (buzzer).

- Ảnh chụp minh họa:  
[Ảnh minh họa](https://raw.githubusercontent.com/pxphbn8/btl_Nhung/main/images/anh-minh-hoa.jpg)
  

## TÁC GIẢ

- Tên nhóm:.6h50
- Thành viên trong nhóm
  |STT|Họ tên|MSSV|Công việc|
  |--:|--|--|--|
  |1|Nguyễn Huy Hoàng|20215581|Tìm hiểu lên ý tưởng, Thiết kế đồ họa bằng TouchGFX, Phát triển phần mềm điều khiển,Xử lý logic và hiển thị trò chơi, Xử lý logic còi, Viết báo cáo|
  |2|Nguyễn Duy Khương|20215602|Tìm hiểu lên ý tưởng, Tìm hiểu thu thập hình ảnh đồ họa, Thiết kế đồ họa bằng TouchGFX, Phát triển phần mềm điều khiển, Xử lý logic và hiển thị trò chơi, Viết báo cáo|
  |3|Vũ Đức Lương|20215614| Tìm hiểu lên ý tưởng, Thiết kế lắp đặt phần cứng, Xử lý logic Joystick, Phát triển phần mềm điều khiển, Viết báo cáo|


## MÔI TRƯỜNG HOẠT ĐỘNG

- Module CPU/dev kit.
STM32F429I-DISC1.
- Các kit, module được sử dụng:

  |Kit,module|Vai trò|
  |--|--|
  |STM32F429I-DISCO| Dev kit chính với LCD tích hợp|
  |LCD ILI9341| RGB565: Màn hình TFT LCD dùng giao tiếp RGB (có thể là màn tích hợp trên F429I-DISCO)|
  |Joystick| Điều khiển nhân vật, sử dụng qua ADC chân PA0/PA1|
  |TouchGFX| Giao diện đồ họa|
  |SDRAM ngoài| Giao tiếp qua FMC (do cấu hình có dùng FMC)|
  |I2C3| Có thể dùng cho cảm biến hoặc Audio codec|
  |DMA2D, LTDC| Xử lý đồ họa và truyền dữ liệu đến LCD|


## SO ĐỒ SCHEMATIC

|STM32F429|Module ngoại vi|
|--|--|
|PA1|VRX của Joystick|
|PA2|VRY củaJoystick|
|PG13|chân dương còi TMB12A05|
|GND|chân âm còi TMB12A05|




### TÍCH HỢP HỆ THỐNG

  Hệ thống phần cứng gồm 5 thành phần chính:
-	Kit STM32F429I-DISC1
-	Mini USB-cable 
- Joystick
-	Dây nối
-	buzzer

  Kiến trúc phần mềm gồm có 5 thành phần chính:
-	GameScreenView: Tiếp nhận dữ liệu từ người dung và hiển thị hình ảnh lên màn hình
-	inputTask: Nhận dữ liệu từ người dụng và gửi vào inputQueue để chờ xử lý
-	gameTask: Cập nhật thông tin các đối tượng xử lý logic trò chơi
-	inputQueue: Chứa dữ liệu được người dung gửi tới trò chơi thông qua các nút bấm
-	gameInstance: Đối tượng trò chơi của chương trình, chứa thông tin về trò chơi


### ĐẶC TẢ HÀM
```c
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Vòng lặp vô hạn để đọc giá trị từ joystick */
  uint32_t count1, count2, count3, count4;
  uint8_t x1, x2, x3, x4;
  uint32_t adc_values[2]; // Mảng lưu giá trị ADC cho trục X và Y

  for(;;)
  {
    // Bắt đầu chuyển đổi ADC
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
      Error_Handler();
    }
    // Đọc giá trị ADC cho cả hai kênh (trục X và Y)
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
      adc_values[0] = HAL_ADC_GetValue(&hadc1); // Trục X
    }
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
      adc_values[1] = HAL_ADC_GetValue(&hadc1); // Trục Y
    }
    HAL_ADC_Stop(&hadc1);

    // Xử lý trục X: phải (Queue1) và trái (Queue2)
    count1 = osMessageQueueGetCount(Queue1Handle);
    if (count1 < 1)
    {
      if (adc_values[0] < JOYSTICK_X_LEFT_THRESHOLD) { x1 = 'R'; } else { x1 = 'N'; } // Đẩy trái (ADC thấp) → Phải
      osMessageQueuePut(Queue1Handle, &x1, 0, 100);
    }

    count2 = osMessageQueueGetCount(Queue2Handle);
    if (count2 < 1)
    {
      if (adc_values[0] > JOYSTICK_X_RIGHT_THRESHOLD) { x2 = 'L'; } else { x2 = 'N'; } // Đẩy phải (ADC cao) → Trái
      osMessageQueuePut(Queue2Handle, &x2, 0, 100);
    }

    // Xử lý trục Y: lên (Queue3) và xuống (Queue4)
    count3 = osMessageQueueGetCount(Queue3Handle);
    if (count3 < 1)
    {
      if (adc_values[1] < JOYSTICK_Y_UP_THRESHOLD)
      {
        x3 = 'U'; // Joystick nghiêng lên
      }
      else
      {
        x3 = 'N'; // Không có hướng
      }
      osMessageQueuePut(Queue3Handle, &x3, 0, 100);
    }

    count4 = osMessageQueueGetCount(Queue4Handle);
    if (count4 < 1)
    {
      if (adc_values[1] > JOYSTICK_Y_DOWN_THRESHOLD)
      {
        x4 = 'D'; // Joystick nghiêng xuống
      }
      else
      {
        x4 = 'N'; // Không có hướng
      }
      osMessageQueuePut(Queue4Handle, &x4, 0, 100);
    }

    osDelay(100); // Đợi 100ms trước khi đọc lại
  }
  /* USER CODE END 5 */
}
  ```
Dữ liệu điều khiển được gửi vào các hàng đợi trong StartDefaultTask, sau đó được GameScreenView kiểm tra (poll) và lấy ra trong mỗi lượt cập nhật của game.

```c
void GameScreenView::setupScreen()
{
	GameScreenViewBase::setupScreen();
  osThreadTerminate(gameTaskHandle);
	const osThreadAttr_t gameTask_attributes = {
	  .name = "gameTask", 
	  .stack_size = 8192 * 2,  
	  .priority = (osPriority_t) osPriorityNormal, 
	};

	gameTaskHandle = osThreadNew(gameTask, NULL, &gameTask_attributes);
	shouldEndGame = false;
	shouldStopTask = false;
	shouldStopScreen = false;
}
```
gameTask sẽ dựa trên các dữ liệu có được từ gameInstance để xử lý logic và cập nhật lại thông tin của gameInstace cho phù hợp. GameScreenView sẽ liên tục polling dữ liệu từ gameInstance để cập nhật lại hiển thị của các đối tượng trên màn hình và hiển thị cho người chơi
  
### KẾT QUẢ
- Giao diện menu
![Giao diện menu](https://raw.githubusercontent.com/pxphbn8/btl_Nhung/main/images/anh-minh-hoa.jpg)
- Giao diện trò chơi
![Giao diện trò chơi](https://raw.githubusercontent.com/pxphbn8/btl_Nhung/main/images/Giao-dien-tro-choi.jpg)
- Giao diện kết thúc
![Giao diện trò chơi](https://raw.githubusercontent.com/pxphbn8/btl_Nhung/main/images/Giao-dien-ket-thuc.jpg)

-  video sản phẩm: 
 [Xem video demo tại đây](https://drive.google.com/file/d/1pMrIorqWK59BUyRV0bhFvDTd9gOwnE5x/view?usp=sharing)


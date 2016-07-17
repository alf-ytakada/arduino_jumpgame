//ジャンプして何処までも高く登っていくゲーム
#include <MGLCD.h>
// https://synapse.kyoto/hard/MGLCD_AQM1248A/page014.html

// MGLCD_SpiPin4の4つの引数SCK_PIN、MOSI_PIN、CS_PIN、およびDI_PINは、
// それぞれSCK信号(SCLK信号)、MOSI信号(SDI信号)、/CS信号、RS信号を、Arduinoのどのピンに接続するかを表しています。
#define SCK_PIN  13
#define MOSI_PIN 11
#define CS_PIN   10
#define DI_PIN    9
#define WAIT      0

#define SCREEN_W 140
#define SCREEN_H 48

MGLCD_AQM1248A_SoftwareSPI MGLCD(MGLCD_SpiPin4(SCK_PIN, MOSI_PIN, CS_PIN, DI_PIN),WAIT);


typedef enum tag_pos {
  pos_left  = 0,
  pos_center,
  pos_right  
} pos_t;

class JumpGame {
public:
  // 点数
  int score;
  // 現在乗っているボード
  pos_t current_position;
  // 現在の高さ
  int current_height;
  // ゲームオーバーフラグ
  bool game_over;
  // 降下量 ( kStepMax から 0まで )
  int step;

  // ボードの個数
  static const int kBoardCount = 7;
  // 降下量の最大値(この値の時が基準のボード位置となる。)
  static const int kStepMax = 10;

  // ボード
  pos_t boards[kBoardCount];

  JumpGame() {
    init();
  }

  void init() {
    score = 0;
    current_position  = pos_center;
    current_height    = 0;
    current_board_idx = 0;
    game_over         = false;
    step              = kStepMax;

    frames_till_step_down = 10;
    
    int i;
    int seed  = analogRead(0);
    Serial.println(seed);
    randomSeed(seed);
    for (i = 0 ; i < kBoardCount ; i++) {
      boards[i] = (pos_t)(random(0, 3));
    }
  }

  // ジャンプする
  void jump(pos_t new_pos) {
    if (is_jumping) return;
    if (current_board_idx == kBoardCount)  return;
    is_jumping    = true;
    new_height    = current_height  + 1;
    new_position  = new_pos;
    frames_after_jump = 0;
    // 加速度計算
    setVelocity(current_position, new_position);
  }

  // ジャンプ中の処理
  void jumping() {
    if (is_jumping == false)  return;
    
    // 移動処理
    frames_after_jump++;
    diff_lr += lr_velocity;
    diff_ud += ud_velocity;
    ud_velocity -= kGravityAccel;

    // 着地判定
    if (frames_after_jump == kJumpingFrame) {
      is_jumping  = false;
      // 次の台に乗るはず。
      if (boards[current_board_idx + 1] == new_position) {
        // 乗った
        current_height    = new_height;
        current_position  = new_position;
        current_board_idx++;
      }
      else {
        // 落ちた
        game_over = true;
      } 
    }
  }

  // 毎フレームの処理
  void move() {
    if (--frames_till_step_down == 0) {
      // TODO: ここの早さでLvを決める
      frames_till_step_down = 10;
      step--;
      Serial.println(String("Step down to ") + step);
      if (step == 0) {
        stepBoard();
        step  = kStepMax;
        current_board_idx--;
        if (current_board_idx < 0) {
          gameOver();
        }
      }
    }
  }

  // ゲームオーバー処理
  void gameOver() {
    Serial.println("game over");
    game_over = true;
  }
  
  // 現在の座標
  void currentDisplayPos(int *x, int *y) {
    if (is_jumping) {
      *x  = (int)diff_lr + base_lr(current_position);
      *y  = (int)diff_ud + base_height();
    }
    else {
      *x  = base_lr(current_position);
      *y  = base_height();
    }
    
  }

  //////////////////////////////////
  // private
private:
  // 現在乗っているボードのindex
  int current_board_idx;
  // ジャンプ中？
  bool is_jumping;
  // ジャンプ中：次の位置
  pos_t new_position;
  // ジャンプ中：次の高さ
  int new_height;
  // ジャンプ中：ジャンプ開始時点からの経過フレーム
  int frames_after_jump;
  // ジャンプ中：左右方向の速度
  float lr_velocity;
  // ジャンプ中：上下方向の速度
  float ud_velocity;
  // 現在の高さからの座標差分：上下
  float diff_lr;
  // 現在の高さからの座標差分：上下
  float diff_ud;
  // ステップダウンするまでの残りフレーム
  int frames_till_step_down;
  

  // 約7フレーム後に、+10の位置に辿り着く
  // ジャンプ時の初速
  const int kJumpVelocity = 8;
  // ジャンプ時の重力加速度
  const int kGravityAccel = 1;
  // 次の台にのるまでの時間(frame)
  const int kJumpingFrame  = 7;

  // ジャンプ開始時の速度設定
  void setVelocity(pos_t old_pos, pos_t new_pos) {
    // 左右
    if (old_pos < new_pos) {
      lr_velocity = ((float)SCREEN_H / kJumpingFrame) * (new_pos - old_pos);
    }
    // 上下
    ud_velocity = kJumpingFrame;
  }

  // 基準 lr
  int base_lr(pos_t pos) {
    if (pos == pos_left)    return SCREEN_H / 8;
    if (pos == pos_center)  return (SCREEN_H / 8) * 4;
    if (pos == pos_right)   return (SCREEN_H / 8) * 7;
  }

  // 基準 height
  int base_height() {
    return (SCREEN_W / 5) * 2;
  }

  // ボードを次に進める
  void stepBoard() {
    int i;
    for (i = 0 ; i < kBoardCount -1 ; i++) {
      boards[i] = boards[i + 1];
    }
    boards[kBoardCount -1] = (pos_t)random(0, 3);
  }

};


void updateScreen();


class Ball {
  public:
  // 位置
  int x,y;
  // 半径
  int r;
  Ball() : x((SCREEN_W - 5)/2), y((SCREEN_H - 5)/2), r(5) {
    
  }
  void move(int dx, int dy) volatile {
    int moved_x = x + dx;
    int moved_y = y + dy;
    x = this->restricted_pos(x + dx, 0, SCREEN_W);
    y = this->restricted_pos(y + dy, 0, SCREEN_H);
  }

  int restricted_pos(int new_pos, int min, int max) volatile {
    if (new_pos - r < 0) {
      return r;
    }
    else if (new_pos + r > SCREEN_W) {
      return SCREEN_W - r;
    }
    else {
      return new_pos;
    }    
  }
  
};

volatile Ball ball;
// 割り込み
void intLeft() {
  ball.move(-1, 0);
  updateScreen();
}

void intRight() {
  ball.move(1, 0);
  updateScreen();
}

void updateScreen(const JumpGame &g) {
  MGLCD.ClearScreen();

  // ボード描画
  int i;
  // 今回は縦向きに使うので、x座標が上、y座標が右, 左下が原点
  int stepH = SCREEN_W / 9;
  int step_down_diff  = (int)((g.step - g.kStepMax) / (float)g.kStepMax * stepH);
  int stepW = SCREEN_H / 4;
  
  for (i = 0 ; i < g.kBoardCount; i++) {
    pos_t pos = g.boards[i];
    int left  = i * stepH + step_down_diff + 10;
    int right = stepW * (0.5 + (int)pos);
    MGLCD.FillRect(left, right, left + 1, right + stepW);
    //Serial.println(String(i) + " : " + (int)pos);
  }
  
  //MGLCD.FillCircle(ball.x, ball.y, ball.r);
  //Serial.println(String("(x, y) = ") + ball.x + "," + ball.y);
}

// ボタンが押されたか？
int last_pressed_times[14] = {0};
bool pressing[14] = {false};
bool pressed(int pin) {
  int now = millis();
  int hl  = digitalRead(pin);
  if (hl == LOW) {
    if (pressing[pin] == false && now - last_pressed_times[pin] > 50) {
      // 50ms立っているならば押したとする
      pressing[pin] = true;
      last_pressed_times[pin] = now;
      return true;
    }
    else {
      last_pressed_times[pin] = now;
    }
  }
  else {
    pressing[pin] = false;
  }
  return false;
}


///////////////////////////////

JumpGame *game;
void setup() {
  MGLCD.Reset();
  MGLCD.print("Hello, World! FugaPiyo" + String("hohoho"));
  
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("start");

  game  = new JumpGame();
  
}

void loop() {
  if (pressed(3)) {
    game->jump(pos_right);
  }
  if (pressed(2)) {
    game->jump(pos_left);
  }

  game->move();
  updateScreen(*game);
  delay(100);
//MGLCD.Circle(60,23,20);
//delay(1000);
}




//ジャンプして何処までも高く登っていくゲーム
#include <MGLCD.h>
// https://synapse.kyoto/hard/MGLCD_AQM1248A/page014.html

// 縦のフォントは
// https://synapse.kyoto/hard/AQM1248A_shield/page001.html#index2_4
// からお借りしました。

#include "num0.h"
#include "num1.h"
#include "num2.h"
#include "num3.h"
#include "num4.h"
#include "num5.h"
#include "num6.h"
#include "num7.h"
#include "num8.h"
#include "num9.h"


// MGLCD_SpiPin4の4つの引数SCK_PIN、MOSI_PIN、CS_PIN、およびDI_PINは、
// それぞれSCK信号(SCLK信号)、MOSI信号(SDI信号)、/CS信号、RS信号を、Arduinoのどのピンに接続するかを表しています。
#define SCK_PIN   6
#define MOSI_PIN  5
#define CS_PIN    8
#define DI_PIN    7
#define WAIT      0

// 操作系
#define LEFT_PIN    9
#define CENTER_PIN  10
#define RIGHT_PIN   11

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
  // 現在乗っているボードのindex
  int current_board_idx;
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
    current_height    = 0;
    current_board_idx = 0;
    game_over         = false;
    step              = kStepMax;

    frames_till_step_down = kMaxFramesTillStepDown;
    current_frames_till_step_down = kMaxFramesTillStepDown;
    is_jumping        = false;
    
    int i;
    int seed  = analogRead(0);
    Serial.println(seed);
    randomSeed(seed);
    for (i = 0 ; i < kBoardCount ; i++) {
      boards[i] = (pos_t)(random(0, 3));
    }
    current_position  = boards[0];
  }

  // ジャンプする
  void jump(pos_t new_pos) {
    if (is_jumping) return;
    Serial.println(String("jump! pos = ") + (int)new_pos);
    
    if (current_board_idx == kBoardCount -1)  return;
    is_jumping    = true;
    new_height    = current_height  + 1;
    new_position  = new_pos;
    frames_after_jump = 0;
    diff_lr       = 0;
    diff_ud       = 0;
    // 加速度計算
    setVelocity(current_position, new_position);
  }

  // ジャンプ中の処理
  void jumping() {
    if (is_jumping == false)  return;
    
    // 移動処理
    frames_after_jump++;
    diff_lr     += lr_velocity;
    diff_ud     += ud_velocity;
    ud_velocity += kGravityAccel;

    // 着地判定
    if (frames_after_jump == kJumpingFrame) {
      is_jumping  = false;
      // 次の台に乗るはず。
      if (boards[current_board_idx + 1] == new_position) {
        // 乗った
        current_height    = new_height;
        current_position  = new_position;
        current_board_idx++;
        score++;

        // ※100になっても特にスピードは上がらない。
        if (current_height >= 100) {
          current_frames_till_step_down = kMaxFramesTillStepDown - 10;
        }
        else if (current_height >= 50) {
          current_frames_till_step_down = kMaxFramesTillStepDown - 9;
        }
        else if (current_height >= 30) {
          current_frames_till_step_down = kMaxFramesTillStepDown - 8;
        }
        else if (current_height >= 20) {
          current_frames_till_step_down = kMaxFramesTillStepDown - 7;
        }
        else if (current_height >= 10) {
          current_frames_till_step_down = kMaxFramesTillStepDown - 6;
        }
        else if (current_height >= 5) {
          current_frames_till_step_down = kMaxFramesTillStepDown - 4;
        }

        frames_till_step_down = current_frames_till_step_down;
        //Serial.println(String("current frames_till_step_down = ") + frames_till_step_down);
      }
      else {
        // 落ちた
        game_over = true;
      } 
    }
  }

  // 毎フレームの処理
  void move() {
    if (game_over)  return;
    this->jumping();
    if (--frames_till_step_down <= 0) {
      // TODO: ここの早さでLvを決める
      frames_till_step_down = current_frames_till_step_down;
      step--;
      //Serial.println(String("Step down to ") + step);
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
  // ※ゲーム盤面上の位置を示す
  const void currentBoardPos(float *x, float *y) const {
    if (is_jumping) {
      *x  = diff_lr + base_lr(current_position);
      *y  = diff_ud + base_height() + current_board_idx;
    }
    else {
      *x  = (float)base_lr(current_position);
      *y  = (float)base_height() + current_board_idx;
    }
    
  }

  //////////////////////////////////
  // private
private:
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
  // 現在の高さからの座標差分：左右
  float diff_lr;
  // 現在の高さからの座標差分：上下
  float diff_ud;
  // ステップダウンするまでの残りフレーム
  int frames_till_step_down;
  // 現在の１ステップ進むのにかかるフレーム数
  int current_frames_till_step_down;
  

  ///////////////// 定数 //////////////////
  // 7フレーム後に、+1の位置に辿り着く
  // ジャンプ時の初速
  const float kJumpVelocity = 0.4928;
  // ジャンプ時の重力加速度
  const float kGravityAccel = -0.1;
  // 次の台にのるまでの時間(frame)
  const int kJumpingFrame  = 7;
  // 1 step ダウンするまでの基準フレーム数。レベルに関係する
  const int kMaxFramesTillStepDown  = 10;
  

  // ジャンプ開始時の速度設定
  void setVelocity(pos_t old_pos, pos_t new_pos) {
    // 左右
    lr_velocity = ((int)new_pos - (int)old_pos) / (float)kJumpingFrame;
    
    // 上下
    ud_velocity = kJumpVelocity;
  }

  // 基準 lr
  int base_lr(pos_t pos) const {
    return (int)pos;
  }

  // 基準 height
  int base_height() const {
    return 0;
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

void drawScore(int x, int y, int score);
void updateScreen();

///////////////
// スクリーン描画
// スコア描画
void drawScore(int x, int y, int score) {
  PROGMEM const unsigned char *map[]  = {
    BMP_num0, BMP_num1, BMP_num2, BMP_num3, BMP_num4,
    BMP_num5, BMP_num6, BMP_num7, BMP_num8, BMP_num9,
  };
  int num;
  String s_score(score);
  const char *c_score = s_score.c_str();
  int i;
  int diff  = 0;
  for (i = 0 ; i < s_score.length() ; i++) {
    //Serial.println(String(c_score) + ", index = " + (c_score[i] - 0x30) + ", x = " + x + ", y = " + (y + diff));
    MGLCD.DrawBitmap(map[c_score[i] - 0x30], x, y + diff, MGLCD_ROM);
    diff  += 6;
  }
}

// 画面全体描画
void updateScreen(const JumpGame &g) {
  MGLCD.ClearScreen();

  //////////////////////////////
  // ボード描画
  int i;
  // 今回は縦向きに使うので、x座標が上、y座標が右, 左下が原点
  int stepH = SCREEN_W / 9;
  int step_down_diff  = (int)((g.step - g.kStepMax) / (float)g.kStepMax * stepH);
  int stepW = SCREEN_H / 4;
  
  for (i = 0 ; i < g.kBoardCount; i++) {
    pos_t pos = g.boards[i];
    int height  = i * stepH + step_down_diff + 10;
    int left    = stepW * (0.5 + (int)pos);
    MGLCD.FillRect(height, left, height + 1, left + stepW);
    //Serial.println(String(i) + " : " + (int)pos);
  }
  
  //////////////////////////////
  // 自キャラ描画
  float char_board_pos_left;
  float char_board_pos_height;
  g.currentBoardPos(&char_board_pos_left, &char_board_pos_height);
  //Serial.println(String("char (x,y) = ") + char_board_pos_left + ", " + char_board_pos_height);
  
  int height  = char_board_pos_height * stepH + step_down_diff + 10;
  int left    = stepW * (1 + char_board_pos_left);
  int r       = 3;
  MGLCD.Circle(height + r + 1, left + r/2, r);
  
  //////////////////////////////
  // Score
  drawScore(SCREEN_W - 20, 2, g.score);
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
///////////////////////////////

JumpGame *game;
void setup() {
  MGLCD.Reset();
  MGLCD.print("Hello, World! FugaPiyo" + String("hohoho"));
  
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(CENTER_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("start");

  game  = new JumpGame();

}

void loop() {
  int allPushed  = 0;
  if (pressed(LEFT_PIN)) {
    game->jump(pos_left);
    allPushed++;
  }
  if (pressed(CENTER_PIN)) {
    game->jump(pos_center);
    allPushed++;
  }
  if (pressed(RIGHT_PIN)) {
    game->jump(pos_right);
    allPushed++;
  }

  game->move();
  // 全ボタン押されたらリセット
  if (allPushed == 3) {
    game->init();
  }
  updateScreen(*game);
  delay(20);
}




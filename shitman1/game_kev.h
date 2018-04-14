
typedef struct Game_KeyEvent {
    Game_KeyCode                code;
    Game_KeyState               state;
} Game_KeyEvent;

extern Game_KeyEvent            Game_KeyQueue[Game_KeyEventQueueMax];
extern unsigned int             Game_KeyQueue_In,Game_KeyQueue_Out;

Game_KeyEvent *Game_KeyEvent_Peek(void);
Game_KeyEvent *Game_KeyEvent_Get(void);
Game_KeyEvent *Game_KeyEvent_Add(void);
void Game_FlushKeyboardQueue(void);


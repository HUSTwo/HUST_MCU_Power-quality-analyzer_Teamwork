#ifndef __UI_H
#define __UI_H

#include <stdint.h>
#include <stdbool.h>

/*
 *    以轮询方式进行扫描的按键库，
 *    只需配置和扫描两步即可完成，
 *     可以在每个按钮上注册回调函数，按下即触发
 */

//显示器使用引脚PA5 PB3 PB4 PB7



//按键状态枚举
typedef enum{
    NORMAL = 0,//正常
    PRESS
}uiKeyState_Enum;


//按键极性枚举
typedef enum{
    LOW = 0,//按下时为低电平
    HIGH
}uiKeyPolarity_Enum;


//按键结构体
struct KeyType{
    uint32_t port;//端口基地址
    uint8_t num;//引脚编号
    uiKeyPolarity_Enum polarity;//极性
    uiKeyState_Enum state;//按键状态
    uint8_t sample[4];//采样
    void (*callback)(void);//按下时的回调函数
    struct KeyType * next;//链表结构，指向下一个按键的索引
};
//按键类型重命名
typedef struct KeyType uiKey_TypeDef;



//显示器字体枚举
typedef enum{
    FONT1206 = 12,//一个字符的像素占12字节
    FONT1608 = 16,
    FONT2412 = 36
}uiFont_Enum;

//数据显示框结构体
struct DisplayType{
    float *data;
    char lable[16];
    uint8_t x;
    uint8_t y;
    uint8_t length;
    struct DisplayType *next;
};
typedef struct DisplayType uiDisplayer_TypeDef;


/*按键*/
void uiKeyInit(uiKey_TypeDef* key, uint32_t Port, uint8_t PinNumber, uiKeyPolarity_Enum Polarity, void(*callbackfuncion)(void) );
void uiKeysRefresh(void);

/*显示器*/
void uiDisplayInit(void);
void uiDisplayClear(void);//清空显示器(立即生效)
void uiGRAMClear(void);//清空显存(下次刷新时生效)
void uiDisplayDrawPoint(int x, int y);
void uiDisplayErasePoint(int x, int y);
void uiDisplayDrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayDrawFrame(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayDrawBlock(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayEraseBlock(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayDrawCircle(uint8_t x0 , uint8_t y0 , uint8_t radius );
void uiDisplayShowChar(uint8_t x , uint8_t y , uiFont_Enum font , char ascii);
void uiDisplayShowString(uint8_t x , uint8_t y , uiFont_Enum font , char* str);
void uiDisplayerRefresh(void);//显示器刷新，每一定时间执行一次即可，把显存同步到屏幕上

/*数据显示框*/
void uiDisplayerInit(uiKey_TypeDef* displayer, const char *lable, float *data, uint8_t x, uint8_t y, uint8_t length);

#endif

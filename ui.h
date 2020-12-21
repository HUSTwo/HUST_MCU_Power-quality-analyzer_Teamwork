#ifndef __UI_H
#define __UI_H

#include <stdint.h>
#include <stdbool.h>

/*
 *    ����ѯ��ʽ����ɨ��İ����⣬
 *    ֻ�����ú�ɨ������������ɣ�
 *     ������ÿ����ť��ע��ص����������¼�����
 */

//��ʾ��ʹ������PA5 PB3 PB4 PB7



//����״̬ö��
typedef enum{
    NORMAL = 0,//����
    PRESS
}uiKeyState_Enum;


//��������ö��
typedef enum{
    LOW = 0,//����ʱΪ�͵�ƽ
    HIGH
}uiKeyPolarity_Enum;


//�����ṹ��
struct KeyType{
    uint32_t port;//�˿ڻ���ַ
    uint8_t num;//���ű��
    uiKeyPolarity_Enum polarity;//����
    uiKeyState_Enum state;//����״̬
    uint8_t sample[4];//����
    void (*callback)(void);//����ʱ�Ļص�����
    struct KeyType * next;//����ṹ��ָ����һ������������
};
//��������������
typedef struct KeyType uiKey_TypeDef;



//��ʾ������ö��
typedef enum{
    FONT1206 = 12,//һ���ַ�������ռ12�ֽ�
    FONT1608 = 16,
    FONT2412 = 36
}uiFont_Enum;

//������ʾ��ṹ��
struct DisplayType{
    float *data;
    char lable[16];
    uint8_t x;
    uint8_t y;
    uint8_t length;
    struct DisplayType *next;
};
typedef struct DisplayType uiDisplayer_TypeDef;


/*����*/
void uiKeyInit(uiKey_TypeDef* key, uint32_t Port, uint8_t PinNumber, uiKeyPolarity_Enum Polarity, void(*callbackfuncion)(void) );
void uiKeysRefresh(void);

/*��ʾ��*/
void uiDisplayInit(void);
void uiDisplayClear(void);//�����ʾ��(������Ч)
void uiGRAMClear(void);//����Դ�(�´�ˢ��ʱ��Ч)
void uiDisplayDrawPoint(int x, int y);
void uiDisplayErasePoint(int x, int y);
void uiDisplayDrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayDrawFrame(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayDrawBlock(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayEraseBlock(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void uiDisplayDrawCircle(uint8_t x0 , uint8_t y0 , uint8_t radius );
void uiDisplayShowChar(uint8_t x , uint8_t y , uiFont_Enum font , char ascii);
void uiDisplayShowString(uint8_t x , uint8_t y , uiFont_Enum font , char* str);
void uiDisplayerRefresh(void);//��ʾ��ˢ�£�ÿһ��ʱ��ִ��һ�μ��ɣ����Դ�ͬ������Ļ��

/*������ʾ��*/
void uiDisplayerInit(uiKey_TypeDef* displayer, const char *lable, float *data, uint8_t x, uint8_t y, uint8_t length);

#endif

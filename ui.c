#include "ui.h"
#include "hw_uc1701.h"
#include "uc1701.h"
#include "font.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "inc/hw_types.h"   //提供了HWREG等的读写寄存器用的宏
#include "inc/hw_memmap.h"  //描述了所有外设的基地址
#include "inc/hw_gpio.h"    //描述了GPIO的寄存器相对于其基地址偏移量

#include "driverlib/sysctl.h" //提供了系统控制的库函数(使能外设时钟0)
#include "driverlib/gpio.h"   //提供了GPIO的库函数




/*声明内部函数*/
static int  uiGPIOKeyInit(uint32_t Port, uint32_t PinNumber);//配置GPIO为输入
static void uiKeyScan(uiKey_TypeDef* key);  //扫描单个按键的状态
static int uiEdgeCheck(int x, int y);//检查点是否在屏幕外

/*显存*/
volatile uint8_t GRAM[8][128] = {0};


/*声明全局变量*/
volatile uiKey_TypeDef *g_first_key = NULL;//记录第一个按键结构体的地址
volatile uiDisplayer_TypeDef *g_first_displayer = NULL;//记录第一个数据显示框的地址



/*实现外部函数*/

//按键初始化函数
void uiKeyInit(
        uiKey_TypeDef* key,
        uint32_t Port,
        uint8_t PinNumber,
        uiKeyPolarity_Enum Polarity,
        void(*callbackfuncion)(void) )
{
    static uiKey_TypeDef* s_previous_key = NULL;//记录上一个按键的地址

    //按键配置保存
    key->port      = Port;
    key->num       = PinNumber;
    key->polarity  = Polarity;
    key->callback  = callbackfuncion;//回调函数
    key->state     = NORMAL;
    key->sample[0] = 0;
    key->sample[1] = 0;
    key->sample[2] = 0;
    key->sample[3] = 0;
    key->next      = NULL;

    //配置GPIO
    uiGPIOKeyInit(Port, PinNumber);

    if(s_previous_key == NULL){
    //如果是第一个被初始化的按键，进行特殊配置
        g_first_key = key;//保存它的地址到全局变量
    }else{
    //不是第一个被初始化的按键，则把本按键的链表连接到上一个按键上
        s_previous_key->next = key;
    }

    //全部配置完成，把本按键的地址留作下次使用
    s_previous_key = key;
}

//全体按键扫描函数
void uiKeysRefresh(void)
{
    uiKey_TypeDef* key_pointer = (uiKey_TypeDef*)g_first_key;

    while(1)
    {
        if(key_pointer != NULL)
        {
            uiKeyScan(key_pointer);//扫描单个按键
            key_pointer = key_pointer->next;//沿链表移动到下一个按键
        }else{
            return;
        }
    }
}

//显示器初始化
void uiDisplayInit(void)
{
    UC1701Init(60000);//60000为SSI的频率
    UC1701Clear();
}

//显示器清除
void uiDisplayClear(void)
{
    UC1701Clear();
}

//显存清除
void uiGRAMClear(void)
{
     uint8_t i,j,*p;
     p=(uint8_t*)GRAM;
     for(i=0; i<8;i++){
         p = (uint8_t*)GRAM[i];
         for(j=0; j<128;j++){
             *p = 0x00;
             p++;
         }
     }
}

//刷新显存到屏幕上
void uiDisplayerRefresh(void)
{
    uint8_t i, j, data;

    for(i=0;i<8;i++){
        //按页连续刷新，无需配置地址
        UC1701CmdWrite(0xb0+i);
        UC1701CmdWrite(0x10);
        UC1701CmdWrite(0x00);

        for(j=0;j<128;j++){
            data = GRAM[i][127-j];
            UC1701DataWrite(data);
        }
    }
}

//画点
void uiDisplayDrawPoint(int x, int y)
{
    uint8_t data;
    if(uiEdgeCheck(x,y)){//要画的点在屏幕外
        return;
    }
    data = 0x01<<(y%8);
    GRAM[y/8][x] |= data;
}

//擦除点
void uiDisplayErasePoint(int x, int y)
{
    uint8_t data = ~( 0x01<<(y%8));
    GRAM[y/8][x] &= data;
}

//画线(Bresenham算法)
void uiDisplayDrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    short int delta_x, delta_y, error, polarity;
    uint8_t x_start, y_start, i=0, j=0, axis;

    delta_x = x2 - x1;
    delta_y = y2 - y1;

    //预处理,取主轴分量较小的那个点为起点
    if(fabsf(delta_x) < fabsf(delta_y)){//y轴为主轴
        axis = 1;//y轴
        if(delta_y < 0){
            y_start = y2;
            x_start = x2;
            polarity = x1>x2 ? 1 : -1;//极性
        }else{
            y_start = y1;
            x_start = x1;
            polarity = x2>x1 ? 1 : -1;//极性
        }
    }else{  //以x轴为主轴
        axis = 0;//x轴
        if(delta_x < 0){
            x_start = x2;
            y_start = y2;
            polarity = y1>y2 ? 1 : -1;//极性
        }else{
            x_start = x1;
            y_start = y1;
            polarity = y2>y1 ? 1 : -1;//极性
        }
    }

    delta_y = fabsf(delta_y);
    delta_x = fabsf(delta_x);


    if(axis){//以y轴为主轴
        error = -1*delta_y;
        for(i=0; i<=delta_y; i++){
            uiDisplayDrawPoint( x_start + polarity*j , y_start + i);//画点
            if(error >= 0)
            {
                j++;
                error -= 2*delta_y;
            }
            error += 2*delta_x;
        }

    }else{//以x轴为主轴
        error = -1*delta_x;
        for(i=0; i<=delta_x; i++){
            uiDisplayDrawPoint( x_start + i , y_start + polarity*j);//画点
            if(error >= 0){
               j++;
               error -= 2*delta_x;
           }
           error += 2*delta_y;
        }
    }
}

//画框
void uiDisplayDrawFrame(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    uiDisplayDrawLine(x1,y1,x1,y2);
    uiDisplayDrawLine(x1,y1,x2,y1);
    uiDisplayDrawLine(x1,y2,x2,y2);
    uiDisplayDrawLine(x2,y1,x2,y2);
}

//整块填充
void uiDisplayDrawBlock(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    //整块的page直接写显存，上下边缘用横线补齐
    uint8_t y_min, y_max, page_address, column_address, width,i,j;
    if(y1>y2){
        y_max = y1;
        y_min = y2;
    }else{
        y_min = y1;
        y_max = y2;
    }

    if(x1<x2){
        column_address = x1;
        width = x2-x1;
    }else{
        column_address = x2;
        width = x1-x2;
    }

    if( (y_min + 7)/8 < y_max/8 ){//框中有整块内存
        for(page_address = (y_min + 7)/8 ; page_address < y_max/8 ; page_address++){
            for(i = 0 ; i <= width ; i++){
                GRAM[page_address][column_address + i] = 0xff;
            }
        }
    }

    for(j = 0; j <= width; j++ ){
        for(i = y_min ; i<((y_min + 7)/8)*8 ; i++){
            uiDisplayDrawPoint(column_address + j , i);
        }

        for(i = (y_max/8)*8 ; i <= y_max ; i++){
            uiDisplayDrawPoint(column_address + j , i);
        }
    }
}

//整块擦除
void uiDisplayEraseBlock(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    //整块的page直接写显存，上下边缘用横线补齐
    uint8_t y_min, y_max, page_address, column_address, width,i,j;
    if(y1>y2){
        y_max = y1;
        y_min = y2;
    }else{
        y_min = y1;
        y_max = y2;
    }

    if(x1<x2){
        column_address = x1;
        width = x2-x1;
    }else{
        column_address = x2;
        width = x1-x2;
    }

    if( (y_min + 7)/8 < y_max/8 ){//框中有整块内存
        for(page_address = (y_min + 7)/8 ; page_address < y_max/8 ; page_address++){
            for(i = 0 ; i <= width ; i++){
                GRAM[page_address][column_address + i] = 0x00;
            }
        }
    }

    for(j = 0; j <= width; j++ ){
        for(i = y_min ; i<((y_min + 7)/8)*8 ; i++){
            uiDisplayErasePoint(column_address + j , i);
        }

        for(i = (y_max/8)*8 ; i <= y_max ; i++){
            uiDisplayErasePoint(column_address + j , i);
        }
    }
}

//画圆(Bresenham算法)
void uiDisplayDrawCircle(uint8_t x0 , uint8_t y0 , uint8_t radius )
{
    uint8_t width , i = 0 , j = radius;
    width = 0.7071f*(float)radius;

    for(i = 0; i<= width; i++){
        uiDisplayDrawPoint(x0 + i, y0 + j);
        uiDisplayDrawPoint(x0 + j, y0 + i);
        uiDisplayDrawPoint(x0 + j, y0 - i);
        uiDisplayDrawPoint(x0 + i, y0 - j);
        uiDisplayDrawPoint(x0 - i, y0 - j);
        uiDisplayDrawPoint(x0 - j, y0 - i);
        uiDisplayDrawPoint(x0 - j, y0 + i);
        uiDisplayDrawPoint(x0 - i, y0 + j);

        //判断(i+1,j)和(i+1,j-1)这两个点中的哪一个点到中心的距离和radius最接近
        if(radius*radius < (i+1)*i + (j-1)*j + 1){
            j--;
        }
    }
}


//显示一个字符
void uiDisplayShowChar(uint8_t x , uint8_t y , uiFont_Enum font , char ascii)
{
    uint8_t label, size, temp, i, j , height, y0, *font_ptr;

    label = ascii - ' ';//字库从 ' ' 符号开始取模，此处获得下标

    if(label > 94){//不是可显示的字符
        label = 31;//显示'?'号
    }

    y0 = y;//保存初值

    switch(font){
        case FONT1206:
            font_ptr = (uint8_t*)&asc2_1206[0][0];
            height = 12;
            size   = 12;
            break;
        case FONT1608:
            font_ptr = (uint8_t*)&asc2_1608[0][0];
            height = 16;
            size = 16;
            break;
        case FONT2412:
            font_ptr = (uint8_t*)&asc2_2412[0][0];
            height = 24;
            size = 36;
            break;
        default:return;//字库不存在
    }

    for(i = 0 ; i < size ; i++)//写字节
    {
        temp = *(font_ptr + label*size + i);
        for(j = 0 ; j < 8 ; j++){//写位
            if(temp&0x80){
                uiDisplayDrawPoint(x,y);
            }
            temp <<= 1;
            y++;
            if(y - y0 == height){
                y = y0;
                x++;
                break;//该列已写完，最后一个字节如果没有用完，说明这一字节本身就只有高4位有用
            }
        }
    }
}

//显示字符串
void uiDisplayShowString(uint8_t x , uint8_t y , uiFont_Enum font , char* str)
{
    int n = strlen(str), i, width, x0;
    x0 = x;

    switch(font){
        case FONT1206: width = 6 ;break;
        case FONT1608: width = 8 ;break;
        case FONT2412: width = 12;break;
        default:return;
    }

    for(i = 0; i < n; i++){
        if(*str == '\n'){//回车
            x = x0;
            y += 2*width;
            str++;
            continue;
        }else if(*str == '\r'){
            x = x0;
            str++;
            continue;
        }else if(*str == '\t'){
            x += 4*width;
            str++;
            continue;
        }


        uiDisplayShowChar(x, y, font, *(str++));
        x += width;
    }
}

/*实现内部函数*/

//GPIO配置函数
static int uiGPIOKeyInit(uint32_t Port, uint32_t PinNumber)
{
    uint32_t sysctl_periph_base;//系统控制外设基地址
    uint8_t unlock_flag = 0;//是否解锁

    //参数只传入了GPIO的外设基地址，需要自己推导出时钟使能需要的值
    switch(Port){
        case GPIO_PORTA_BASE     :
        case GPIO_PORTA_AHB_BASE : sysctl_periph_base = SYSCTL_PERIPH_GPIOA; break;
        case GPIO_PORTB_BASE:
        case GPIO_PORTB_AHB_BASE : sysctl_periph_base = SYSCTL_PERIPH_GPIOB; break;
        case GPIO_PORTC_BASE:
        case GPIO_PORTC_AHB_BASE : sysctl_periph_base = SYSCTL_PERIPH_GPIOC; break;
        case GPIO_PORTD_BASE:
        case GPIO_PORTD_AHB_BASE : sysctl_periph_base = SYSCTL_PERIPH_GPIOD; break;
        case GPIO_PORTE_BASE:
        case GPIO_PORTE_AHB_BASE : sysctl_periph_base = SYSCTL_PERIPH_GPIOE; break;
        case GPIO_PORTF_BASE:
        case GPIO_PORTF_AHB_BASE : sysctl_periph_base = SYSCTL_PERIPH_GPIOF; break;
        default: return -1;//发生错误
    }

    //外设使能
    SysCtlPeripheralEnable(sysctl_periph_base);

    //判断是否解锁
    switch(Port){
        default:
            unlock_flag = 0;
            break;
        case  GPIO_PORTC_BASE   :
            if(PinNumber < 4){//PC[3:0]被锁定
                unlock_flag = 1;
            }
        case  GPIO_PORTC_AHB_BASE   :
            if(PinNumber < 4){//PC[3:0]被锁定
                unlock_flag = 1;
            }
        case  GPIO_PORTD_BASE   :
            if(PinNumber == 7){//PD7被锁定
                unlock_flag = 1;
            }
        case  GPIO_PORTD_AHB_BASE   :
            if(PinNumber == 7){//PD7被锁定
                unlock_flag = 1;
            }
        case  GPIO_PORTF_BASE   :
            if(PinNumber == 0){//PF0被锁定
                unlock_flag = 1;
            }
        case  GPIO_PORTF_AHB_BASE   :
            if(PinNumber == 0){//PF0被锁定
                unlock_flag = 1;
            }
            break;
    }

    //解锁
    if(unlock_flag){
        HWREG(Port + GPIO_O_LOCK) = GPIO_LOCK_KEY;
        HWREG(Port + GPIO_O_CR) |= 0x01;
        HWREG(Port + GPIO_O_LOCK) = 0;
    }

    //GPIO配置为输入模式
    GPIOPinTypeGPIOInput(Port, 0x00000001<<PinNumber);//GPIO配置为输入

    return 0;
}

//单个按键扫描函数
static void uiKeyScan(uiKey_TypeDef* key)
{
    uint32_t voltage;
    const uint8_t key_state_press[4] = {1,1,0,0};

    //保存采样值
    key->sample[3] = key->sample[2];
    key->sample[2] = key->sample[1];
    key->sample[1] = key->sample[0];
    voltage = GPIOPinRead(key->port , 0x00000001<<(key->num));

    //若按键按下，则GPIO读到电压与极性相同，sample值置1
    if( voltage&&(key->polarity) || (!voltage)&&(!(key->polarity)) ){
        key->sample[0] = 1;
    }else{
        key->sample[0] = 0;
    }


    //判断连续四次的sample值是否满足按钮按下的状态
    if( !memcmp(key->sample, key_state_press, 4) ){
        key->state = PRESS;
    }else{
        key->state = NORMAL;
    }

    //判断是否触发回调函数
    if(key->state){
        if(key->callback != NULL){
            key->callback();
        }
    }
}

int uiEdgeCheck(int x, int y)
{
    if(y>63){
        return -1;
    }
    if(x>127){
        return -1;
    }
    if(x<0){
        return -1;
    }
    if(y<0){
        return -1;
    }

    return 0;
}

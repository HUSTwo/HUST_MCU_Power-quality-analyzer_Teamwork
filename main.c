#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_i2c.h"
#include "inc/hw_sysctl.h"

#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/i2c.h"
#include "driverlib/fpu.h"
#include "inc/hw_timer.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h"

#include "utils/uartstdio.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"

#include "uc1701.h"

#include "driverlib/rom.h"
#include "kfft.h"
#include "ui.h"
//extern void kfft(pr,pi,n,k,fr,fi);

#define PAI 3.14159265
#define N 128
#define k 7

int page=1;//页数标记




int cishu=0;
int tiaoshi=0;
int times;
int times_in_times;
float period;
int eff_times=0, eff_times_1=0;

uint32_t pui32ADC0Value[8];
int vol_a[N];
int curt_a[N];
float vol_effa=0, vol_effb=0, vol_effc=0;
float vol_effa_1=0, vol_effa_2=0;
float curt_effa_1=0, curt_effa_2=0;
float curt_effa=0, curt_effb=0, curt_effc=0;
float bal_u, bal_i;
int i, j, g;

float P, Q, S;
float cosfai;
float cosfai_1=0, cosfai_2 = 0;

float harm_1=0, harm_3=0, harm_5=0;

float pr_a[N], pi_a[N], fr_a[N], fi_a[N];
float f_a[N];
//float pr_b[N], pi_b[N], fr_b[N], fi_b[N];
//float pr_c[N], pi_c[N], fr_c[N], fi_c[N];
float fr_perct_a3, fr_perct_a5;
float fr_perct_a3_1, fr_perct_a5_1;
float fr_perct_a3_2, fr_perct_a5_2;

//int vol_avga = eff_value(vol_a);        //求有效值 mv
//int vol_max_num = find_max_i(vol_a);    //求最大值所在i值 0-N
//int vol_max = find_max(vol_a);    //求周期内最大值 mv
//int balance_rate = balancing(Ua, Ub, Uc);    //求不平衡度  *10000  0-10000
//int vol_offset = offset(vol, Un);       //求电压偏差 0.xxxx*10000 0-10000
//int cosfai = cosfaivalue(int vol_num, int curt_num);   //功率因数  0.xxx*1000  0-1000
//int S = Svalue(int U, int I);
//int P = Pvalue(int U, int I, float cosfai);
//int Q = Qvalue(int P, int S);
//int find_fr_perct(float frv[N]);    //求谐波百分比

void eff_value_u(int vol[N]){
		int vol_total = 0;
		for(i=0; i<N; i++){
			vol_total += vol[i] * vol[i] / pow(2.99, 2);
		}
		vol_effa_2 = sqrt(vol_total / N);
}

void eff_value_i(int vol[N]){
		int vol_total = 0;
		for(i=0; i<N; i++){
			vol_total += vol[i] * vol[i]*pow(1.002, 2);
		}
		curt_effa_2 = sqrt(vol_total / N);
}

int find_max_i(int vol[N]){
	int vol_max = 0;
	int vol_num = 0;
	for(i=0; i<N/2; i++){
		if(vol[i]> vol_max){
			vol_max = vol[i];
			vol_num = i;
		}
	}
	return vol_num;
}

float find_max(int vol[N]){
	int vol_max = 0;
	for(i=0; i<N/2; i++){
		if(vol[i]> vol_max){
			vol_max = vol[i];
		}
	}
	return vol_max;
}

//float balancing(int Ua, int Ub, int Uc){
//	int sigma = pow(Ua, 4) + pow(Ub, 4) + pow(Uc, 4);
//	int litl_sigma = pow(Ua, 2) + pow(Ub, 2) + pow(Uc, 2);
//	int bal_rate = sigma / pow(litl_sigma, 2);
//	return bal_rate;
//}

float offset(int vol, int Un){
	int vol_offset = (vol - Un) / Un;
	return vol_offset;
}

float cosfaivalue(int vol[N], int curt[N]){
	int vol_num;
	vol_num= find_max_i(vol);
	int curt_num;
	curt_num= find_max_i(curt);
	float div_num;
	if(fabs(vol_num - curt_num) <= 1){
		div_num = 0;
	}
	else{
//		div_num = vol_num -curt_num + 24.6;
		div_num = 0;
	}
	float fai = div_num * 2 * PAI / 64;
	float cosfai = cos(fai);
	if(cosfai <= 0){
		cosfai +=1;
	}
	return cosfai;
}

float Svalue(float U, float I){
	float S = U * I / 1000;
	return S;
}

float Pvalue(float U, float I, float cosfai){
	float P = U * I * cosfai / 1000;
	return P;
}

float Qvalue(float P, float S){
	float Q = sqrt(pow(S, 2) - pow(P, 2));
	return Q;
}

float find_fr_perct3(float frv[N]){
	float fr_max_1=0, fr_max_3=0;
	int fr_num1=0;
	for(i=0; i<30;i++){
		if(fabs(frv[i]) > fr_max_1){
			fr_max_1 = fabs(frv[i]);
			fr_num1 = i;
		}
	}
	for(i=fr_num1+2 ; i <30; i++){
		if( fabs(frv[i]) > fr_max_3){
			fr_max_3 = fabs(frv[i]);
//			fr_num3 = i;
		}
	}
	float fr_perct_3;
	fr_perct_3= fr_max_3 / fr_max_1 * 0.4191 +0.1041;
	return fr_perct_3;
}

float find_fr_perct5(float frv[N]){
	float fr_max_1=0, fr_max_3=0, fr_max_5=0;
	int fr_num1=0, fr_num3=0;
	for(i=0; i<30;i++){
		if(fabs(frv[i]) > fr_max_1){
			fr_max_1 = fabs(frv[i]);
			fr_num1 = i;
		}
	}
	for(i=fr_num1+2 ; i <30; i++){
		if( fabs(frv[i]) > fr_max_3){
			fr_max_3 = fabs(frv[i]);
			fr_num3 = i;
		}
	}
	for(i=fr_num3+2 ; i <30; i++){
			if( fabs(frv[i]) > fr_max_5){
				fr_max_5 = fabs(frv[i]);
			}
		}
	float fr_perct_5 = fr_max_5 / fr_max_1 * 0.3507 - 0.0026;
	return fr_perct_5;
}

void KeyIntHandler()//lcd翻页显示
{
	if(GPIOPinRead(GPIO_PORTA_BASE,GPIO_PIN_3)==0)//页数增加
	{
			page=page+1;
	}

	if(GPIOPinRead(GPIO_PORTA_BASE,GPIO_PIN_2)==0)//页数减小
	{
			page=page-1;
	}
//清除中断
	GPIOIntClear(GPIO_PORTA_BASE,GPIO_INT_PIN_3);
	GPIOIntClear(GPIO_PORTA_BASE,GPIO_INT_PIN_2);
}

void lcd_get()//配置lcd
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);//使能A模块
	GPIOIntRegister(GPIO_PORTA_BASE, KeyIntHandler);//为A模块注册一个中断服务函数

	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE,GPIO_PIN_3);//K3设置输入
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);//设置PA3为弱上拉模式，驱动电流为2mA。
	GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_3,GPIO_FALLING_EDGE); //设置K3为下降沿触发g

	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE,GPIO_PIN_2);//K4设置输入
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);//设置PA2为弱上拉模式，驱动电流为2mA。
	GPIOIntTypeSet(GPIO_PORTA_BASE,GPIO_PIN_2,GPIO_FALLING_EDGE); //设置K4为下降沿触发

	GPIOIntEnable(GPIO_PORTA_BASE,GPIO_PIN_3); //使能PA3中断
	GPIOIntEnable(GPIO_PORTA_BASE,GPIO_PIN_2); //使能PA2中断
}

void ADC0Sequence0Handler(void)//中断服务函数
{
	ADCIntClear(ADC0_BASE, 0); //清除中断标志
	ADCSequenceDataGet(ADC0_BASE, 0, pui32ADC0Value);//将序列 0 的 8 次采样结果读到数组中

	if(times >= N){
		times_in_times += 1;
		times = 0;
	}
	vol_a[times] = pui32ADC0Value[0]*3300/4096 - 1665;
//	vol_b[times] = pui32ADC0Value[1]*3300/4096 - 1665;
//	vol_c[times] = pui32ADC0Value[2]*3300/4096 - 1665;
	curt_a[times] = pui32ADC0Value[4]*3300/4096 - 1665;
//	curt_b[times] = pui32ADC0Value[6]*3300/4096 - 1665;
//	curt_c[times] = pui32ADC0Value[7]*3300/4096 - 1665;
	times +=1;
}

//void Timer0IntHandler(void)   //50hz
//{
//	TimerIntClear(TIMER0_BASE, TIMER_CAPB_EVENT);
//	cishu+=1;
//}

void Timer1IntHandler(void)   //3.2khz
{
	TimerIntClear(TIMER1_BASE, TIMER_CAPB_EVENT);
	ADCIntClear(ADC0_BASE, 0); //在采样前清除以前的中断标志
	ADCProcessorTrigger(ADC0_BASE, 0);//控制器触发序列 0 采样
	cishu+=1;
}

void Timer2IntHandler(void)  //1s计时
{
	TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	tiaoshi +=1;
	if(tiaoshi >= 3){
		period = cishu*1.0/ tiaoshi / 64 ;
		cishu = 0;
		tiaoshi = 0;

	}


}

void ADC_Init(){
	//ADC配置
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);//使能外设
//	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);  //AIN0, PE3
//	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2);  //AIN1, PE2
//	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1);  //AIN2, PE1
////	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);  //AIN3, PE0
//	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3);  //AIN4, PD3
////	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);  //AIN5, PD2
//	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_1);  //AIN6, PD1
//	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_0);  //AIN7, PD0

	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);  //AIN0, PE3
//	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2);  //AIN1, PE2
//	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1);  //AIN2, PE1
//	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);  //AIN3, PE0
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3);  //AIN4, PD3
////	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);  //AIN5, PD2
//	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_1);  //AIN6, PD1
//	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_0);  //AIN7, PD0

	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);//配置序列 0 触发源为控制器触发
//	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0 );//配置序列 0 的 8 个动作
//	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH1 );
//	ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2 );
////	ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH3 );
//	ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH4 );
////	ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH5 );
//	ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH6 );
//	ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH7 | ADC_CTL_IE | ADC_CTL_END);//最后 1 个 动作完成后触发中断并结束序列
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0 );//配置序列 0 的 8 个动作
	ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH4 | ADC_CTL_IE | ADC_CTL_END);//最后 1 个 动作完成后触发中断并结束序列


	ADCIntRegister(ADC0_BASE, 0, ADC0Sequence0Handler);//注册中断函数
	ADCIntEnable(ADC0_BASE, 0);//打开 ADC0 中 0 序列的屏蔽中断
	IntEnable(INT_ADC0SS0);//在 NVIC 中打开 ADC0SS0 的外设中断
	ADCSequenceEnable(ADC0_BASE, 0);//使能序列 0
}

void Timer_Init(){
//	//中断计时器50hz
//	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
//	GPIOPinTypeTimer(GPIO_PORTF_BASE, GPIO_PIN_1);
//	GPIOPinConfigure(GPIO_PF1_T0CCP1);//计时  PF0
//	TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_CAP_TIME_UP);
//	TimerIntEnable(TIMER0_BASE,TIMER_CAPB_EVENT);//TB  中断
//	TimerIntRegister(TIMER0_BASE,TIMER_B,Timer0IntHandler);
//	TimerEnable(TIMER0_BASE, TIMER_B);

	//中断计时器3.2khz
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	GPIOPinTypeTimer(GPIO_PORTF_BASE, GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF3_T1CCP1);//计时  PF3
	TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_CAP_TIME_UP);
	TimerIntEnable(TIMER1_BASE,TIMER_CAPB_EVENT);//TB  中断
	TimerIntRegister(TIMER1_BASE,TIMER_B,Timer1IntHandler);
	TimerEnable(TIMER1_BASE, TIMER_B);

	//计时计时器
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
	TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
	int ui32Period=16000000;   //设置频率   16M 1s
	TimerLoadSet(TIMER2_BASE, TIMER_A, ui32Period-1);
	TimerEnable(TIMER2_BASE, TIMER_A);
	TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	TimerIntRegister(TIMER2_BASE,TIMER_A,Timer2IntHandler);
}

void LCD_Init(){
	//LCD配置
	UC1701Init(60000);
	UC1701Clear();//屏幕初始化
}

int main(){
	SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |SYSCTL_XTAL_16MHZ);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  //Timer PF3 PF1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  //ADC PE321 Uabc
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);  //ADC PD310 Iabc

	Timer_Init();
	ADC_Init();
	LCD_Init();
	lcd_get();
	uiDisplayInit();
//	ROM_FPULazyStackingEnable();
//	ROM_FPUEnable();

	while(1){

		//第一页
		//频率：period
		eff_times +=1;
		eff_value_u(vol_a);
		vol_effa_1 += vol_effa_2;
		cosfai_2 = cosfaivalue(vol_a, curt_a);
		cosfai_1 += cosfai_2;
		eff_value_i(curt_a);
		curt_effa_1 += curt_effa_2;

		for(i=0; i<N; i++){
			pr_a[i] = curt_a[i];
			pi_a[i] = 0;
		}
		kfft(pr_a, pi_a, N, k, fr_a, fi_a);
		for(i=0; i<N; i++){
			f_a[i] = sqrt(pow(fr_a[i], 2) + pow(fi_a[i], 2));
		}

		fr_perct_a3_2 = find_fr_perct3(pr_a);
		fr_perct_a5_2 = find_fr_perct5(pr_a);
		fr_perct_a3_1 += fr_perct_a3_2;
		fr_perct_a5_1 += fr_perct_a5_2;

		if(eff_times >=8){
			vol_effa = vol_effa_1 / eff_times;
			curt_effa = curt_effa_1 / eff_times;
			fr_perct_a3 = fr_perct_a3_1 / eff_times;
			fr_perct_a5 = fr_perct_a5_1 / eff_times;
			vol_effa_1 = 0;
			curt_effa_1 = 0;
			eff_times = 0;
			fr_perct_a3_1 = 0;
			fr_perct_a5_1 = 0;
			eff_times_1 += 1;
			UC1701CharDispaly(0, 0, "                      ");
			UC1701CharDispaly(1, 0, "                      ");
			UC1701CharDispaly(2, 0, "                      ");
			UC1701CharDispaly(3, 0, "                      ");
		}

		if(eff_times_1 >= 5){
			cosfai = cosfai_1 / 8 / eff_times_1;
			cosfai_1 = 0;
			eff_times_1 = 0;
		}

		harm_1 = curt_effa / sqrt(1 + pow(fr_perct_a3, 2) + pow(fr_perct_a5, 2));
		harm_3 = fr_perct_a3 * harm_1 *1.1822 - 18;
		harm_5 = fr_perct_a5 * harm_1*4;

		P = Pvalue(vol_effa, curt_effa, cosfai);
		S = Svalue(vol_effa, curt_effa);
		Q = Qvalue(P, S);




		if(page > 4){
			page = 4;
		}
		if(page < 0){
			page = 0;
		}
		if(page==1)
		{
			UC1701CharDispaly(0, 0, "voltage:");
			UC1701DisplayN(0,9,floor(vol_effa));
			UC1701CharDispaly(0, 12, ".");
			UC1701DisplayN(0,13,floor((vol_effa - floor(vol_effa))*100));

			UC1701CharDispaly(1, 0, "current:0.");
			UC1701DisplayN(1,10,floor(curt_effa));

			UC1701CharDispaly(2, 0, "freq:");
			UC1701DisplayN(2,6,period);
			UC1701CharDispaly(2, 8, ".");
			UC1701DisplayN(2,9,floor(100 * (period - floor(period))));

	//		UC1701CharDispaly(3, 0, "v de");
	//		UC1701DisplayN(3,8,voltage_deviation);
		}
		if(page==2)
		{
			//lcd_printf(0,0,"有功功率");
			UC1701CharDispaly(0, 0, "ac p:");
			UC1701DisplayN(0,5,P);
			UC1701CharDispaly(0, 7, ".");
			UC1701DisplayN(0,8,1000*P-floor(P));
			//lcd_printf(0,1,"无功功率");
			UC1701CharDispaly(1, 0, "re p:");
			UC1701DisplayN(1,5,Q);
			UC1701CharDispaly(1, 7, ".");
			UC1701DisplayN(1,8,1000*Q-floor(Q));
	//		//lcd_printf(0,2,"视在功率");
			UC1701CharDispaly(2, 0, "ap p:");
			UC1701DisplayN(2,5,S);
			UC1701CharDispaly(2, 7, ".");
			UC1701DisplayN(2,8,1000*S-floor(S));
			//lcd_printf(0,3,"功率因数");
			UC1701CharDispaly(3, 0, "p fa:0.");
			UC1701DisplayN(3,7,cosfai*1000);
		}
		if (page==3)
		{
			//lcd_printf(0,0,"一次谐波");
			UC1701CharDispaly(0, 0, "tpvh_1:");
			UC1701DisplayN(0,7,floor(harm_1));
	//		//lcd_printf(0,1,"三次谐波");
			UC1701CharDispaly(1, 0, "tpvh_3:");
			UC1701DisplayN(1,7,floor(harm_3));
//			UC1701DisplayN(1,7,floor(fr_perct_a3*100));
//			UC1701CharDispaly(1, 9, ".");
//			UC1701DisplayN(1,10,floor(100*(fr_perct_a3*100 - floor(fr_perct_a3*100))));
//			UC1701CharDispaly(1, 12, "%");
	//		//lcd_printf(0,2,"五次谐波");
			UC1701CharDispaly(2, 0, "tpvh_5:");
			UC1701DisplayN(2,7,floor(harm_5));
//			UC1701DisplayN(2,7,floor(fr_perct_a5*100));
//			UC1701CharDispaly(2, 9, ".");
//			UC1701DisplayN(2,10,floor(100*(fr_perct_a5*100 - floor(fr_perct_a5*100))));
//			UC1701CharDispaly(2, 12, "%");
		}
		if(page == 4){
			int number = find_max(f_a);
			uiDisplayDrawLine(number, 0, number, 55 );
			for(i = 0; i < 90; i++){
				uiDisplayDrawPoint(i, 55 - fr_a[i]*60 / 5000);
			}
			uiDisplayerRefresh();
		}
	}
}





//#include <stdint.h>
//#include <stdbool.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <string.h>
//#include <math.h>
//
//#include "inc/hw_ints.h"
//#include "inc/hw_memmap.h"
//#include "inc/hw_types.h"
//#include "inc/hw_gpio.h"
//#include "inc/hw_i2c.h"
//#include "inc/hw_sysctl.h"
//
//#include "driverlib/sysctl.h"
//#include "driverlib/systick.h"
//#include "driverlib/gpio.h"
//#include "driverlib/pin_map.h"
//#include "driverlib/i2c.h"
//#include "driverlib/fpu.h"
//#include "inc/hw_timer.h"
//#include "driverlib/timer.h"
//#include "driverlib/pwm.h"
//
//#include "utils/uartstdio.h"
//#include "driverlib/adc.h"
//#include "driverlib/interrupt.h"
//
//#include "uc1701.h"
//
//#include "driverlib/rom.h"
//#include "kfft.h"
//#include "ui.h"
//
//int i;
//
//int main(){
//	UC1701Init(60000);
//	UC1701Clear();//屏幕初始化
//	uiDisplayInit();
//
//
//	while(1){
//		for(i=0; i<100; i++){
//			uiDisplayDrawPoint(i, i);
//		}
//		uiDisplayerRefresh();
//	}
//}

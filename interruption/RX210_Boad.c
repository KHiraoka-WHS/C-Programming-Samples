#include "iodefine.h"  
#include "LCD.h"
#include "stdio.h"

#define AM_L 520
#define AM_H 1700
#define FM_L 760
#define FM_H 949

int freq, frq_prev;
int frq_prev_AM = AM_L;
int frq_prev_FM = FM_L;

//sw2スイッチカウント（前の状態はprev)
int sw2_cnt, sw2_prev = 1;

//各スイッチのフラグ
unsigned char sw1_flg;
unsigned short sw2_flg;
unsigned char sw3_flg;

//周波数スピードアップ用（cnt は回数、flg はプラスマイナス状態）
int df_cnt = 0, df_flg = 1;

void sw2_det(void){
	
	if (sw2_flg){
			sw2_cnt++;
			sw2_cnt %= 3;
			timer(10);
		}
	sw2_flg = 0;
		
return; } // sw2(P33) sw2_cnt = 0 → ｵﾌ、1→AM 2→FM

// character buffer 
char frq_show[9] = "########"; // 上段
//static char state_show[9] = { 0 }; // 下段

//ON, AM表示用(sw2=P33)
char band_on[3][5] = {"OFF|", "AM |", "FM |"}; // 下段左
char lower[9] = { 0 };


/*周波数の上げ下げ*/
//sw1は、sw3はともにインクリメントするが、sw1は足す方向、sw3は引く方向
void IRQ1_get(void){
	sw1_flg ^= 1; //両エッジ（押し続ける限り１）
return; } 

void IRQ3_get(void){
	sw2_flg = 1; //立下りエッジ
return;}

void IRQ4_get(void){
	sw3_flg ^= 1; //両エッジ（押し続ける限り１）
return;}

void freq_change(int radi){
		char c_plus = 0, c_minus = 0;
		
			
		if (sw1_flg){
			if(df_cnt < 10){ timer(1000); }// 待つ
			else if(df_cnt >= 10) { timer(1); } // 0.1 ms 待つ
			if(sw1_flg){ c_plus++; }
				}
		else { sw1_flg = 0; }
		if (sw3_flg){
			if(df_cnt < 10){ timer(1000); }// 50ms 待つ
			else if(df_cnt >= 10) { timer(1); } // 0.1 ms 待つ
				if(sw3_flg){ c_minus++; }
				}
		else { sw3_flg = 0; }
		
		if (radi == 1) { 
			frq_prev = frq_prev_AM;
			freq = frq_prev + (c_plus - c_minus); // 1 kHzあげるか下げるか
			if (freq > AM_H) {freq = AM_L;}
			else if (freq < AM_L) {freq = AM_H;} 
			sprintf(frq_show, "%4d kHz", freq);
			frq_prev_AM = freq;}
		if (radi == 2) { 
			frq_prev = frq_prev_FM;
			freq = frq_prev + (c_plus - c_minus);
			if (freq > FM_H) {freq = FM_L;}
			else if (freq < FM_L) {freq = FM_H;}
			sprintf(frq_show, "%.1f MHz", (float)freq/10);
			frq_prev_FM = freq;}
			// 周波数超過させないようにする
			

			}


/*--------------------------------------------
	送信用リングバッファ構造体
----------------------------------------------*/
#define BUFSIZE 32
typedef struct {
	unsigned short rPos;
	unsigned short wPos;
	unsigned char buf[BUFSIZE];
} stSIO ;

/*--------------------------------------------
	グローバルメモリ
----------------------------------------------*/
stSIO mRev;		//受信管理
char sendbuff[4]={0};	// 送信データ
//int cntk;


/*受信割り込み*/
void intr_receive(void){
	
	mRev.buf[mRev.wPos++] = SCI0.RDR;			// 受信リングバッファに格納
	if(mRev.wPos >= BUFSIZE){ mRev.wPos = 0; }		// 格納位置補正
return;}

/*受信エラー割り込み*/
void intr_err_receive(void){
	unsigned char i = 0;
	
	SCI0.SCR.BIT.RIE = 0; // RX1およびERI割り込み要求を禁止
	
	i = SCI0.SSR.BIT.ORER;	//ORERエラー読み込み
	i = SCI0.SSR.BIT.FER;	//FERエラー読み込み	
	i = SCI0.SSR.BIT.PER;	//PERエラー読み込み	
	
	i++;
	
	SCI0.SSR.BIT.ORER = 0;	//ORERエラークリア	
	SCI0.SSR.BIT.FER = 0;	//FERエラークリア
	SCI0.SSR.BIT.PER = 0;	//PERエラークリア
	
	SCI0.SCR.BIT.RIE = 1; // RX1およびERI割り込み要求を許可
return;}

void send_char( char as_data[] ){
	while (*as_data != '\0'){
		SCI0.TDR = *as_data;
		while (SCI0.SSR.BIT.TEND == 0){;}
		as_data++;}
	return;}


void main(void)
{	
	unsigned int v0, vol, vol_prev = 0;
	
	//LED0(P14)からLED3(P16)までの設定
	PORT1.PODR.BIT.B4 = 1;
	PORT1.PODR.BIT.B5 = 1;
	PORT1.PODR.BIT.B6 = 1;
	
	PORT1.PDR.BIT.B4 = 1;
	PORT1.PDR.BIT.B5 = 1;
	PORT1.PDR.BIT.B6 = 1;
	
	
	//LCD初期化
	LCD_init();
	
	while(1){
		

		// sw2 は状態設定
		sw2_det();
		
		if (sw2_cnt){
			freq_change(sw2_cnt);
			PORT1.PODR.BYTE |= 0xF0;
			PORT1.PODR.BIT.B4 = 0; //LED0 ON
			if (sw2_cnt == 1) {
				PORT1.PODR.BIT.B5 = 0; }
			if (sw2_cnt == 2){
				PORT1.PODR.BIT.B5 = 1;
				PORT1.PODR.BIT.B6 = 0;}	
			
			v0 = S12AD.ADDR0;
			if (v0 < 140) { vol = 0;}
			else if (v0 >3800) { vol = 30;}
			else { vol = (int)((v0 - 140)/122);}
			
		} else { 
			for (int i = 0; i < 8; i++){
				frq_show[i]= '#';}
			PORT1.PODR.BYTE |= 0xF0;
			vol = 0;
			}
		
		if ((freq != frq_prev) || (sw2_cnt != sw2_prev)){
			
			LCD_cursor(1);
			LCD_struct(frq_show);

		}
		if (sw2_cnt != sw2_prev || vol != vol_prev){
			sprintf(lower, "%s %03d", band_on[sw2_cnt], vol);
			LCD_cursor(2);
			LCD_struct(lower);}
		
		df_flg = freq - frq_prev;
		vol_prev = vol;
		
		frq_prev = freq;
		sw2_prev = sw2_cnt;
		
		if(df_flg) { df_cnt++; }	
		        else{ df_cnt = 0;}		
		
		if(mRev.rPos != mRev.wPos){
			sendbuff[0] = mRev.buf[mRev.rPos++];
			send_char(sendbuff);
			mRev.rPos %= BUFSIZE;
			timer(200);}
			
	}	
	return;	
}

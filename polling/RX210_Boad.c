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

//sw2スイッチフラグ（前の状態はprev)
int sw2_flg, sw2_prev = 1;

//周波数スピードアップ用（cnt は回数、flg はプラスマイナス状態）
int df_cnt = 0, df_flg = 1; // このフラグの扱い要検討

void sw2_det(void){
	
	if (!(PORT3.PIDR.BIT.B3)){
			timer(1);
			while(!(PORT3.PIDR.BIT.B3));
			sw2_flg++;
			sw2_flg %= 3;
			timer(10);
		}
		
} // sw2(P33) sw2_flg = 0 → ｵﾌ、1→AM 2→FM

// character buffer 
char frq_show[9] = "########"; // 上段
//static char state_show[9] = { 0 }; // 下段

//ON, AM表示用(sw2=P33)
char band_L[3][9] = {"<  ｵﾌ  >", "<  AM  >", "<  FM  >"}; // 下段


/*周波数の上げ下げ*/
//sw1は、sw3はともにインクリメントするが、sw1は足す方向、sw3は引く方向
int sw1_in(void){
	return (int)PORT3.PIDR.BIT.B1 & 0x0001 ? 0: 1;
} // sw1(P31) on → '1', off → '0' に（正論理化）

int sw3_in(void){
	return (int)PORT3.PIDR.BIT.B4 & 0x0001 ? 0: 1;
} // sw3(P34) on → '1', off → '0' に（正論理化）

void freq_change(int radi){
		char c_plus = 0, c_minus = 0;
		
		
		if (sw1_in()){
			if(df_cnt < 10){ timer(1000); }// 50ms 待つ
			else if(df_cnt >= 10) { timer(1); } // 0.1 ms 待つ
				if(sw1_in()){ c_plus++; }
				}
		if (sw3_in()){
			if(df_cnt < 10){ timer(1000); }// 50ms 待つ
			else if(df_cnt >= 10) { timer(1); } // 0.1 ms 待つ
				if(sw3_in()){ c_minus++; }
				}
		
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
		
		



void main(void)
{	
	//LED0(P14)からLED3(P16)までの設定
	PORT1.PODR.BIT.B4 = 1;
	PORT1.PODR.BIT.B5 = 1;
	PORT1.PODR.BIT.B6 = 1;
	
	PORT1.PDR.BIT.B4 = 1;
	PORT1.PDR.BIT.B5 = 1;
	PORT1.PDR.BIT.B6 = 1;
	
	//スイッチ入力設定
	PORT3.PDR.BIT.B1 = 0; // SW1
	PORT3.PDR.BIT.B3 = 0; // SW2
	PORT3.PDR.BIT.B4 = 0; // SW3
	

	
	//LCD初期化
	LCD_init();
	
	while(1){

		// sw2 は状態設定
		
		
		sw2_det();
		
		if (sw2_flg){
			freq_change(sw2_flg);
			PORT1.PODR.BYTE |= 0xF0;
			PORT1.PODR.BIT.B4 = 0; //LED0 ON
			if (sw2_flg == 1) {
				PORT1.PODR.BIT.B5 = 0; }
			if (sw2_flg == 2){
				PORT1.PODR.BIT.B5 = 1;
				PORT1.PODR.BIT.B6 = 0;}	
		} else { 
			for (int i = 0; i < 8; i++){
				frq_show[i]= '#';}
			PORT1.PODR.BYTE |= 0xF0;
			}
		
		if ((freq != frq_prev) || (sw2_flg != sw2_prev)){
			
			LCD_cursor(1);
			LCD_struct(frq_show);

		}
		if (sw2_flg != sw2_prev){
			LCD_cursor(2);
			LCD_struct(band_L[sw2_flg]);}
		
		df_flg = freq - frq_prev;
		frq_prev = freq;
		sw2_prev = sw2_flg;
		
		if(df_flg) { df_cnt++; }	
		        else{ df_cnt = 0;}
			
		
	}	
	return;	
}

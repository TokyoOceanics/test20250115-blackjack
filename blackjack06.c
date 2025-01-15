/*
 * BlackJackプログラムの検討
 * 2022/11/末の状態遷移図を参考とした。
 *
 * 状態遷移に従う動作を検討
 
 2023/01/05 プログラミングを再開、stack03の成果を反映させること。 
 2023/01/08 blackjack05 1/5の机上検討を実装すること
2023/01/09 blackjack05b 若干の調整

2023/02/23 blackjack06 エースの点数を、手札のスコアが10以下の場合は、１１とし、11以上の場合は１とする。
コンピュータの手札の表示を抑制、但し若干のバグあり。
勝敗判定に1条件を追加した。ただし、あまり意味ないかもしれない。

 * */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>	//sleep()
#include <time.h>	//srand()
#include <string.h>	//strcat()


//デバッグ用
//#define DEBUG1
//#define DEBUG_GET_SWITCH


//スタック構造用の定数
#define max	13 
#define NoData -1



//データ構造の検討・検証用の状態
#define PUSH_DATA	100
#define POP_DATA	200
#define SHOW_DATA	300
#define GET_STACK_SIZE	400
#define SET_SAMPLE_DATA	500
#define RANDOMIZE_DATA	600

#define EXIT_PROGRAM	900	


//汎用フラグ
#define ON		1
#define OFF	0


//BLACK JACK GAME データ状態
#define BLACK_JACK	1000
#define GAME_START	1005
#define PLAYER_MAIN	1010
#define COMPUTER_MAIN	1020
#define GAME_RESULT	1030

#define ERROR_START	1900
#define	ERROR_PLAYER	1910
#define ERROR_COMPUTER	1920


//
//スイッチ読み込みの定義
//
#define	START   2000
#define HIT 	2010
#define STAND   2020



//
//ゲーム結果判定用定数
//
#define	OVR_21	3000
#define EQL_21	3010
#define ELS_21	3020

#define WIN		0
#define	LOSE	1
#define	DRAW	2


//構造体の定義

typedef struct stack{
	int data[max];
	int top;
} STACK;



//スタックデータの初期化
void init_stack(STACK *p)
{
	p->top=-1;	/*empty*/
	for(int i=0; i<max; i++) /* all clear */
		p->data[i]=0;
}


//スタックへのデータの格納
int push(STACK *p, int value)
{

	if(p->top >= max-1){
		printf("Can not push any more!\n");
		return -1;
	}else{
		p->top++;
		p->data[p->top]=value;
		return 0;
	}
}

//スタックからのデータの取り出し
int pop(STACK *p, int *value)
{
	if(p->top <= -1){
		//printf("Stack end detected.\n");
		*value=NoData;
		return -1;
	}else{
		*value = p->data[p->top];
		p->top--;
		return 0;
	}
}


//スタック内容の表示
void show_stack(STACK *p)
{
	for(int i=p->top; i>=0; i--)
		printf("%3d ", p->data[i]);

	printf("\n");
}


//
//スタック内容の表示 コンピュータ手札表示用
//
//スタック内容の最初の1つ目のデータを表示させない
// 2023/02/23 表示上のバグを含むも、リリースする。
//
void show_stack_for_computer(STACK* p)
{
	char score_string[100];
	char temp_string[5];
	for(int i=p->top; i>=0; i--){
		if(i==0) //(i==p->top)
			sprintf(temp_string,"???");
		else
			sprintf(temp_string,"%3d ", p->data[i]);

		strcat(score_string, temp_string);		
	}
	strcat(score_string,"\n");	
	
	printf("%s", score_string);

}


//スタックに格納されているデータ数の取得
int get_stack_size(STACK p)
{
	return p.top+1;
}

//汎用のデータ交換関数
void swap(int *a, int *b)
{
	int temp;
	temp=*a;
	*a=*b;
	*b=temp;
}

/* 配列の内容をランダムに並べ替えする
 * ０からN-1までのN個の乱数の発生
 * https://okwave.jp/qa/q5876682.htmlを参考とした。
 *
 */
void randomize_data(int size, int array[])
{

	for(int i=0; i<size; i++)
		swap(&array[i],&array[(int)((double)rand()/((double)RAND_MAX+1) * size)]);


}

/*
 * スタック内のデータをランダムに並べ替えする
 */
int randomize_stack(STACK *p)
{
	int i, data_size;
	int data[max], temp_data;

	/* pop all data */
	i=0;
	data_size=get_stack_size(*p);

	while(pop(p, &temp_data)!=-1){
		data[i]=temp_data;
		i++;
	}

	/*randomize array*/
	randomize_data(data_size, data);

	/*push array into stack*/
	for(i=0; i<data_size; i++){
		if(push(p, data[i])!=0)
			return -1;
	}


	return 0;

}

//
//１から１３の値をスタックに格納する
int set_initial_stack(STACK *p)
{
	p->top=-1;
	for(int i=0; i<13; i++)
		if(push(p, i+1)!=0)
			return -1;
	return 0;

}



//
// * 手札の構造体ND *hand)
//
typedef struct hand{
	int score;
	STACK card;

} HAND;


/*
 * カードを1枚引く関数
 *
 * 引いたカードは自分の手札に貯める
 * 引いたカードの値を手札の点数として加算する

 *手札の現在のスコアが１０点以下の場合は、引いたカードがエースなら１１点とする。
 *そうでなければ、エースは１点とする。
 *
* 第一引数 取り出し元のカードの山(STACK型変数)の先頭アドレス
* 第二引数　自分の手札(HAND型変数）の先頭アドレス
*
* 返り値 0：カードを一枚引くことが成功した　
*       -1:カードを一枚引くことに失敗した
*
*/

int draw_card(STACK *stack, HAND *hand)
{
	int card_value;

	if(pop(stack, &card_value)!=0){
		return -1;
	}else{
		if(push(&hand->card,card_value)!=0){ //カードを自分の手札にする。
			return -1;
		}else{
			if(card_value==1){		// ***エースの扱い***  2023/02/23変更
				if(hand->score<=10)	//手札のスコアが10以下なら、エースを11とする。
					card_value=11;
			}
			hand->score = hand->score + card_value; //点数の計算
			return 0;
		}
	}
}


/*
 * 現在の手札を表示する
 */
 
void show_hand(HAND hand)
{
	printf("Score:%d\n",hand.score);
	printf("Hand: ");
	show_stack(&hand.card);
}


/*
* コンピュータの手札表示
2023/02/23
*/

void show_hand_of_computer(HAND hand)
{
	printf("Score:%d\n",hand.score);
	printf("Hand: ");
	show_stack_for_computer(&hand.card);
}







/*
*	手札のスコアを求める
*/
int get_score(HAND hand){
	return hand.score;
}


/*
 手札の初期化
*/
void init_hand(HAND *hand)
{
	hand->score=0;
	init_stack(&hand->card);
}


//
//スイッチ読み込み関数
//
int get_switch(void)
{
	char c;

	while(1)
	{
		c=getc(stdin);
		switch(c){
			case 'h':
				return HIT;
			case 's':
				return STAND;
			case 'r':
				return START;
			default:
				continue;
		}
	}
}




//カードの山
STACK stack_data;

//手札の変数
HAND player_hand, computer_hand;


//ゲームの進行場面を表示する
void show_mode(int mode)
{
	printf("\n -- mode --\n");
	switch(mode)
	{
		case GAME_START:
			printf("GAME_START\n");
			break;
		case PLAYER_MAIN:
			printf("PLAYER_MAIN\n");
			break;
		case COMPUTER_MAIN:
			printf("COMPUTER_MAIN\n");
			break;
		case GAME_RESULT:
			printf("GAME_RESULT\n");
			break;
		default:
			printf("No Data %d\n",mode);
			break;
	}
	printf(" ---------\n");

}

//
//スタート場面
//
int blackjack_start(void)
{

	//スタックの初期化
	init_stack(&stack_data);

	//スタックに１－１３のカードを積む
	if(set_initial_stack(&stack_data)!=0){
		printf("Initial Stack NOT Generated.\n");
		return ERROR_START;
	}

	//スタックのシャッフル
	srand((unsigned int)time(NULL));
	int shuffle;
	shuffle=rand()%13;

	for(int i=0; i<shuffle; i++){
		if(randomize_stack(&stack_data)!=0){
			printf("Randomize Failure.\n");
			return ERROR_START;
		}else{
			printf("Data randomized.");
			show_stack(&stack_data);
		}
	}

	//手札の初期化
	init_hand(&player_hand);
	init_hand(&computer_hand);
	
	//スタックから、プレーヤ・コンピュータ双方で手札を2枚づつ引く

	for(int i=0; i<2; i++){
		if( draw_card(&stack_data, &player_hand)!=0){
			printf("Draw card Failure.\n");
			printf("[%d] player hand error\n",i);
			return ERROR_START;
		}

		if( draw_card(&stack_data, &computer_hand)!=0){
			printf("Draw card Failure.\n");
			printf("[%d] computer hand error\n",i);
			return ERROR_START;
		}		
	}

	//手札の表示(仮)
	printf("\nPlayer's Hand: ");
	show_hand(player_hand);
	printf("\nComputer's Hand: ");
	show_hand_of_computer(computer_hand);
	
	//ボタン押下待ち
	printf("\nHit START ([R]ed Button to Start Game)->");
	while(get_switch()!=START);
	return PLAYER_MAIN;
}




//
//プレーヤのターン
//
int player_main(void)
{
	int loop=ON;
	int return_code;
	int switch_code;

	if(get_score(player_hand)>21){				//プレーヤが21越えであれば即決で勝負あり
		return_code=GAME_RESULT;				//ゲーム結果判定へ
	}else if(get_score(computer_hand)>21){		//コンピュータが21越えであれば即決で勝負あり
		return_code=GAME_RESULT;				//ゲーム結果判定へ
	}else{										//そうでないならば、勝負をする
		printf("\nHit key ([S]tand Button to move Computer's turn.)");
		printf("\n        ([H]it Button to draw a card.)");
		printf("\n        ([R]  -- No Action --\n");

		while(loop){

			switch_code=get_switch();	//ボタン押し待ち

			switch(switch_code){
				case HIT:				//HITならカードをひく

					if( draw_card(&stack_data, &player_hand)!=0){
						printf("Draw card Failure.\n");
						printf("player hand error\n");
						return_code= ERROR_PLAYER;
						loop=OFF;
						break;
					}

					if( get_score(player_hand)>=21){	//引いた結果21以上なら勝負決まり
						return_code= GAME_RESULT;		//ゲーム結果判定へ
						loop=OFF;
					}

					show_hand(player_hand);

					continue;

				case STAND:					//STANDならコンピュータのターンへ
					return_code= COMPUTER_MAIN;
					loop=OFF;	
					break;

				default:
					continue;
			}
		}
	}
	return return_code;
}



//
//コンピュータのターン
//
int computer_main(void){

	int return_code;
	int score;


	score=get_score(computer_hand);		//17点以上なら、ゲーム結果判定へ
	if(score>=17){
		return_code=GAME_RESULT;
		return return_code;
	}


	while(1){							//17越えなら、勝負

		if( draw_card(&stack_data, &computer_hand)!=0){	//カードをひく
			printf("Draw card Failure.\n");
			printf("computer hand error\n");
			return ERROR_COMPUTER;
		}
		show_hand(computer_hand);
		score=get_score(computer_hand);
		
		if(score<21)	//21未満なら引き続き勝負　★★これだと、絶対負けるのでは？★★
			continue;
		else{			//21以上なら、ゲーム結果判定へ
			return_code=GAME_RESULT;
			break;
		}

	}
	return return_code;
}



//
// ゲーム結果の判定
//
/*

			判定表

score	Status	player	computer	result

21越え	OVR_21	>21		false		Player Lose
				false	>21			Player Win

21		EQL_21	=21		false		Player Win
				false	=21			Player Lose
				=21		=21			DRAW

21未満	ELS_21	player>				Player Win	
				player<				Player Lose
				player=				DRAW
*/


int judge_game(int player, int computer){

	int	game_result=LOSE;
	int status_number;

	//判定表から状態を取得
	if(player>21||computer>21)			//21越え
		status_number= OVR_21;
	else if(player==21||computer==21)	//21
			status_number= EQL_21;
		else
			status_number= ELS_21;		//21未満

	//状態から結果を判定
	switch(status_number){

		case OVR_21:			
			if(player<computer)
				game_result=WIN;
			else
				game_result=LOSE;
			break;

		case EQL_21:
			if(player==computer){
				game_result=DRAW;
			}else if( player==21)
				game_result=WIN;
			else if( computer==21)	//2023/02/23 add
				game_result=LOSE;
			else
				game_result=LOSE;
			break;
		case ELS_21:
			if(player>computer)
				game_result=WIN;
			else if(player==computer)
				game_result=DRAW;
			break;
		
		default:
			break;
	}

	return game_result;



}

//
//ゲーム結果表示
//
void show_game_result(int result){
	char game_result[][20]={"Player Win!", "Player Lose", "Draw!"};

	//手札の表示
	printf("\nPlayer's Hand: ");
	show_hand(player_hand);
	printf("\nComputer's Hand: ");
	show_hand(computer_hand);

	printf("\n-------------------");
	printf("\n%s", game_result[result]);
	printf("\n-------------------");
}


//
//ゲーム結果判定
//
int game_result(void)
{
	int game_result=LOSE;
	int player_score, computer_score;

	player_score=get_score(player_hand);					//ハンドのスコア取得
	computer_score=get_score(computer_hand);

	game_result=judge_game(player_score,computer_score);	//結果判定
	
	show_game_result(game_result);							//結果表示
	
	printf("\nHit START ([R] Button to ReStart Game)-> ");
	while(get_switch()!=START);

	return GAME_START;
}







//メイン関数
int main(int argc, char* argv[]){

	int mode;

	init_stack(&stack_data);
	init_hand(&player_hand);
	init_hand(&computer_hand);

	mode=GAME_START;
	
	while(1){
		show_mode(mode);
		switch(mode){
			case GAME_START:
				mode=blackjack_start();
				continue;

			case PLAYER_MAIN:
				mode=player_main();
				continue;

			case COMPUTER_MAIN:
				mode=computer_main();
				continue;

			case GAME_RESULT:
				mode=game_result();
				continue;

			default:
				break;


		}

	}


}


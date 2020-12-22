//編集：田上慶治
#include "DxLib.h"
#include <math.h>
#include "Shooting.h"



/*以下に定数の定義(define)*/

/*以下に構造体の定義*/
enum Scene {
	start,
	end,
	stage1,
	ending
};
typedef struct{
	int circle;
}Graphic;
//リストのポインタ
PBullet *pbHead = NULL;//プレイヤーの弾の空リスト(先頭)を作成
PBullet *pbTail = NULL;//プレイヤーの弾の空リスト(末尾)を作成
Elist *eHead = NULL;//敵の空リスト(先頭)を作成
Elist *eTail = NULL;//敵の空リスト(末尾)を作成
EBullet *ebHead = NULL;//敵弾の空リスト(先頭)を作成
EBullet *ebTail = NULL;//敵弾の空リスト(末尾)を作成
/*構造体のポインタ*/
Key *keys;//キー構造体のポインタ
Graphic *gphandle;//画像ハンドル構造体

/*以下に関数のプロトタイプ宣言を書く*/
Player initializePlayer();
void updateKeys();
//head->...->tailの末尾にnodeを追加
template<typename T>
void pushBack(T **node, T ** head, T **tail);
//nodeをhead-tailのリストから消す(nodeの次のノードを返す)
template<typename T>
T* deleteNode(T **node, T **head, T **tail);
//head-tailのリストのノードをすべて消去
template<typename T>
void deleteAllNode(T **head, T **tail);
template<typename T, typename U>
int onCollisionCircle(T obj1, U obj2);
void collisionEnemyAndPlayerShot();
void collisionPlayerAndEnemy(Player *p);
void collisionPlayerAndEnemyShot(Player *p);
void drawHPBar(double x, double y, double hp_per,int vertical);
void loadGraphHandles();
void drawHPCircle(double x, double y, double hp_per, double size_per);
void drawUI(Player p);
void gameStage1(int count, Player *p, int *fairy1, int *fairy2);
void gameEnd(Player *p);

/*メイン関数*/
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	ChangeWindowMode(TRUE); // ウィンドウモードに設定
	DxLib_Init(); // DXライブラリ初期化処理
	SetDrawScreen(DX_SCREEN_BACK);//描画先を裏画面にする

	keys = (Key*)malloc(sizeof(Key));//キー入力の状態を格納する構造体
	GetHitKeyStateAll(keys->nowKeys);//最初のキー入力の状態を初期化
	Scene scene = start;
	/*以下に初期化処理を書く*/
	Player p = initializePlayer();//プレイヤーの初期化
	loadGraphHandles();//画像ハンドルのロード
	int fairy[4];//妖精の画像ハンドル
	LoadDivGraph("graphic/fairy.png", 4, 2, 2, 100, 100, fairy);
	int fairy1[2] = { fairy[0],fairy[1] };//青色の妖精
	int fairy2[2] = { fairy[2],fairy[3] };//赤色の妖精


	int count = 0;//ステージカウント
				  /*メインループ*/
	while (1) {
		if (ProcessMessage() != 0) {//メッセージ処理
			break;//×ボタンでループ終了
		}
		ClearDrawScreen();//画面を消す
		clsDx();//ループごとに前の結果を消す
		
		switch (scene) {
		case start:
			//gameStart();
			DrawString(CX, CY, "スペースで始まる", COLOR::white);
			if (keys->nowKeys[KEY_INPUT_SPACE])scene = stage1;
			break;
		case stage1:
			gameStage1(count,&p,fairy1,fairy2);
			if (p.hp == 0)scene = ending;
			break;
		case ending:
			//gameEnding();
			DrawString(CX, CY, "GameOver...スペースで終了", COLOR::white);
			if (keys->nowKeys[KEY_INPUT_SPACE])scene = end;
			
			break;
		case end:
			gameEnd(&p);
			break;
		}
		if (scene == end)
			break;
		updateKeys();
		count++;
		ScreenFlip();//裏画面を表画面に反映
	}
	DxLib_End(); // DXライブラリ終了処理
	return 0;
}

void gameStage1(int count,Player *p,int *fairy1,int *fairy2) {
	/*以下に描画処理を書く*/
	drawEnemy(count);//敵の描画処理
	drawPlayer(p);
	drawPlayerBullet();
	drawEnemyBullet();
	/*以下に更新(+補正)処理を書く*/
	calcEnemy();//敵の計算処理
	movePlayer(p, count);//プレイヤーの動作
	calcPlayerBullet();
	calcEnemyBullet();
	//敵を2通り作る
	Enemy e1 = { 0,0,1,1,100,30,100,100,0.7,fairy1 };
	Enemy e2 = { MX,MY,-1,-1,100,30,100,100,0.7,fairy2 };
	if (count % 60 == 0) {
		addEnemy(e1);
	}
	else if (count % 60 == 30) {
		addEnemy(e2);
	}

	//ここから弾を生み出す処理
	createPlayerShot(p, count);
	createEnemyShot(*p, count);
	collisionEnemyAndPlayerShot();//敵-プレイヤー弾のコリジョン処理
	collisionPlayerAndEnemyShot(p);//プレイヤー-敵弾のコリジョン処理
	collisionPlayerAndEnemy(p);//プレイヤー-敵のコリジョン処理
	if (p->hp < 0) p->hp = 0;//プレイヤーのHPを-にしない
	drawUI(*p);
}
void gameEnd(Player *p) {
	delAllEnemy();//一括フリー
	delAllPlayerBullet();//一括フリー
	delAllEnemyBullet();
	free(keys);
	free(p->graph);
	free(gphandle);
}
void loadGraphHandles() {
	using namespace GRAPHIC;
	gphandle =(Graphic*) malloc(sizeof(Graphic));
	gphandle->circle = LoadGraph(circle);
}
//HPバーの描画
void drawHPBar(double x, double y, double hp_per,int vertical=1) {
	if (vertical) {
		DrawBox(x, y - HPBAR_YSIZE, x + HPBAR_XSIZE, y, COLOR::gray, 1);
		DrawBox(x, y - (HPBAR_YSIZE * hp_per), x + HPBAR_XSIZE, y, COLOR::green, 1);
	}
	else {
		DrawBox(x, y - HPBAR_XSIZE, x + HPBAR_YSIZE, y, COLOR::gray, 1);
		DrawBox(x, y - HPBAR_XSIZE, x + (HPBAR_YSIZE * hp_per), y, COLOR::green, 1);
	}
}
//円形HPの描画
void drawHPCircle(double x, double y, double hp_per,double size_per) {
	DrawCircleGauge(x, y, 100, gphandle->circle, 0, size_per, 1, 0);
	SetDrawBright(0, 255, 0);
	DrawCircleGauge(x, y, hp_per * 100, gphandle->circle, 0, size_per, 1, 0);
	SetDrawBright(255, 255, 255);
}
void drawUI(Player p) {
	DrawBox(MX, 0, 640, MY, GetColor(200,100,0),1);//枠の描画
	drawHPBar(HPBAR_X, HPBAR_Y, (double)p.hp / PLAYER::hp,0);
}

//円の当たり判定
template<typename T,typename U>
int onCollisionCircle(T obj1, U obj2) {
	//2点間の距離
	double dist = hypot(obj2.x - obj1.x, obj2.y - obj1.y);
	if (dist <= (obj1.r + obj2.r))
		return TRUE;
	return FALSE;
}

void collisionEnemyAndPlayerShot() {
	Elist *e = eHead;
	PBullet *pb;
	while (e != NULL) {
		pb = pbHead;
		while (pb != NULL) {
			if (onCollisionCircle(e->e, pb->s)) {
				e->e.hp -= 3;//敵HPを減らす
				pb = delPlayerBullet(pb);
			}
			else
				pb = pb->next;
		}
		e = e->next;
	}
}
void collisionPlayerAndEnemyShot(Player *p) {
	EBullet *eb = ebHead;
	while (eb != NULL) {
		if (onCollisionCircle(*p, eb->s)) {
			p->hp -= 1;//プレイヤーHPを減らす
			eb = delEnemyBullet(eb);
		}
		else
			eb = eb->next;
	}
}
void collisionPlayerAndEnemy(Player *p) {
	Elist *e = eHead;
	while (e != NULL) {
		if (onCollisionCircle(e->e, *p)) {
			p->hp -= 2;//プレイヤーHPを減らす
			e->e.hp -= 10;
		}
		e = e->next;
	}
}
Player initializePlayer() {
	using namespace PLAYER;
	//x,y,size_x,size_y,v,exrate,interval,scount,index,(graph)
	const int scount = 0, index = 1;//隠しパラメータの初期化
									//プレイヤーの構造体と初期化
	Player p = { x,y,hp,r,s_x,s_y ,v,exrate,interval, scount ,index };
	p.graph = (int*)malloc(sizeof(int) * 9);
	LoadDivGraph(path, 9, 3, 3, s_x, s_y, p.graph);
	return p;
}
//キー構造体の更新
void updateKeys() {
	memcpy(keys->preKeys, keys->nowKeys, sizeof(keys->nowKeys));//前のキーの押下状態を更新
	GetHitKeyStateAll(keys->nowKeys);//現在のキーの押下状態を取得
	for (int i = 0; i < 256; i++)//全てのキーについて
		keys->state[i] = (KeyState)(keys->nowKeys[i] - keys->preKeys[i]);//パルスの検出→押した瞬間,離した瞬間を状態として格納
}

/*以下に関数の定義を書く*/
template<typename T>
void pushBack(T **node, T ** head, T **tail) {
	(*node)->next = NULL;//追加ノードの次はNULL

	if (*head == NULL) //空のリストの場合
		*head = *node;//ヘッドは追加ノードを指す
	if (*tail)//末尾のノードが既にあるとき
		(*tail)->next = *node;//前ノードの先を追加ノードに
	(*node)->before = *tail;//追加ノードの前を前ノードに
	*tail = *node;//Tailは最後のノードを指す
}
template<typename T>
T* deleteNode(T **node, T **head, T **tail) {
	T * pReturn = (*node)->next;//一個先のノードを指すポインタの用意
	if ((*node)->before)//ノードが先頭でないとき
		(*node)->before->next = pReturn;//一個前の次を一個先のノードにする
	else
		*head = pReturn;//ヘッドの指すノードを一個先にする

	if (pReturn)//pが末尾を指さないとき
		pReturn->before = (*node)->before;//一個先のノードから2個前のノードをつなぐ
	else//pが末尾を指すとき
		*tail = (*node)->before;//テールは前のノードを指す

	free(*node);//ノードを消す
	return pReturn;//次のポインタを返す

}
template<typename T>
void deleteAllNode(T **head, T **tail) {
	T *p, *pnext;//ポインタを2つ用意
	pnext = *head;//ヘッドのノードを指す
	while (pnext) {//ノードがなくなるまで繰り返す
		p = pnext;//pを進める
		pnext = p->next;//pnextを進める
		free(p);//pのノードを開放
	}
	*head = NULL;//ヌル
	*tail = NULL;//ヌル
}

void drawEnemyBullet() {
	int sum = 0;//リストの要素数
	EBullet *itr = ebHead;//ヘッドのポインタをコピー
	while (itr != NULL) {//末尾まで回す
						 //円の描画
		DrawCircle(itr->s.x, itr->s.y, itr->s.r, itr->s.color, 1);
		itr = itr->next;//進める
		sum++;
	}
	printfDx("eBullet=%d\n", sum);//要素数を表示
}
void calcEnemyBullet() {
	EBullet *itr = ebHead;
	while (itr != NULL) {//末尾まで
		itr->s.x += itr->s.vx;//x方向の移動
		itr->s.y += itr->s.vy;//y方向の移動

							  //範囲内
		if (Btwn(-20, itr->s.x, MX + 20) && Btwn(-20, itr->s.y, MY + 20)) {//画面外+20のとき
			itr = itr->next;//進める
		}
		else {//範囲外
			itr = delEnemyBullet(itr);//該当の弾をEBulletから消す
		}


	}
}
void createEnemyShot(Player p, int count) {
	Bullet b = { 0, 0, 4, 0, 5 ,GetColor(255,255,255) };//弾データ
	if (count % 60 == 0) {//(0,30...)のタイミング
		Elist *itr = eHead;
		int j;
		//全ての敵でループ
		while (itr != NULL) {
			b.x = itr->e.x, b.y = itr->e.y;//敵の座標に重ねる
			double v = hypot(b.vx, b.vy);
			double rad = getAngle(b, p) / 180 * PI;
			b.vx = v * cos(rad), b.vy = v * sin(rad);
			addEnemyBullet(b);//弾を追加

			itr = itr->next;
		}
	}
}
//弾からプレイヤーへの角度を度数法で返す
double getAngle(Bullet b, Player p) {
	double rad = atan2(p.y - b.y, p.x - b.x);
	return 180 * rad / PI;
}
//敵の弾の追加(リストの末尾に追加)
void addEnemyBullet(Bullet b) {
	//新しいnodeデータ
	EBullet *pnode = (EBullet *)malloc(sizeof(EBullet));
	pnode->s = b;//新たなノードにデータを入れる
	pushBack(&pnode, &ebHead, &ebTail);//末尾に追加

}

//敵弾の削除
//引数：削除するノードを指すポインタ
//戻値：次のポインタ
EBullet* delEnemyBullet(EBullet *p) {
	return deleteNode(&p, &ebHead, &ebTail);
}

//敵弾リスト全体を消去
void delAllEnemyBullet() {
	deleteAllNode(&ebHead, &ebTail);
}




void drawPlayerBullet() {
	int sum = 0;//リストの要素数
	PBullet *itr = pbHead;//ヘッドのポインタをコピー
	while (itr != NULL) {//末尾まで回す
						 //円の描画
		DrawCircle(itr->s.x, itr->s.y, itr->s.r, itr->s.color, 1);
		itr = itr->next;//進める
		sum++;
	}
	printfDx("pBullet=%d\n", sum);//要素数を表示
}
void calcPlayerBullet() {
	PBullet *itr = pbHead;
	while (itr != NULL) {//末尾まで
		itr->s.x += itr->s.vx;//x方向の移動
		itr->s.y += itr->s.vy;//y方向の移動
							  //範囲内
		if (Btwn(-20, itr->s.x, MX + 20) && Btwn(-20, itr->s.y, MY + 20)) {//画面外+20のとき
			itr = itr->next;//進める
		}
		else {//範囲外
			itr = delPlayerBullet(itr);//該当の弾をPBulletから消す
		}

	}
}

void createPlayerShot(Player *p, int count) {
	int isPush = keys->state[KEY_INPUT_Z] == pushNow;//zを押したか
	int isPull = keys->state[KEY_INPUT_Z] == pullNow;//zを離したか
	int isEnoughInterval = p->sCount + p->interval <= count;//連打していないか
	if (isPull)p->sCount = count;//発射タイミングの基準を更新
	if (isPush&&isEnoughInterval) p->sCount = count;//発射タイミングの基準を更新

	if (!keys->nowKeys[KEY_INPUT_Z]) return;//zキーを押すかつ
	if (!((count - p->sCount) % p->interval == 0)) return;//インターバル内なら

														  //以下はショットの決定と生成
	Bullet b1 = { p->x - 10, p->y, 0, -5, 5 ,GetColor(255,0,0) };//弾データを用意
	Bullet b2 = { p->x + 10, p->y, 0, -5, 5 ,GetColor(255,0,0) };//弾データを用意
	addPlayerBullet(b1);//弾を追加
	addPlayerBullet(b2);//弾を追加
}


//画面+blank内にいるか
int isInWall(double x, double y, double blank) {
	return Btwn(-blank, x, MX + blank) && Btwn(-blank, y, MY + blank);
}

//敵の描画処理
void drawEnemy(int count) {
	int sum = 0;//リストの要素数
	Elist *itr = eHead;//ヘッドのポインタをコピー
	while (itr != NULL) {//末尾まで回す
						 //画像の描画
		if (count % 30 < 15) {
			DrawRotaGraph(itr->e.x, itr->e.y, itr->e.exRate, 0, itr->e.graph[0], 1);
		}
		else {
			DrawRotaGraph(itr->e.x, itr->e.y, itr->e.exRate, 0, itr->e.graph[1], 1);
		}
		drawHPCircle(itr->e.x, itr->e.y, itr->e.hp / 100.0,0.4);
		itr = itr->next;//進める
		sum++;
	}
	printfDx("Enemy=%d\n", sum);//要素数を表示
}
//敵の計算処理
void calcEnemy() {
	Elist *itr = eHead;
	while (itr != NULL) {//末尾まで
		itr->e.x += itr->e.vx;//x方向の移動
		itr->e.y += itr->e.vy;//y方向の移動

		if (itr->e.hp>0 && isInWall(itr->e.x, itr->e.y, 100)) {//生きているかつ範囲内にいる場合
			itr = itr->next;//進める
		}
		else {//hpが0か負または画面の範囲外にいるとき
			itr = delEnemy(itr);//該当の敵をElistから消す
		}

	}
}


//敵の追加(リストの末尾に追加)
void addEnemy(Enemy e) {
	// 新しいnodeデータ
	Elist *pnode = (Elist *)malloc(sizeof(Elist));
	pnode->e = e;//新たなノードにデータを入れる
	pushBack(&pnode, &eHead, &eTail);//末尾に追加

}

//敵の削除
//引数：削除するノードを指すポインタ
//戻値：次のポインタ
Elist* delEnemy(Elist *p) {
	return deleteNode(&p, &eHead, &eTail);
}

//リスト全体を消去
void delAllEnemy() {
	deleteAllNode(&eHead, &eTail);
}

//プレイヤーの弾の追加(リストの末尾に追加)
void addPlayerBullet(Bullet b) {
	// 新しいnodeデータ
	PBullet *pnode = (PBullet *)malloc(sizeof(PBullet));
	pnode->s = b;//新たなノードにデータを入れる
	pushBack(&pnode, &pbHead, &pbTail);//末尾に追加

}

//弾の削除
//引数：削除するノードを指すポインタ
//戻値：次のポインタ
PBullet* delPlayerBullet(PBullet *p) {
	return deleteNode(&p, &pbHead, &pbTail);

}

//リスト全体を消去
void delAllPlayerBullet() {
	deleteAllNode(&pbHead, &pbTail);
}

// p1からp2の範囲にpがあるとき1を返す
int Btwn(int p1, int p, int p2) {
	if (p2>p1 && (p1 <= p) && (p <= p2))
		return 1;
	if (p2 < p1 && (p2 <= p) && (p <= p1))
		return 1;
	return 0;
}
void movePlayer(Player *p, int count) {
	int index, interval = 15;//画像のインデックスとアニメーションの間隔
							 //フレームごとのアニメーション画像の基準を指定
	if (count%interval < interval / 3)
		index = 0;
	else if (count%interval < interval / 3 * 2)
		index = 3;
	else {
		index = 6;
	}
	//キー入力
	bool isR = keys->nowKeys[KEY_INPUT_RIGHT];
	bool isL = keys->nowKeys[KEY_INPUT_LEFT];
	bool isU = keys->nowKeys[KEY_INPUT_UP];
	bool isD = keys->nowKeys[KEY_INPUT_DOWN];
	bool isDiag = (isR - isL) && (isD - isU);
	double v = p->v;
	if (isDiag)v *= cos(PI / 4);//斜めで減衰
	double vx = v*(isR - isL);
	double vy = v*(isD - isU);
	printfDx("v=%lf\n", v);
	//インデックスの決定
	if (isR) {
		p->index = index;//そのまま
	}
	else if (isL) {
		p->index = index + 2;//2列ずらす
	}
	else {
		p->index = index + 1;//1列ずらす
	}
	//移動
	p->x += vx, p->y += vy;
	//上限値と下限値の処理
	int minX = p->size_x / 2 * p->exRate;
	int maxX = MX - p->size_x / 2 * p->exRate;
	int minY = p->size_y / 2 * p->exRate;
	int maxY = MY - p->size_y / 2 * p->exRate;
	if (p->y > maxY)//下端を越えている
		p->y = maxY;
	else if (p->y < minY)//上端を越えている
		p->y = minY;
	if (p->x > maxX)//右端を越えている
		p->x = maxX;
	else if (p->x < minX)//左端を越えている
		p->x = minX;

}
//プレイヤーの描画
void drawPlayer(Player *p) {
	DrawRotaGraph(p->x, p->y, p->exRate, 0, p->graph[p->index], 1);
}
//2dベクトル(x,y)を時計回りにdeg°回転する
//引数：x,yのポインタ,回転角度deg
void rotate2d(double *x, double *y, double deg) {
	double rad = deg / 180.0 * PI;//度数法→弧度法
	double x0 = *x, y0 = *y;//更新前のx,yをx0,y0として格納
	*x = x0 * cos(rad) - y0 * sin(rad);//xの回転
	*y = x0 * sin(rad) + y0 * cos(rad);//yの回転
}
#include "widget.h"
#include "ui_widget.h"

QMutex ChessBoard::m_move;
QMutex ChessBoard::m_hash;
ChessBoard* ChessBoard::pChess = nullptr;
AI* AI::obj = nullptr;

int ChessBoard::CHESS_VALUE[7] = {2, 8, 12, 24, 28, 32, 64};
//int ChessBoard::CHESS_VALUE[0] = 2;     // MOO+++
//int ChessBoard::CHESS_VALUE[1] = 8;     // MOOO++
//int ChessBoard::CHESS_VALUE[2] = 8;     // MOO+O+
//int ChessBoard::CHESS_VALUE[3] = 32;    // +OO+++
//int ChessBoard::CHESS_VALUE[4] = 32;    // +OO+O+
//int ChessBoard::CHESS_VALUE[5] = 64;    // +OOO++
//int ChessBoard::CHESS_VALUE[6] = 64;    // MOOO+O or MOOOO+

int ChessBoard::UNION_VALUE[4] = {500, 800, 1000, 2000};
//int ChessBoard::UNION_VALUE[0] = 500;   // 双活三
//int ChessBoard::UNION_VALUE[1] = 800;   // 四三
//int ChessBoard::UNION_VALUE[2] = 1000;  // 活四
//int ChessBoard::UNION_VALUE[3] = 2000;  // 五连

int ChessBoard::CHESS_PRIOR[3] = {1, 100, 10000};
//int ChessBoard::CHESS_PRIOR[0] = 1;     // 活三
//int ChessBoard::CHESS_PRIOR[1] = 100;   // 冲四
//int ChessBoard::CHESS_PRIOR[2] = 10000; // 五连

ChessBoard::ChessBoard(){

    memset(vis, 0, sizeof(vis));
    memset(chess, 0, sizeof(chess));

    H = new HashItem[HASH_TABLE_SIZE]();
    qsrand(QTime(0,0,0).msecsTo(QTime::currentTime()));

    FF(0, size, 0, size){
        Z[BLACK][i][j] = _rand64();
        Z[WHITE][i][j] = _rand64();
        add[i][j] = 2*size/(1+(abs(i-size/2)+abs(j-size/2)));

    }

    for(int i = 0; i < size; i++){
        QString tmp, out;
        for(int j = 0; j< size; j++){
            tmp.sprintf("%3d", add[i][j]);
            out += tmp;
        }
        out += "\n";
        qDebug(out.toLatin1());
    }

#ifdef DEBUG_DEFINE
    qDebug("initialing board complete");
#endif

}

ChessBoard::~ChessBoard(){
    delete H;
}

ChessBoard* ChessBoard::operator=(const ChessBoard &obj){
    return nullptr;
}

ChessBoard* ChessBoard::_getInstance(){
    if(pChess == nullptr){
        QMutexLocker lock(&m_move);
        QMutexLocker lock2(&m_hash);
        if(pChess == nullptr){
            pChess = new ChessBoard();
        }
    }

#ifdef DEBUG_DEFINE
    qDebug("return board pointer pChess complete");
#endif

    return pChess;
}

bool ChessBoard::_inside(int x, int y){
    if (0 <= x && x < size && 0 <= y && y < size)
        return true;
    return false;
}

bool ChessBoard::_inside(const Pos &move){
    if (0 <= move.x && move.x < size && 0 <= move.y && move.y < size)
        return true;
    return false;
}

bool ChessBoard::_isEmpty(const Pos &move){
    return chess[move.x][move.y] == 0;
}

bool ChessBoard::_isEmpty(int x, int y){
    return chess[x][y] == 0;
}

bool ChessBoard::_makeMove(int x, int y){

    if(runing){
        QMessageBox::information(NULL, "warnings", "operation failed", QMessageBox::Yes);
        return false;
    }

    _powerOperation(x, y, FLAGS_CONDESE, hold);
    path.push_back(Pos(x, y));
    hold ^= FLAG_CHESS;

#ifdef DEBUG_DEFINE
    qDebug("makeMove compelete, val=%d", _evaluate(3-hold));
#endif

    return true;
}

bool ChessBoard::_reMakeMove(){

    if(runing || path.empty()){
        QMessageBox::information(NULL, "warnings", "operation failed", QMessageBox::Yes);
        return false;
    }

    int x = path.last().x;
    int y = path.last().y;
    path.pop_back();
    _powerOperation(x, y, FLAGS_RELEASE, hold);
    hold ^= FLAG_CHESS;

#ifdef DEBUG_DEFINE
    qDebug("undoMove compelete");
#endif

    return true;
}

int ChessBoard::_step(int x, int y){
    int val, prior, winner = -1;
    val = _valueChess(x, y, hold, prior);

    if (prior >= CHESS_PRIOR[2]){
        winner = hold;
    }else if (prior <= -CHESS_PRIOR[2]){
        winner = hold^FLAG_CHESS;
    }
    _makeMove(x, y);
    return winner;
}

int ChessBoard::_valueChess(int x, int y, int key, int &piority){

    int i, j;
    int p[8]={0}, b[8]={0}, bp[8]={0}, bpb[8]={0};

    // p对应方向的子力;
    // b对应方向的子力，之后的空位;
    // bp对应方向的子力，之后的空位，之后的子力;
    // bpb对应方向的子力，之后的空位，之后的子力，之后的空位;

    int two=0, three=0, jump=0, four=0, five=0;
    int sleep_three=0, sleep_two=0, sleep_jump=0;

    // 方向从上方开始，顺时针寻找
    for (i = 0, j = 1; i < 8; ++i, j = 1){
        for(; j <= 5 && _inside(x+vx[i]*j, y+vy[i]*j) && chess[x+vx[i]*j][y+vy[i]*j] == key; ++j, ++p[i]);
        for(; j <= 5 && _inside(x+vx[i]*j, y+vy[i]*j) && chess[x+vx[i]*j][y+vy[i]*j] == 0;   ++j, ++b[i]);
        for(; j <= 5 && _inside(x+vx[i]*j, y+vy[i]*j) && chess[x+vx[i]*j][y+vy[i]*j] == key; ++j, ++bp[i]);
        for(; j <= 5 && _inside(x+vx[i]*j, y+vy[i]*j) && chess[x+vx[i]*j][y+vy[i]*j] == 0;   ++j, ++bpb[i]);
    }

    for (i = 0; i < 4; i++){

        if(p[i] + p[i+4] >= 4) {
            // OOOOO
            five++;
        }

        if(p[i] + p[i+4] == 3){
            // +OOOO+
            if(b[i] >= 1 && b[i+4] >= 1 )       { four += 2;}      // 四连
            // +OOOO
            if(b[i] == 0 && b[i+4] >= 1 )       { four++;}      // 冲四
            if(b[i] >= 1 && b[i+4] == 0 )       { four++;}      // 冲四
        }

        if(p[i] + p[i+4] == 2){
            // OOO+O
            if(b[i]   == 1 && bp[i]   >= 1 )    { four++;} // 单跳四
            if(b[i+4] == 1 && bp[i+4] >= 1 )    { four++;}
            // ++OOO+
            if     (b[i] >= 2 && b[i+4] >= 1 && b[i] + b[i+4] >= 3 && bp[i+4] == 0)   { three++;}   // 活三
            else if(b[i] >= 1 && b[i+4] >= 2 && b[i] + b[i+4] >= 3 && bp[i]   == 0)   { three++;}   // 活三
            // OOO++ // +OOO+
            if(b[i] == 1 && b[i+4] == 1)           { sleep_three++;}    // 眠三
            else if(b[i] == 0 && b[i+4] >= 2)           { sleep_three++;}    // 眠三
            else if(b[i+4] == 0 && b[i] >= 2)           { sleep_three++;}    // 眠三
        }

        if(p[i] + p[i+4] == 1){
            // OO+OO
            if(b[i]   == 1 && bp[i]   >= 2 )    { four++;}   // 单跳四
            if(b[i+4] == 1 && bp[i+4] >= 2 )    { four++;}
            // +OO+O+
            if     (b[i]   == 1 && bp[i]   == 1 && bpb[i] >= 1 && b[i+4] >= 1 && b[i+4] - bp[i+4] > 0)   { jump++;} // 跳三
            else if(b[i+4] == 1 && bp[i+4] == 1 && b[i] >= 1 && bpb[i+4] >= 1 && b[i] - bp[i] > 0)   { jump++;} // 跳三

            // OO+O+ or +OO+O
            if     (b[i]   == 1 && bp[i]   == 1 && bpb[i] + b[i+4] == 1 )   { sleep_jump++;}       // 眠三
            else if(b[i+4] == 1 && bp[i+4] == 1 && b[i] + bpb[i+4] == 1 )   { sleep_jump++;}
            // OO++O
            if     (b[i]   == 2 && bp[i]   >= 1 )                   { sleep_jump++;}
            else if(b[i+4] == 2 && bp[i+4] >= 1 )                   { sleep_jump++;}
            // +++OO++ && ++OO+++
            if (b[i] >= 1 && b[i+4] >= 1 && b[i] + b[i+4] >= 5)  { two++; }       // 活二
            else if (b[i] + b[i+4] <= 5)  { sleep_two++; }       // 眠二
            else if (b[i] == 0 && b[i+4] >= 5)  { sleep_two++; }       // 眠二
            else if (b[i+4] == 0 && b[i] >= 5)  { sleep_two++; }       // 眠二

        }

        if(p[i] + p[i+4] == 0){
            // O+OOO
            if(b[i]   == 1 && bp[i]   >= 3 )    { four++;}
            if(b[i+4] == 1 && bp[i+4] >= 3 )    { four++;}
            // +O+OO+
            if     (b[i]   == 1 && bp[i]   == 2 && bpb[i]   >= 1 && b[i+4] >= 1)   { jump++;}
            else if(b[i+4] == 1 && bp[i+4] == 2 && bpb[i+4] >= 1 && b[i]   >= 1)   { jump++;}

            // O+OO+ && +O+OO
            if((b[i] == 1 && bp[i] == 2 && (bpb[i] >= 1 ||  b[i+4] >= 1)))           { sleep_jump++;}
            else if((b[i+4] == 1 && bp[i+4] == 2 && (bpb[i+4] >= 1 ||  b[i] >= 1)))  { sleep_jump++;}

            // +O+O++
            if(b[i]   >= 2 && b[i+4] == 1 && bp[i+4] == 1 &&  bpb[i+4] >= 1)    { two++;}
            if(b[i+4] >= 2 && b[i]   == 1 && bp[i]   == 1 &&  bpb[i]   >= 1)    { two++;}

            // +O++O+
            if(b[i]   >= 1 && b[i+4] == 2 && bp[i+4] == 1 &&  bpb[i+4] >= 1)    { two++;}
            if(b[i+4] >= 1 && b[i]   == 2 && bp[i]   == 1 &&  bpb[i]   >= 1)    { two++;}

            // O+O++ or ++O+O
            if(b[i]   >= 2 && b[i+4] == 1 && bp[i+4] == 1 &&  bpb[i+4] == 0)    { sleep_two++;}
            if(b[i+4] >= 2 && b[i]   == 1 && bp[i]   == 1 &&  bpb[i]   == 1)    { sleep_two++;}
            if(b[i]   == 0 && b[i+4] == 1 && bp[i+4] == 1 &&  bpb[i+4] >= 2)    { sleep_two++;}
            if(b[i+4] == 0 && b[i]   == 1 && bp[i]   == 1 &&  bpb[i]   >= 2)    { sleep_two++;}
        }
    }

    piority = CHESS_PRIOR[0]*(jump+three) +
              CHESS_PRIOR[1]*four +
              CHESS_PRIOR[2]*five;

    if (five >= 1)
        return UNION_VALUE[3];

    if (four >= 2)
        return UNION_VALUE[2];

    if (four >= 1 && jump+three >= 1)
        return UNION_VALUE[1];

    if (jump+three >= 2)
       return UNION_VALUE[0];

    return CHESS_VALUE[0]*sleep_two + CHESS_VALUE[1]*sleep_three +
           CHESS_VALUE[2]*sleep_jump + CHESS_VALUE[3]*two +
           CHESS_VALUE[4]*jump + CHESS_VALUE[5]*three +
           CHESS_VALUE[6]*four + add[x][y];
}

int ChessBoard::_evaluate(int key){

    int p, o_val=0, d_val=0, o_pri=0, d_pri=0;

    FF(0, size, 0, size){
        if (chess[i][j] == 3-key){
            o_val = _Max(o_val, _valueChess(i, j, key^FLAG_CHESS, p));
            o_pri = _Max(o_pri, p);
        }
        if (chess[i][j] == key)
        {
            d_val = _Max(d_val, _valueChess(i, j, key, p));
            d_pri = _Max(d_pri, p);
        }
    }
    // 后手方五连
    if (d_pri >= CHESS_PRIOR[2])
        return R_INF;
    // 先手方五连
    if (o_pri >= CHESS_PRIOR[2])
        return -R_INF;
    // 先手方有活三，后手方无冲四
    if(o_pri > 0 && o_pri < CHESS_PRIOR[1] && d_pri < CHESS_PRIOR[1])
        return -R_INF;
    // 先手方有冲四，后手方无五连
    if(o_pri >= CHESS_PRIOR[1] && o_pri < CHESS_PRIOR[2] && d_pri < CHESS_PRIOR[2])
        return -R_INF;

    if(d_pri >= 1)  d_val *=1.5;
    if(o_pri >= 1)  o_val *=1.5;
    return d_val - o_val*1.5;
}

void ChessBoard::_powerOperation(int x, int y, int flag, int key){

    int dx, dy;
    hash ^= Z[key][x][y];

    if (flag == FLAGS_CONDESE){
        order++;
        chess[x][y] = key;
        FF(0, 8, 1, 3){
            dx = x + vx[i]*j;
            dy = y + vy[i]*j;
            if(_inside(dx, dy))
                vis[dx][dy]++;
        }
//        FF(x-2, x+3, y-2, y+3){
//            if(_inside(i, j))
//                vis[i][j]++;
//        }


    }
    else{
//        FF(x-2, x+3, y-2, y+3){
//            if(_inside(i, j))
//                vis[i][j]--;
//        }
        FF(0, 8, 1, 3){
            dx = x + vx[i]*j;
            dy = y + vy[i]*j;
            if(_inside(dx, dy))
                vis[dx][dy]--;
        }
        order--;
        chess[x][y] &= 0;
    }
}

uint64_t ChessBoard::_rand64(){
    return (uint64_t)(rand() ^ (rand() << 15) ^ (rand() << 30) ^ (rand() << 45)) ;
}

bool ChessBoard::_lookup(int depth, int alpha, int beta, Pos& ret){

    uint64_t index = hash & (HASH_TABLE_SIZE - 1);

    if(H[index]._checknum == hash && H[index]._move.a2 >= depth){

        target++;

        if(H[index]._flag == HASH_EXACT){
            ret = H[index]._move;
        }else if(H[index]._flag == HASH_ALPHA && H[index]._move.val >= alpha){
            ret = H[index]._move;
        }else if(H[index]._flag == HASH_BETA  && H[index]._move.val <=  beta){
            ret = H[index]._move;
        }else
            return false;

        return true;
    }
    return false;
}

bool ChessBoard::_store(int hashf, uint64_t hashIndex, Pos ret, int deep){

    QMutexLocker lock(&m_hash);
    uint64_t index = hashIndex & (HASH_TABLE_SIZE - 1);

    if(H[index]._move.a2 < deep || H[index]._flag < 0 || H[index]._flag > 3 && _inside(ret)){

        H[index]._checknum = hashIndex;
        H[index]._flag = hashf;
        H[index]._move = ret;
        H[index]._move.a2 = deep;
        return true;
    }

    return false;
}

void ChessBoard::reStart(){
    delete H;
    H = new HashItem[HASH_TABLE_SIZE]();
}

AI::AI(bool _openLog, int *args){
    openLog = _openLog;
    time_kill = args[0];
    time_mtdf = args[1];
    branches = args[2];
    p = ChessBoard::_getInstance();
    obj = this;

#ifdef DEBUG_DEFINE
    qDebug("create AI class complete");
#endif

}

AI::~AI(){
    delete p;
    delete obj;
}

AI *AI::_getInstance(){
    if(obj == nullptr) return nullptr;
    else return obj;
}

void AI::deepening(QString *buffer){

    int pri;
    p->runing = true;
    timer.start();

#ifdef DEBUG_DEFINE
    qDebug("deepening mtdf start up");
#endif

    Pos newMove;
    QVector<Pos> killer(20, Pos());
    for(depth = 2; depth < depth_max; depth+=2)
    {
        newMove = Pos();
        if(p->runing){

            p->target = 0;
            MTD(newMove, best.val, depth, killer);


#ifdef DEBUG_DEFINE
    qDebug("MTD: depth=%d, [%d,%d]=%d, tag=%d\n", depth, newMove.x, newMove.y, newMove.val, p->target);
#endif

            if(openLog && p->runing){
                QString tmp;
                tmp.sprintf("MTD: depth=%d, [%d,%d]=%d, tag=%d\n",
                            depth, newMove.x, newMove.y, newMove.val, p->target);
                *buffer += tmp;
            }

            if(!p->_inside(newMove)){
                p->runing = false;
            }

            if(p->runing && depth%2 == 0){
                best = newMove;
                p->_valueChess(newMove.x, newMove.y, 3-(p->hold), pri);
                if(newMove.val >= p->UNION_VALUE[3] || pri >= p->UNION_VALUE[3]){
                    p->runing = false;
                }
            }

        }
    }
    QString tmp;
    tmp.sprintf("count: %d, ABcut=%d\n\n", Count, ABcut);
    Count = ABcut = 0;
    *buffer += tmp;
    p->runing = false;

    int p1, p2;
    QVector<Pos> res;
    if(!p->_inside(best)){

#ifdef DEBUG_DEFINE
        qDebug("correct:[%d,%d]", best.x, best.y);
#endif

        FF(0, p->size, 0, p->size){
            if(p->vis[i][j] >= 1 && p->chess[i][j] == 0){
                p->_valueChess(i, j, (p->hold), p1);
                p->_valueChess(i, j, 3-(p->hold), p2);
                res.push_back(Pos(i, j, _Max(2*p1, p2)));
            }
        }
        qSort(res.begin(), res.end(), [](Pos &a, Pos &b){
            return a.val > b.val;
        });
        best = res[0];



    }

}

void AI::update(Pos &ret, Pos &obj){

    QMutexLocker locker(&m_ai);

    if(obj.a2 == 0 && (obj.val > ret.val || ret.a3 == 0)){
        if(openLog){
            qDebug("[%d,%d,%d]->[%d,%d,%d]",
                   ret.x, ret.y, ret.a2, obj.x, obj.y, obj.a2);
        }
        ret.x = obj.x;
        ret.y = obj.y;
        ret.val = obj.val;
        ret.a3++;
    }
}

void AI::MTD(Pos &ret, int f, int deep, QVector<Pos>& killer){
    int alpha, beta, best_value, test, speed[2]={0};
    QVector<Pos> line;
    Pos newMove;

#ifdef DEBUG_DEFINE
    qDebug("[MTD, depth=%d, start up]", deep);
#endif

    test = f;               // 试探值
    alpha = - p->R_INF;     // 下界
    beta  = + p->R_INF;     // 上界

    do{
        best_value = MT(newMove, p->hold, deep, test-1, test, line, killer);

        if(best_value < test){
            // alpha 结点
            beta = best_value;
            speed[1]=0;
//            if(++speed[0] > 3)
//                test = (alpha+beta) >> 1;
//            else
                test = best_value;

        }else{
            // beta 结点
            alpha = best_value;
            if(p->runing){
                ret = newMove;
                ret.val = alpha;
            }
//            speed[0]=0;
//            if(++speed[1] > 3)
//                test = ((alpha+beta) >> 1) + 1;
//            else
                test = best_value+1;
        }

#ifdef DEBUG_DEFINE
        qDebug("[%d,%d]", alpha, beta);
#endif

    }while(alpha < beta);
}

int AI::MT(Pos &ret, int key, int deep, int alpha, int beta, QVector<Pos> &path, QVector<Pos>& killer){

    int i, p1, p2, cur=-(p->R_INF), k;
    int hashf = p->HASH_ALPHA;
    uint64_t hashIndex=0, hashBest;
    Pos newMove, bestMove;
    QVector<Pos> vec_moves, attackQueue;

    // 判断运算状态
    if(!p->runing){
        return alpha;
    }

    // 查找哈希表
    if(p->_lookup(deep, alpha, beta, newMove)){
        newMove.a2 = depth - deep;
        update(ret, newMove);
        return newMove.val;
    }

    // 检查游戏是否结束
    if(depth > deep && path.last().a1 >= 10000){
        return cur;
    }

    // 生成合适着法
    FF(0, p->size, 0, p->size){
        if (p->vis[i][j] >= 2 && p->chess[i][j] == 0){
            Count++;
            k = 1.1*p->_valueChess(i, j, key, p1) + p->_valueChess(i, j, 3-key, p2);
            attackQueue.push_back(Pos(i, j, k, p1, depth-deep, _Max(p1, p2)));
        }
    }

    qSort(attackQueue.begin(), attackQueue.end(), [](Pos &a, Pos &b){
        return a.val > b.val;
    });

    if(attackQueue[0].val >= 10000){
        // 下一手形成五连
        vec_moves.push_back(attackQueue[0]);
    }

    else if(attackQueue[0].val >= 1100){
        // 下一手形成己方活四
        for(i = 0; i < attackQueue.size() && attackQueue[i].val >= 1100; i++)
            vec_moves.push_back(attackQueue[i]);
    }

    else if(attackQueue[0].val >= 1000){
        // 下一手形成对方活四，收集防守和进攻棋型
        for(i = 0; i < attackQueue.size() && attackQueue[i].val >= 1000; i++)
            vec_moves.push_back(attackQueue[i]);
        // 找己方的冲四点
        for(; i < attackQueue.size(); i++){
            p->_valueChess(attackQueue[i].x, attackQueue[i].y, key, p1);
            if(p1 >= 100) vec_moves.push_back(attackQueue[i]);
        }
    }

    else{
        // 正常处理
        vec_moves.swap(attackQueue);

//        // 杀手启发
//        if(p->_isEmpty(killer[deep]) && p->_inside(killer[deep])){
//            killer[deep].a2 = depth - deep;
//            vec_moves.push_front(killer[deep]);
//        }
    }

    // 遍历搜索树
    for(Pos& move: vec_moves){

        hashIndex = p->hash;
        p->_powerOperation(move.x, move.y, p->FLAGS_CONDESE, key);
        path.push_back(move);

        if(deep > 1)
            move.val = - MT(ret, 3-key, deep-1, -beta, -alpha, path, killer);
        else{
            move.val = p->_evaluate(key);
            p->_store(p->HASH_EXACT, p->hash, move, 1);
        }

        p->_powerOperation(move.x, move.y, p->FLAGS_RELEASE, key);
        path.pop_back();

        if(move.val > cur){
            cur = move.val;
            if(move.val > alpha){
                alpha = move.val;
                hashf = p->HASH_EXACT;
                hashBest = hashIndex;
                bestMove = move;
                update(ret, move);
            }
            if(move.val >= beta){
                ABcut++;
                if(p->runing) p->_store(p->HASH_BETA, hashIndex, move, deep);
                killer[deep] = move;
                return move.val;
            }
        }
    }
    if(p->runing) p->_store(hashf, hashBest, bestMove, deep);
    return cur;
}

int AI::killSearch(Pos& ret, int origin, int key, int deep, int alpha, int beta, QVector<Pos>& path)
{
    int p1, p2, k, three=0;
    int hashf = p->HASH_ALPHA;
    uint64_t hashIndex=0, hashBest=0;
    Pos newMove, bestMove;
    QVector<Pos> attackQueue, vec_moves;

    if(!p->runing){
        return alpha;
    }

    if(p->_lookup(deep, alpha, beta, newMove)){
        newMove.a2 = depth - deep;
        update(ret, newMove);
        return newMove.val;
    }

    FF(0, p->size, 0, p->size){
        if(p->chess[i][j] == 0 && p->vis[i][j] >= 1){
            // 算杀的结点选择
            // 进攻方：活三、冲四、活四、五连、防守对方的冲四
            // 防守方：防守对方的活三、冲四，自身的冲四
            k = 2*p->_valueChess(i, j, key, p1) + p->_valueChess(i, j, 3-key, p2);
            if(origin == key){
                // 进攻方选点
                if(p1 > 0 || p2 >= 10000){
                    attackQueue.push_back(Pos(i, j, k, p1, depth-deep));
                }
            }else{
                // 防守方选点
                if(p2 > 0 || p1 >= 100){
                    attackQueue.push_back(Pos(i, j, k, p1, depth-deep));
                }
            }
        }
    }

    while(!attackQueue.isEmpty()){

        // 取出尾结点，并从队列中删除
        newMove = attackQueue.last();
        attackQueue.pop_back();

        // 执行试探测试
        if(newMove.a1 > 0) three++;
        p->_powerOperation(newMove.x, newMove.y, p->FLAGS_CONDESE, key);
        newMove.val = p->_evaluate(key);
        p->_powerOperation(newMove.x, newMove.y, p->FLAGS_RELEASE, key);

        if (newMove.val <= -0.5*p->R_INF) continue;

        vec_moves.push_back(newMove);
    }

    // 如果进攻方没找到结点，情况如下：
    // 1.局面没有进攻结点了，表明进攻失败，应该返回alpha。
    // 2.进攻过程中遭到反击，如反活三，说明进攻不能成功，应该返回alpha。
    // 如果防守方没找到结点，情况如下：
    // 1.无法阻挡攻势，应该返回-INF。

    // 第一层搜索无活三
    if(origin == key && three == 0 && topFlag)
        return alpha;
    // 进攻方无棋
    if(origin == key && vec_moves.isEmpty())
        return alpha;
    // 防守方无棋
    if(origin == 3-key && vec_moves.isEmpty())
        return -(p->R_INF);

    qSort(vec_moves.begin(), vec_moves.end(), [](Pos &a, Pos &b){
        return a.val > b.val;
    });

    if(origin == key && vec_moves[0].a1 >= 10000){
        update(ret, vec_moves[0]);
        return beta;
    }

    if(topFlag) topFlag = false;
    for(Pos& move: vec_moves){

        p->_powerOperation(move.x, move.y, p->FLAGS_CONDESE, key);
        path.push_back(move);

        if(deep > 1)
            move.val = - killSearch(ret, origin, 3-key, deep-1, -beta, -alpha, path);
        else{
            move.val = p->_evaluate(key);
            if(p->runing) p->_store(p->HASH_EXACT, p->hash, move, deep);
        }

        hashIndex = p->hash;
        path.pop_back();
        p->_powerOperation(move.x, move.y, p->FLAGS_RELEASE, key);

        if(move.val > alpha){
            alpha = move.val;
            hashf = p->HASH_EXACT;
            hashBest = hashIndex;
            bestMove = move;
            update(ret, move);
        }

        if (move.val >= beta){
            return move.val;
        }
    }
    return alpha;
}

Widget::Widget(QWidget *parent):QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);
    timerIndex = QObject::startTimer(300);

    s[0] = '_';
    s[1] = 'X';
    s[2] = 'O';

    mem["0"] = 0;
    mem["1"] = 1;
    mem["2"] = 2;
    mem["3"] = 3;
    mem["4"] = 4;
    mem["5"] = 5;
    mem["6"] = 6;
    mem["7"] = 7;
    mem["8"] = 8;
    mem["9"] = 9;

    mem["A"] = 10; mem["a"] = 10;
    mem["B"] = 11; mem["b"] = 11;
    mem["C"] = 12; mem["c"] = 12;
    mem["D"] = 13; mem["d"] = 13;
    mem["E"] = 14; mem["e"] = 14;

    lcd[0] = "0";
    lcd[1] = "1";
    lcd[2] = "2";
    lcd[3] = "3";
    lcd[4] = "4";
    lcd[5] = "5";
    lcd[6] = "6";
    lcd[7] = "7";
    lcd[8] = "8";
    lcd[9] = "9";
    lcd[10] = "A";
    lcd[11] = "B";
    lcd[12] = "C";
    lcd[13] = "D";
    lcd[14] = "E";

    p = ChessBoard::_getInstance();
    p->change = true;

    int args[3] = {3000, 7000, 30};
    ai = new AI(true, args);
    ai->buffer = &buffer;
}

Widget::~Widget()
{
    delete ui;
    delete p;
}

void Widget::timerEvent(QTimerEvent *){

    if(p != nullptr && p->change){
        p->change = false;
        board.clear();
        for(int x=0; x < p->size; x++){
            for(int y=0; y < p->size; y++){
                board += s[p->chess[x][y]] + " ";
            }
            board += "\n";
        }
        ui->Display->setPlainText(board);
    }

    if(buffer_change && ai != nullptr){
        ui->Message->setPlainText(*(ai->buffer));
        QTextCursor cursor = ui->Message->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->Message->setTextCursor(cursor);
    }
}

void Widget::on_make_move_clicked()
{
    if(p->runing || (ui->pos_wnd->text().isEmpty() && ui->cal_wnd->text().isEmpty())) return;

    QString tmp = ui->pos_wnd->text();
    int x, y;

    if(tmp.isEmpty()){
        tmp = ui->cal_wnd->text();
        x = mem[tmp.section(".",0,0)];
        y = mem[tmp.section(".",1,1)];
    }else{
        x = mem[tmp.section(".",0,0)];
        y = mem[tmp.section(".",1,1)];
    }



#ifdef DEBUG_DEFINE
    qDebug("makemove_button_clicked");
#endif

    int winner;
    if(p->_inside(x, y) && p->chess[x][y] == 0){
        p->change = true;
        winner = p->_step(x, y);
        if (winner == p->BLACK){
            QMessageBox::information(NULL, "tips", "BLACK are winner", QMessageBox::Yes);
            exit(0);
        }else if (winner == p->WHITE){
            QMessageBox::information(NULL, "tips", "WHITE are winner", QMessageBox::Yes);
            exit(0);
        }else if (p->order == p->size*p->size){
            QMessageBox::information(NULL, "tips", "tie", QMessageBox::Yes);
            exit(0);
        }
    }else{
        QMessageBox::information(NULL, "warnings", "failed", QMessageBox::Yes);
    }
    ui->pos_wnd->clear();
}

void Widget::on_calculate_clicked()
{

#ifdef DEBUG_DEFINE
    qDebug("calculate_clicked");
#endif

    int loc[2];
    if(p->order == 0){
        ai->best.x = p->size/2;
        ai->best.y = p->size/2;
        ui->cal_wnd->setText(QString(lcd[ai->best.x] +"."+lcd[ai->best.y]));
    }else if(p->order == 1){
        do{
            loc[0] = p->path.last().x + rand()%3 - p->order;
            loc[1] = p->path.last().y + rand()%3 - p->order;
        }while(!(p->_inside(loc[0], loc[1]) && p->chess[loc[0]][loc[1]] == p->EMPTY));
        ai->best.x = loc[0];
        ai->best.y = loc[1];
        ui->cal_wnd->setText(QString(lcd[ai->best.x] +"."+lcd[ai->best.y]));
    }else{

        ui->calculate->setText(tr("停止计算"));

        if(p->runing){
            ui->calculate->setText(tr("开始计算"));
            qDebug("quit calculate thread");
            p->runing = false;
            return;
        }

        qDebug("create calculate thread");
        p->runing = true;
        ui->cal_wnd->clear();

        if(ai != nullptr){
            ai->best.a3 = 0;
            p->reStart();
        }

        AI_Thread *ai_thread = new AI_Thread();
        ai_thread->ai = ai;
        ai_thread->moveToThread(&t);
        ai_thread->connect(&t, &QThread::finished, ai_thread, &QThread::deleteLater);
        ai_thread->connect(ai_thread, &AI_Thread::resultReady, this, &Widget::recive);
        ai_thread->connect(this, &Widget::startThread, ai_thread, &AI_Thread::run);
        t.start();
        emit startThread("mtdf");
        buffer_change = true;
    }
}

void Widget::on_clear_btn_clicked()
{
    buffer.clear();
    buffer_change = true;
}

void Widget::recive(const QString str){

#ifdef DEBUG_DEFINE
    qDebug("recive finish signal");
#endif

    t.quit();
    t.wait();
    buffer_change = false;
    ui->calculate->setText(tr("开始计算"));
    ui->cal_wnd->setText(QString(lcd[ai->best.x] +"."+lcd[ai->best.y]));
}

AI_Thread::AI_Thread(QObject *parent) : QObject(parent){
    isStop = false;
}

AI_Thread::~AI_Thread(){

}

void AI_Thread::run(const QString str){

    if(!isStop){

        if (str == "mtdf"){

#ifdef DEBUG_DEFINE
    qDebug("execute mtdf algorithm");
#endif

            ai = AI::_getInstance();
            if (ai == nullptr){
                isStop = true;
                return;
            }

            ai->deepening(ai->buffer);
            isStop = true;
            emit resultReady("over");
        }

        if (str == "kill"){

        }

    }else{
        QThread::sleep(1);
    }


}

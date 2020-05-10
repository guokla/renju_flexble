#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "include.h"

struct Pos{
    int x, y, val;
};

class ChessBoard
{
public:
    // 常量定义
    const int BLACK = 1;
    const int WHITE = 2;
    int size = 15;
    const int Kernel = 2;

    const int R_INF = 1000;
    const int HASH_EXACT = 1;
    const int HASH_ALPHA = 2;
    const int HASH_BETA  = 3;
    uint64_t HASH_TABLE_SIZE = 20;
    const int FLAGS_CONDESE = 1;
    const int FLAGS_RELEASE = 2;

    // 眠二，眠三，活二，跳活三，直活三，冲四
    // 双活三，冲四活三，双冲四（活四），五连
    int CHESS_VALUE[10]={1,2,3,3,5,5,50,80,120,200};
    int CHESS_PRIOR[4]={1,2,100,10000};

    // 方向矢量
    const int vx[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
    const int vy[8] = {-1,-1, 0, 1, 1, 1, 0,-1};

    int chess[20][20];              // 棋盘数组
    uint64_t Z[3][20][20];          // 散列表
    uint64_t hash = 0;              // 散列哈希值
    int vis[20][20];                // 棋子能量分布
    int hold = 1;                   // 当前走棋方
    int order = 0;                  // 棋子数量
    bool runing = false;            // 运算状态
    QVector<Pos> path;              // 走棋路径

public:
    ChessBoard(); // 私有构造函数
    ChessBoard(ChessBoard &obj);
    ChessBoard& operator=(const ChessBoard&);
    static ChessBoard* pChess;
    static QMutex m_hash;                  // 读写锁
    static QMutex m_move;                  // 落子锁

public:
    ~ChessBoard();
    // 单例模式
    static ChessBoard *_getInstance();

    // 常用函数
    bool _makeMove(int x, int y);
    bool _reMakeMove();

    // 越界判断函数
    bool inline _inside(int x, int y);

    // 局面计算函数
    int _valueChess(int x, int y, int key, int &piority);
    int _evaluate(int key);
    void _powerOperation(int x, int y, int flag, int key);

    // 哈希表操作
    uint64_t _rand64();
    bool _lookup(int depth, int alpha, int beta, Pos& ret);
    bool _store(int hashf, long long hashIndex, const Pos ret, int deep);
};

ChessBoard* ChessBoard::pChess = nullptr;
QMutex ChessBoard::m_hash;                  // 读写锁
QMutex ChessBoard::m_move;                  // 落子锁

#endif // CHESSBOARD_H

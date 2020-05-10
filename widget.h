#ifndef WIDGET_H
#define WIDGET_H

#define DEBUG_DEFINE

#include <QTime>
#include <QMutex>
#include <QMutexLocker>
#include <QMessageBox>
#include <QWidget>
#include <QMap>
#include <assert.h>
#include <QThread>

namespace Ui {
    class Widget;
}

#define FF(a,b,c,d) for(int i=(a); i<(b); i++)for(int j=(c); j<(d); j++)

#define _Max(a,b) ((a)>(b))?(a):(b)

#define _Min(a,b) ((a)<(b))?(a):(b)

struct Pos{
    int x, y, val, a1, a2, a3; // 价值，权值，次序，保留
    Pos(int _x=-1, int _y=-1, int _val=0, int _pri=0, int _ord=0, int _res=0):x(_x), y(_y){
        val = _val; a1 = _pri; a2 = _ord; a3 = _res;
    }
    bool operator <(const Pos& obj){
        return val < obj.val;
    }
    bool operator >(const Pos& obj){
        return val > obj.val;
    }
};

struct HashItem{

    uint64_t _checknum;
    Pos _move;
    int _flag;

    HashItem(): _checknum(0),_flag(-1){

    }
};

class ChessBoard
{
public:
    // 常量定义
    const int EMPTY = 0;
    const int BLACK = 1;
    const int WHITE = 2;

    const int FLAG_CHESS = BLACK ^ WHITE;
    int size = 7;
    const int MAX_SIZE = 20;
    const int KERNEL_SIZE = 3;

    const int R_INF = 10000;
    const int HASH_EXACT = 1;
    const int HASH_ALPHA = 2;
    const int HASH_BETA  = 3;
    uint64_t HASH_TABLE_SIZE = 1 << 21;
    const int FLAGS_CONDESE = 1;
    const int FLAGS_RELEASE = 2;

    static int CHESS_VALUE[7];
    static int UNION_VALUE[4];
    static int CHESS_PRIOR[3];

    // 方向矢量
    const int vx[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
    const int vy[8] = {-1,-1, 0, 1, 1, 1, 0,-1};

    int chess[20][20];              // 棋盘数组
    uint64_t Z[3][20][20];          // 散列表
    uint64_t hash = 0;              // 散列哈希值
    int vis[20][20];                // 棋子能量分布
    int add[20][20];                // 附加值
    int hold = BLACK;               // 当前走棋方
    int minX=size, minY=size;       // 最小的棋子位置
    int order = 0;                  // 棋子数量
    volatile bool runing = false;   // 运算状态
    bool change = true;             // 绘图状态
    int target = 0;
    QVector<Pos> path;              // 走棋路径

private:
    ChessBoard();// 私有构造函数
    ChessBoard(ChessBoard &obj);
    ChessBoard* operator=(const ChessBoard&);
    static ChessBoard* pChess;
    HashItem *H = nullptr;
    static QMutex m_hash;                  // 读写锁
    static QMutex m_move;                  // 落子锁

public:
    ~ChessBoard();
    // 单例模式
    static ChessBoard *_getInstance();


    // 常用函数
    bool _makeMove(int x, int y);
    bool _reMakeMove();
    int _getPosition(QVector<Pos> &vec, int branches);
    int _step(int x, int y);
    void reStart();


    // 越界判断函数
    bool inline _inside(int x, int y);
    bool inline _inside(const Pos& move);
    bool inline _isEmpty(int x, int y);
    bool inline _isEmpty(const Pos& move);

    // 局面计算函数
    int _valueChess(int x, int y, int key, int &piority);
    int _evaluate(int key);
    void _powerOperation(int x, int y, int flag, int key);

    // 哈希表操作
    uint64_t _rand64();
    bool _lookup(int depth, int alpha, int beta, Pos& ret);
    bool _store(int hashf, uint64_t hashIndex, const Pos ret, int deep);

};

class AI
{
public:
    static AI *obj;
    ChessBoard *p = nullptr;
    int time_mtdf = 7000;
    int time_kill = 3000;
    int branches = 30;
    int ABcut = 0;
    int Count = 0;
    int depth;
    int depth_max = 31;
    bool openLog = false;
    bool topFlag;
    Pos best;
    QVector<Pos> bestLine;
    QMutex m_ai;
    QTime timer;
    QString *buffer;

public:

    AI(bool _openLog, int *args);
    ~AI();

    static AI *_getInstance();

    void deepening(QString *buffer);
    void update(Pos& ret, Pos &obj);
    void MTD(Pos &ret, int f, int deep, QVector<Pos>& killer);
    int MT(Pos &ret, int key, int deep, int alpha, int beta, QVector<Pos>& path, QVector<Pos>& killer);
    int killSearch(Pos& ret, int origin, int key, int deep, int alpha, int beta, QVector<Pos>& path);

};

class AI_Thread : public QObject
{
    Q_OBJECT

public:
    explicit AI_Thread(QObject *parent = nullptr);
    ~AI_Thread();

signals:
    void resultReady(const QString str);

public slots:
    void run(const QString str);

public:
    volatile bool isStop;
    AI *ai = nullptr;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    void timerEvent(QTimerEvent *);

signals:
    void startThread(const QString str);

public slots:
    void recive(const QString str);

private slots:
    void on_make_move_clicked();
    void on_calculate_clicked();
    void on_clear_btn_clicked();

private:
    Ui::Widget *ui;
    QThread t;
    int timerIndex, size = 15;
    ChessBoard *p = nullptr;
    AI *ai = nullptr;
    QString board, buffer, s[4];
    QMap<QString, int> mem;
    QMap<int, QString> lcd;
    bool buffer_change = false;
};

#endif // WIDGET_H

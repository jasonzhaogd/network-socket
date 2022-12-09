/**
 * 多线程服务器端程序虽然可以正常工作，不过它有一个缺点，那就是如果并发连接过多，就会引起线程的频繁创建和销毁。
 * 虽然线程切换的上下文开销不大，但是线程创建和销毁的开销却是不小的。
 *
 * 可以使用预创建线程池的方式来进行优化。在服务器端启动时，可以先按照固定大小预创建出多个线程，
 * 当有新连接建立时，往连接字队列里放置这个新连接描述字，线程池里的线程负责从连接字队列里取出连接描述字进行处理。
 *
 * 除了连接池，还有一个关键是连接字队列的设计，因为这里既有往这个队列里放置描述符的操作，也有从这个队列里取出描述符的操作。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include "../00-lib/error.h"

#define BUF_SIZE 30

#define THREAD_POOL_SIZE 2
#define QUEQUE_SIZE 100

void *thread_run(void *arg);
void do_echo(int fd);

// 定义一个队列
typedef struct
{
    int capacity;          // 队列容量
    int *fd;               // fd数组指针
    int head;              // 队列头位置
    int tail;              // 队列尾位置
    pthread_mutex_t mutex; // 互斥锁
    pthread_cond_t cond;   // 条件变量
} block_queue;

// 初始化队列
void init_block_queue(block_queue *queue, int cap)
{
    queue->capacity = cap;
    queue->fd = calloc(QUEQUE_SIZE, sizeof(int));
    queue->head = 0;
    queue->tail = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

// 往队列里放一个fd
void push_fd(block_queue *queue, int fd)
{
    pthread_mutex_lock(&queue->mutex);
    queue->fd[queue->tail] = fd;
    printf("push fd %d\n", fd);
    if (++queue->tail >= queue->capacity)
        queue->tail = 0;
    // 上面的判断和处理其实是做成了一个 循环队列，
    // todo 但是循环队列还有一个很重要的逻辑没有处理，就是尾部覆盖了头部

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

// 从队列里拿出一个fd
int pop_fd(block_queue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    while (queue->head == queue->tail)
        pthread_cond_wait(&queue->cond, &queue->mutex);
    int fd = queue->fd[queue->head];
    if (++queue->head >= queue->capacity)
        queue->head = 0;

    printf("pop fd %d\n", fd);
    pthread_mutex_unlock(&queue->mutex);

    return fd;
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    // char buf[BUF_SIZE];
    // int str_len;

    // 准备队列
    block_queue queue;
    init_block_queue(&queue, QUEQUE_SIZE);

    // 准备线程池
    pthread_t *thread_pool = calloc(THREAD_POOL_SIZE, sizeof(pthread_t));
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_run, (void *)&queue);
    }

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // 打开 SO_REUSEADDR
    int option = 1;
    int optlen = sizeof(option);
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen error");

    while (1)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            continue;
        else
            puts("new client connected......");

        push_fd(&queue, clnt_sock);
    }

    close(serv_sock);
    return 0;
}

void *thread_run(void *arg)
{
    // 把自己分离，自己负责资源回收
    pthread_t tid = pthread_self();
    pthread_detach(tid);

    while (1)
    {
        int fd = pop_fd((block_queue *)arg);
        printf("get fd in thread, fd==%d, tid == %lu \n", fd, tid);
        do_echo(fd);
    }

    return 0;
}

void do_echo(int fd)
{
    char buf[BUF_SIZE];
    int str_len;

    while ((str_len = read(fd, buf, BUF_SIZE)) > 0)
    {
        buf[str_len] = '\0';
        printf("(%lu) Message from client: %s", (unsigned long)pthread_self(), buf);
        write(fd, buf, str_len);
    }
    if (str_len < 0)
        perror("read error");
    close(fd);
    puts("client disconnected...");
}

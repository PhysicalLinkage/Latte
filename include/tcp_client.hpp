#include <mtcp.hpp>



static int TCP_CLIENT_TEST()
{
    int sd;
    int acc_sd;
    struct sockaddr_in addr;

    socklen_t sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in from_addr;

    char buf[2048];

   // 受信バッファの初期化
    memset(buf, 0, sizeof(buf));

    // IPv4 TCP のソケットを作成
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // 待ち受けるIPとポート番号を設定
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4889);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // バインドする
    if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    // パケット受信待ち状態とする
    // 待ちうけキューを１０としている
    if(listen(sd, 10) < 0) {
        perror("listen");
        return -1;
    }

    // クライアントからコネクト要求が来るまで停止する
    // 以降、サーバ側は acc_sd を使ってパケットの送受信を行う
    if((acc_sd = accept(sd, (struct sockaddr *)&from_addr, &sin_size)) < 0) {
        perror("accept");
        return -1;
    }

    // パケット受信。パケットが到着するまでブロック
    if(recv(acc_sd, buf, sizeof(buf), 0) < 0) {
        perror("recv");
        return -1;
    }

    // パケット送受信用ソケットのクローズ
    close(acc_sd);

    // 接続要求待ち受け用ソケットをクローズ
    close(sd);

    // 受信データの出力
    printf("%s\n", buf);

    return 0;
}


#include <mtcp.hpp>



static int TCP_CLIENT_TEST()
{
    int sd;
    int acc_sd;
    struct sockaddr_in addr;

    socklen_t sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in from_addr;

    char buf[2048];

   // �����Хåե��ν����
    memset(buf, 0, sizeof(buf));

    // IPv4 TCP �Υ����åȤ����
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // �Ԥ�������IP�ȥݡ����ֹ������
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4889);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // �Х���ɤ���
    if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    // �ѥ��åȼ����Ԥ����֤Ȥ���
    // �Ԥ��������塼�򣱣��Ȥ��Ƥ���
    if(listen(sd, 10) < 0) {
        perror("listen");
        return -1;
    }

    // ���饤����Ȥ��饳�ͥ����׵᤬���ޤ���ߤ���
    // �ʹߡ�������¦�� acc_sd ��Ȥäƥѥ��åȤ���������Ԥ�
    if((acc_sd = accept(sd, (struct sockaddr *)&from_addr, &sin_size)) < 0) {
        perror("accept");
        return -1;
    }

    // �ѥ��åȼ������ѥ��åȤ����夹��ޤǥ֥�å�
    if(recv(acc_sd, buf, sizeof(buf), 0) < 0) {
        perror("recv");
        return -1;
    }

    // �ѥ��å��������ѥ����åȤΥ�����
    close(acc_sd);

    // ��³�׵��Ԥ������ѥ����åȤ򥯥���
    close(sd);

    // �����ǡ����ν���
    printf("%s\n", buf);

    return 0;
}


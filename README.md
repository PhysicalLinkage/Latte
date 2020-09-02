# ゲームサーバー

## 開始方法

```bash
git clone https://github.com/PhysicalLinkage/Latte.git
cd Latte
bash docker/build.sh latte
bash docker/create.sh latte
docker start latte
docker attach latte
```
コンテナ内で
```bash
bash ql test-cpp include/gachi_land_server.hpp
```

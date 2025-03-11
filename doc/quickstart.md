# 快速启动指南
----------

## 启动太阴（taiyin）

使用docker来启动：

```sh
docker run \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-default \
    --restart unless-stopped zuowangdaox/taiyi
```

### 低内存节点

上面的指令就启动了一个低内存节点，这种节点适用于:
- 种子节点
- 司命节点（也是出块节点）
- 资源交易节点

对于启动一个完全API的节点，运行：

```
docker run \
    --env USE_WAY_TOO_MUCH_RAM=1 --env USE_FULL_WEB_NODE=1 \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-full \
    --restart unless-stopped \
    zuowangdaox/taiyi
```

## 将节点数据挂载到储存系统

只需要通过 [`-v` ](https://docs.docker.com/reference/cli/docker/container/run/#volume)参数将容器内的 `/var/taiyi` 目录挂载到主机的指定目录即可

```sh
docker run \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-default \
    -v ~/taiyi-mount:/var/taiyi \
    --restart unless-stopped zuowangdaox/taiyi
``` 

## 根据需求配置示例

在运行容器的时候提供环境变量即可配置 taiyin 的启动参数

### 完全API节点

你需要像上述指令一样使用 `USE_WAY_TOO_MUCH_RAM=1` 和 `USE_FULL_WEB_NODE=1`。
你也可以使用 `contrib/fullnode.config.ini` 作为你自己的`config.ini`文件的基础设置。

### 用于资源交换的节点（交易）

使用低内存配置节点。

同时，确保你的 `config.ini` 文件包含如下配置：
```
enable-plugin = account_history
public-api = database_api
track-account-range = ["yourexchangeid", "yourexchangeid"]
```
注意，在不清楚的情况下，请不要随意启用其他APIs或者插件。

Docker下使用如下命令也可以激活类似配置：


```
docker run -d --env TRACK_ACCOUNT="yourexchangeid" \
    --name taiyin \
    --restart unless-stopped \
    zuowangdaox/taiyi
```

### 环境变量配置参数参考

| 环境变量                 | 说明                                                                |
| ------------------------ | ------------------------------------------------------------------- |
| `IS_TESTNET`             | 启用测试网络模式                                                    |
| `USE_WAY_TOO_MUCH_RAM`   | 启用高内存模式，会启动账户历史插件                                  |
| `USE_FULL_WEB_NODE`      | 启用以 fullnode.config.ini 配置文件启动节点                         |
| `IS_BROADCAST_NODE`      | 启用以 以 config-for-broadcaster.ini 配置文件启动节点               |
| `IS_AH_NODE`             | 启用以 config-for-ahnode.ini 配置文件启动节点                       |
| `IS_OPSWHITELIST_NODE`   | 启用以 fullnode.opswhitelist.config.ini 配置文件启动节点            |
| `USE_NGINX_FRONTEND`     | 启用 nginx 前端代理，如果有 Web 端请求节点需求一般需要开启避免 CORS |
| `TAIYI_SEED_NODES`       | 指定种子节点                                                        |
| `TAIYI_SIMING_NAME`      | 指定节点司命账户名称                                                |
| `TAIYI_PRIVATE_KEY`      | 指定节点司命账户私钥                                                |
| `STALE_PRODUCTION`       | 启用区块生成，即使区块链已经过时。产                                |
| `REQUIRED_PARTICIPATION` | 为了生成区块必须司命参与百分比，启动本地测试网时通常为 0            |
| `TRACK_ACCOUNT`          | 指定需要追踪的账号                                                  |


## 启动测试节点示例

### 启动同步当前测试网的全节点（Full Node）

启动一个提供*所有*可查询数据的测试网络节点，同步当前的测试网络。

`TAIYI_SEED_NODES`指向测试网络的一个种子节点，实际中可能会有所不同。

```sh
docker run \
    --env IS_TESTNET=1 --env USE_FULL_WEB_NODE=1 \
    --env REQUIRED_PARTICIPATION=0 --env TAIYI_SEED_NODES="47.109.49.30:2025" \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-full \
    zuowangdaox/taiyi

docker logs -f taiyin-full
```

### 本地启动一个初始化的测试网，全节点（Full Node）

对于一些调试和开发工作，可以在本地启动一个全新的测试网络。

由于是初始节点，因此`STALE_PRODUCTION`设置为true表示要立即出块。

`TAIYI_SIMING_NAME`和`TAIYI_PRIVATE_KEY`分别设置为测试网络的初始司命账号名和初始私钥，目前initminer的账号名和私钥在测试网络上是固定的。

```sh
docker run \
    --env IS_TESTNET=1 --env USE_FULL_WEB_NODE=1 \
    --env STALE_PRODUCTION=1 --env REQUIRED_PARTICIPATION=0 \
    --env TAIYI_SIMING_NAME=initminer \
    --env TAIYI_PRIVATE_KEY=5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n \
    -d -p 2025:2025 -p 8090:8090 --name taiyin-testnet \
    zuowangdaox/taiyi

docker logs -f taiyin-testnet
```

## 升级节点

> [!NOTE]
> 如果不使用 `-v` 参数，则需要自己保证容器内的 `/var/taiyi` 目录的完整性以及在不同容器中的数据同步，否则在升级过程中可能会导致节点数据丢失导致重新同步。

推荐通过[将节点数据挂载到储存系统](./quickstart.md#将节点数据挂载到储存系统)来保证节点数据储存到本机的指定目录来简化保存 `/var/taiyi` 目录；或者通过其他手段将旧版本容器中的 `/var/taiyi` 目录备份出来。

然后升级镜像：

```sh
docker pull zuowangdaox/taiyi:latest
```

然后停止旧容器然后删除旧容器(或者重新启动时提供不同的容器名称)：

```sh
docker stop taiyin
# 可选
docker rm taiyin
```

然后应该直接以旧容器相同的参数启动新容器即可，如果未删除旧容器则需要提供不同容器名称：

```sh
docker run \
    -d -p 2025:2025 -p 8090:8090 --name taiyin \
    -v ~/taiyi-mount:/var/taiyi \
    zuowangdaox/taiyi:latest
```

## 资源使用配置

确保你的机器有足够的运行资源。

区块数据本身占用超过**16GB**的存储空间。

### 全节点
全节点的状态文件使用超过**65GB**的存储空间

### 交易节点
资源交易用的节点的状态文件使用超过**16GB**的存储空间（主要用于追踪单一账号的历史信息）

### 种子节点
种子节点的状态文件使用超过**5.5GB**的存储空间

### 其他情况
不同情况有不同的状态文件占用规模，不同配置占用磁盘规模一般介于“种子节点”和“全节点”之间。

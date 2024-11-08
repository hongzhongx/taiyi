快速启动指南
----------

### 启动太阴（taiyin）
使用docker来启动：
```
docker run \
    -d -p 2001:2001 -p 8090:8090 --name taiyin-default \
    --restart unless-stopped zuowangdao/taiyi
```
#### 低内存节点
上面的指令就启动了一个低内存节点，这种节点适用于:
- 种子节点
- 司命节点（也是出块节点）
- 资源交易节点

对于启动一个完全API的节点，运行：

```
docker run \
    --env USE_WAY_TOO_MUCH_RAM=1 --env USE_FULL_WEB_NODE=1 \
    -d -p 2001:2001 -p 8090:8090 --name taiyin-full \
    --restart unless-stopped \
    zuowangdao/taiyi
```
### 根据需求配置示例

#### 完全API节点
你需要像上述指令一样使用 `USE_WAY_TOO_MUCH_RAM=1` 和 `USE_FULL_WEB_NODE=1`。
你也可以使用 `contrib/fullnode.config.ini` 作为你自己的`config.ini`文件的基础设置。

#### 资源交换（交易）
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
    zuowangdao/taiyi
```

### 资源使用配置

确保你的机器有足够的运行资源。

区块数据本身占用超过**16GB**的存储空间。

#### 全节点
全节点的状态文件使用超过**65GB**的存储空间

#### 交易节点
资源交易用的节点的状态文件使用超过**16GB**的存储空间（主要用于追踪单一账号的历史信息）

#### 种子节点
种子节点的状态文件使用超过**5.5GB**的存储空间

#### 其他情况
不同情况有不同的状态文件占用规模，不同配置占用磁盘规模一般介于“种子节点”和“全节点”之间。

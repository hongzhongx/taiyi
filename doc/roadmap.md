# 路线图/计划

## 第一部分[【起源】](https://github.com/hongzhongx/taiyi/blob/main/doc/yuzhou_explain.md#%E6%BA%90%E9%83%A8%E8%B5%B7%E6%BA%90)

本阶段用于启动最初的因果天道运行，各种最初的法宝开始运转，最初的物质体系形成。这个阶段需要对太乙宇宙[“起源”](https://github.com/hongzhongx/taiyi/blob/main/doc/yuzhou_explain.md#%E6%BA%90%E9%83%A8%E8%B5%B7%E6%BA%90)部分理论有所理解的参与者。

账号在这个阶段可以开始炼制法宝，并通过法宝执行各种操作。

包含如下四个计划：

- [x] [【司命】](https://github.com/users/hongzhongx/projects/1)集结最多二十一个司命启动因果天道网络————太乙网络，建立基础的因果历史信息序列
  - 司命天道共识，建立一息约为3秒钟的因果链式信息结构
  - 实现[白玉京APIs](../libraries/plugins/baiyujing_api/)，统一对天道网络的访问请求
  - 实现可执行程序“[太阴（taiyin）](../programs/taiyin/)”来运行太乙节点
  - 实现可执行程序“[玄牝（xuanpin）](../programs/xuanpin/)”来和白玉京交互

- [x] [【阳寿】](https://github.com/users/hongzhongx/projects/2)同质化游戏资产（FA，一种同质化资产）的原生实现
  - 原生实现阳寿（YANG）和先天一炁（QI），阳寿有时也称阳寿丹，炁同气，有时也叫做真气
  - 账号可以转换“阳寿”和“气”，并用气来拜司命（通过祭祀、祈祷、供养以增加司命的实力——Adore）
  - 对出块司命开始奖励阳寿丹
  - 原生实现游戏基本物质资产：金石（Gold）、食物（Food）、木材（Wood）、织物（Fabric）、药材（Herb）
  - 建立账号各种操作的运行资源消耗记录，对应各种操作都需要消耗账号的真气，消耗的真气以非罡（feigang）形式奖励给创作者或者坐忘道财库
  - 确立《真气守恒定律》

- [x] [【天道】](https://github.com/users/hongzhongx/projects/3)智能游戏脚本（SGS）引擎
  - 集成[Lua虚拟机](../libraries/lua/README.md)
  - 建立天道运转化生机制，天道运转需要消耗真气，天道运转消耗的真气转化为对应的非罡来奖励天道创作者
  - 化生的非罡可以转化为真气奖励给天道的创建和开发者（激励规则，通过核算智能脚本运行时的消耗来激励到创作者，使得通胀后大部分新阳寿以真气形式奖励给内容创作者）
  - 天道（智能脚本）运行对账号真气的消耗
  - 开始在[天道实践](https://github.com/hongzhongx/taiyi-contracts)示例项目中探索各种天道

- [ ] [【安慈】](https://github.com/users/hongzhongx/projects/4)非同质游戏资产（NFA）原生实现
  - NFA的组合包含关系
  - NFA的天道（NFA绑定SGS，**可编程游戏内组件**），NFA心跳
  - 规范化NFA的天道接口，实现可编程游戏内组件之间的可组合性
  - 原生实现游戏物品扩展NFA（Item），创建首批非同质游戏实体（初级物品/道具）的配方和图谱，设立需要的基本材料和相关天道
  - NFA具有真气，原生实现在NFA运转天道时候对其真气的消耗，以及化成非罡奖励到天道创作者
  - NFA的物质性，材质和五行属性，账号开始炼器和炼丹（由基本材料生产物品，并注入天道）
  - 首批自身具有天道的物品NFA开始运转，例如“炼天塔（转换气到基本物质或者从基本物质炼制NFA的法宝）”等等

## 第二部分[【网络】](https://github.com/hongzhongx/taiyi/blob/main/doc/yuzhou_explain.md#%E7%BD%91%E9%83%A8%E7%BD%91%E7%BB%9C)

本阶段将启动一个基于太乙网络的社交系统（名为`坐忘道`），用于天道的普遍开发、应用和讨论。这个阶段重点对应太乙宇宙的[“网络”](https://github.com/hongzhongx/taiyi/blob/main/doc/yuzhou_explain.md#%E7%BD%91%E9%83%A8%E7%BD%91%E7%BB%9C)部分，参与者需要对太乙宇宙网部理论比较了解。

NFA物品道具的设计也在坐忘道进行，并且通过一种`铁匠铺`方式发行配方和图谱（届时将不再支持阶段一的直接创建方式）。而铁匠铺的发行判定，则由所有账号用真气来投票决定，这些操作均在坐忘道平台上完成。

包含如下三个计划：

- [ ] [【牦之门】](https://github.com/users/hongzhongx/projects/5)社交型文本发布系统的原生实现
  - 无准入的文本数据发布、点赞、评论等操作，支撑游戏逻辑和内容的社交化创建
  - 实现SGS的社交化发布机制，应用账号真气限制
  - 实现SGS的社交化应用机制（通过文本社交系统，对物品等游戏实体绑定天道）

- [ ] [【坐忘】](https://github.com/users/hongzhongx/projects/6)实现访问白玉京的一个前端，提供Web界面的文本社区实现
  - 项目名为[坐忘道（ZuowangDAO）](https://)

- [ ] [【舞狮】](https://github.com/users/hongzhongx/projects/7)实现NFA的去中心化`铁匠铺`发行判定，铁匠铺在坐忘道平台上组织
  - 账号对新NFA（物品/角色/区域/道具等等）配方或者图谱的点赞和踩，凭净胜真气来判定

## 第三部分[【星空】](https://github.com/hongzhongx/taiyi/blob/main/doc/yuzhou_explain.md#%E6%98%9F%E9%83%A8%E6%98%9F%E7%A9%BA)

包含如下一个计划：

- [ ] [【袄景】](https://github.com/users/hongzhongx/projects/8)游戏角色NFA（Actor）扩展对象的原生实现
  - 角色内置属性原生实现（天赋、基本属性等）
  - 角色天赋的设计和发布在坐忘道完成（通过坐忘道平台投票）
  - 角色相互关系原生实现（亲友夫妻师徒父兄仇敌等）
  - 角色托管资产，支持FA和NFA
  - 角色对应NFA的原生实现，应用NFA的真气消耗规则
  - 角色内秉天道（主合约脚本）
  - 角色通过消耗FOOD等资源来获得真气
  - 角色天赋事件（合约）
  - 角色互动事件判定的原生或者合约实现（爱慕、交谈、春宵、霸凌、怀孕等）
  - 账号开始创建角色

## 第四部分[【相接】](https://github.com/hongzhongx/taiyi/blob/main/doc/yuzhou_explain.md#%E6%8E%A5%E9%83%A8%E7%9B%B8%E6%8E%A5%E8%BF%9E%E6%8E%A5)

包含如下一个计划：

- [ ] [【监天】](https://github.com/users/hongzhongx/projects/9)游戏区域NFA（Zone）扩展对象的原生实现
  - 区域自然属性（数字地理学）
  - 区域连通性，建立游戏的空间概念（从箱子到房间到城市都是相互连接的区域）
  - 区域内秉天道（主合约脚本）
  - 区域对应NFA的原生实现，应用NFA的真气消耗规则
    <div align='center'><a href='../doc/imgs/gradio_map.jpg'><img src='../doc/imgs/gradio_map.jpg' width=50%></a></div>
    <div align='center'><i>Gradio客户端接入白玉京后，分析出来牛心村和周围地区的连接</i></div>

## 第五部分[【大地】](https://github.com/hongzhongx/taiyi/blob/main/doc/yuzhou_explain.md#%E5%9C%B0%E9%83%A8%E5%9C%B0%E7%90%83%E5%A4%A7%E5%9C%B0)

本阶段通过[坐忘道平台](https://)，由玩家角色开始建设大傩世界

包含如下两个计划：

- [ ] [【修真】](https://github.com/users/hongzhongx/projects/10)角色功法和修炼
  - 角色通过一种“概率性挖矿”形式创造真气（修真-Cultivation），这部分真气来自链上通货膨胀的激励Fund（修真基金）
  - SGS实现的各种功法

- [ ] [【大傩】](https://github.com/users/hongzhongx/projects/11)开始构建最初的大傩世界
  - 实现一个类MUD前端可执行程序，名为“[大傩（danuo）](../programs/danuo/)”
  - 建立起至少一个村落，如牛心村
  - 建立起至少一个城镇，如四齐
  - 建立起至少一个门派，如清风观
  - 普通玩家在不接触白玉京的情况下，开始进入大傩世界
    <div align='center'><a href='../doc/imgs/mud.jpg'><img src='../doc/imgs/mud.jpg' width=50%></a></div>
    <div align='center'><i>普通玩家操作的Mud界面</i></div>

<h1 align='center'>太乙（Taiyi）</h1>

<br>
<div align='center'>
    <a href='https://hub.docker.com/r/hongzhongx/taiyi/tags'><img src='https://img.shields.io/github/v/tag/hongzhongx/taiyi?style=flat-square'></a>
    <a href='https://github.com/hongzhongx/taiyi/blob/main/LICENSE'><img src='https://img.shields.io/github/license/hongzhongx/taiyi?style=flat-square'></a>
    <a href='https://github.com/hongzhongx/taiyi/graphs/contributors'><img src='https://img.shields.io/github/contributors-anon/hongzhongx/taiyi?style=flat-square'></a>
    <a href='https://github.com/hongzhongx/taiyi/commits/main'><img src='https://img.shields.io/github/commit-activity/y/hongzhongx/taiyi?style=flat-square'></a>
</div>
<br>

# 介绍

这是一个在线小游戏，模拟一个虚拟的修仙世界，为乐子人找找乐子。

<br>
<div align='center'><a href='./doc/imgs/map.jpg'><img src='./doc/imgs/map.jpg' width=40%></a></div>
<div align='center'><i>在Mud游戏中，如果你能真正拥有并设计自己的房间，又或者能运营自己的城镇门派，会是一种什么体验？</i></div>


## 模拟游戏世界的基础网络——太乙（Taiyi）

司命们相互连接，逐渐形成一个因果网络，上面运转各种天道规则，在这些规则下，演化众多虚拟的世界。其中之一，就是大傩世界。

无论你是开发者、运营者、玩家还是NPC、真人还是AI Agent，只要你接入太乙网络，你就是在玩这个游戏，任何接入者都有资格平等地建设上面的虚拟世界并且和它交互。不仅如此，各种规则（天道）也随着游戏进行而逐渐创建完善并不断演化，这些规则也可以由任何接入者来开发。

开发和维持这个游戏网络，可以由任何匿名接入者完成，不需要中心化设施。玩家们能在互不了解和信任的情况下，组织游戏设计和开发，并公平地获得奖励。

大傩世界游戏使用了区块链思想和技术，游戏不是建立在抽象的通用区块链上，游戏本身包含了区块链。

## 文档

* 智能游戏脚本（SGS）：[https://github.com/hongzhongx/taiyi-contracts/blob/main/doc/sgs-whitepaper.pdf](https://github.com/hongzhongx/taiyi-contracts/blob/main/doc/sgs-whitepaper.pdf) (*不断更新中*)
* 前期交流社区（Discord）：[https://discord.gg/g4f84UEGCD](https://discord.gg/g4f84UEGCD)。理想情况下，社区将由太乙网络本身承载，详见[坐忘道项目](https://)

## 特色

* 游戏交互无需费用（可恢复的行动力 = 免费模式）
* 快速事件确认（一息=3秒钟）
* 延时安全性（气/QI = 延时转换的阳寿/YANG）
* 分层权限（多级私钥）
* 原生实现的内置游戏资产（通证）分配
* 天道：智能游戏脚本（Smart Game Scripts）
* 持续丰富的游戏内容用户创建（天道的运行，化生阳寿或者气的激励）

## 为什么用Lua作为智能游戏脚本的虚拟机？而不是EVM、WASM、V8或者Docker等等

* 实现的司命网络中使用了区块链技术，并且原生实现了游戏的基本逻辑和资产。记住太乙网络本身属于游戏的一部分，她已经部署了原生的游戏资产，用户不需要再使用智能合约来为游戏资产定义。太乙网络上的智能合约就是智能游戏脚本（SGS），被用于游戏应用本身的逻辑和配置，而非为通用应用开发，这点很像早期的MUD游戏编程语言。
* Lua本身非常轻量级，而且能很好地和C++应用集成。欢迎所有用过Lua开发游戏的程序员们！
* 我们需要的是一种配置型的智能合约，而不是通用智能合约。我们要方便地在这种区块链式的网络上配置大傩世界游戏的资源和逻辑，而不是要搞各种各样的区块链应用。 一句话，太乙网络本身就是一个游戏，只不过使用了加密区块链技术。
* 更多关于太乙Lua虚拟机的信息，请参阅[文档](libraries/lua/README.md)

## 一些指标细节

* 基本通货是阳寿丹，简称阳寿，符号是YANG
* 司命因果网络的共识机制采用了代理权益证明（DPOS）算法，司命的实力需要信众的力量
* 全网阳寿供应持续进行通货膨胀，在现实时间二十年内，通货膨胀率从10%年化率（APR）线性降低到1%年化率（APR）
    * 90%的新增供应用于奖励游戏逻辑开发、游戏内容的生产和运作
    * 10%的新增供应用于奖励维护天道因果网络的司命们（即区块生产节点）
* 大傩世界的一个月，持续时长约为现实时间2小时，即现实一天，大傩一年

# 路线图/阶段计划

## 阶段一

本阶段用于启动最初的因果天道运转，各种最初的法宝开始运转，最初的物质体系形成。账号也可以炼制法宝，并通过法宝执行各种操作。

包含如下四个计划节点：

- [ ] [【司命】](https://github.com/users/hongzhongx/projects/1)集结最多二十一个司命启动因果天道网络————太乙网络，建立基础的因果历史信息序列
    * 司命天道共识，建立一息约为3秒钟的因果链式信息结构   
    * 实现[白玉京APIs](libraries/plugins/baiyujing_api/)，统一对天道网络的访问请求
    * 实现可执行程序“[太阴（taiyin）](programs/taiyin/)”来运行天道节点
    * 实现可执行程序“[玄牝（xuanpin）](programs/xuanpin/)”来和白玉京交互

- [ ] [【阳寿】](https://github.com/users/hongzhongx/projects/2)同质化游戏资产（FA，一种同质化资产）的原生实现
    * 原生实现阳寿（YANG）和先天一炁（QI），阳寿有时也称阳寿丹，炁同气，有时也叫做真气
    * 账号可以转换“阳寿”和“气”，并用气来拜司命（通过祭祀、祈祷、供养以增加司命的实力——Adore）
    * 对出块司命开始奖励阳寿丹
    * 原生实现游戏基本物质资产：金石（Gold）、食物（Food）、木材（Wood）、织物（Fabric）、药材（Herb）
    * 账号行动力（Mana）的原生实现，建立气和行动力上限以及恢复速度的关系
    * 建立账号各种操作的资源消耗记录，对应各种操作都需要消耗的行动力

- [ ] [【天道】](https://github.com/users/hongzhongx/projects/3)智能游戏脚本（SGS）引擎
    * 集成[Lua虚拟机](libraries/lua/README.md)
    * 建立天道运转化生机制，天道运转需要消耗行动力（Mana），天道运转消耗的行动力化生出新的气或者阳寿来奖励
    * 化生产物奖励给天道的创建和开发者（激励规则，通过核算智能脚本运行时的消耗来激励到创作者，使得通胀后大部分新阳寿奖励给内容创作者）
    * 天道（智能脚本）运行对账号行动力的消耗
    * 开始在[天道实践](https://github.com/hongzhongx/taiyi-contracts)示例项目中探索各种天道

- [ ] [【安慈】](https://github.com/users/hongzhongx/projects/4)非同质游戏资产（NFA）原生实现
    * NFA的组合包含关系
    * NFA的天道（NFA绑定SGS，可编程游戏内组件），NFA心跳
    * 规范化NFA的天道接口，实现可编程游戏内组件之间的可组合性
    * 原生实现游戏物品扩展NFA（Item），创建首批非同质游戏实体（初级物品/道具）的配方和图谱，设立需要的基本材料和相关天道
    * NFA具有气，原生实现其行动力（Mana），建立气和行动力上限以及恢复速度的关系
    * 账号开始炼器和炼丹（由基本材料生产物品，并注入天道）
    * 首批自身具有天道的物品NFA开始运转，例如“炼天塔（转换气到基本物质或者从基本物质炼制NFA的法宝）”和“气铃铛（账号将气传递给NFA）”等

## 阶段二

本阶段用于启动一个基于太乙网络的社交系统，用于天道的普遍开发、应用和讨论。

包含如下两个计划节点：

- [ ] 【牦之门】社交型文本发布系统的原生实现
    * 无准入的文本数据发布、点赞、评论等操作，支撑游戏逻辑和内容的社交化创建
    * 实现SGS的社交化发布机制，应用账号行动力限制
    * 实现SGS的社交化应用机制（通过文本社交系统，对物品等游戏实体绑定天道）

- [ ] 【坐忘】实现访问白玉京的一个前端，提供Web界面的文本社区实现
    * 项目名为[坐忘道（ZuowangDAO）](https://)

## 阶段三

本阶段需要原生实现几个重要的可编程游戏实体，为大傩世界的创建和玩家角色的进入做准备。

包含如下两个计划节点：

- [ ] 【袄景】游戏角色NFA（Actor）扩展对象的原生实现
    * 角色内置属性原生实现（天赋、基本属性等）
    * 角色相互关系原生实现（亲友夫妻师徒父兄仇敌等）
    * 角色托管资产，支持FA和NFA
    * 角色行动力（Mana）的原生实现，建立角色拥有的气和行动力上限以及恢复速度的关系
    * 角色合约运行对行动力的消耗
    * 角色内秉天道（主合约脚本）
    * 角色天赋事件（合约）
    * 角色互动事件判定的原生或者合约实现（爱慕、交谈、春宵、霸凌、怀孕等）
    * 账号开始创建角色

- [ ] 【监天】游戏区域NFA（Zone）扩展对象的原生实现
    * 区域自然属性（数字地理学）
    * 区域连通性，建立游戏的空间概念（从箱子到房间到城市都是相互连接的区域）
    * 区域内秉天道（主合约脚本）
    * 区域行动力（Mana）的原生实现，，建立区域拥有的气和行动力上限以及恢复速度的关系
        <div align='center'><a href='./doc/imgs/gradio_map.jpg'><img src='./doc/imgs/gradio_map.jpg' width=50%></a></div>
        <div align='center'><i>Gradio客户端接入白玉京后，分析出来牛心村和周围地区的连接</i></div>

## 阶段四

本阶段通过[坐忘道平台](https://)，由玩家角色开始建设大傩世界

包含如下？个计划节点：

- [ ] 【修真】角色功法和修炼
    * SGS实现的各种功法

- [ ] 【大傩】开始构建最初的大傩世界
    * 实现一个类MUD前端可执行程序，名为“[大傩（danuo）](programs/danuo/)”
    * 建立起至少一个村落，如牛心村
    * 建立起至少一个城镇，如四齐
    * 建立起至少一个门派，如清风观
    * 普通玩家在不接触白玉京的情况下，开始进入大傩世界
        <div align='center'><a href='./doc/imgs/mud.jpg'><img src='./doc/imgs/mud.jpg' width=50%></a></div>
        <div align='center'><i>普通玩家操作的Mud界面</i></div>

# 责任和权益

大傩世界游戏没有项目方，该游戏网络（即太乙网络）由坐忘道🀄️发起，纯粹为了乐子人找乐子，任何爱好者都可以自由参与、分发或者离开，本项目造成一切后果均由坐忘道负责。

本项目的代码全部开源，坐忘道🀄️放弃一切权益，不承担任何责任，不承诺提供任何支持。

<br>
<div align='center'><b><big><i>听到天外的号角，揭开你修仙的帷幕</i></big></b></div>

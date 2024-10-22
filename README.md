<h1 align='center'>太乙（Taiyi）</h1>

<br>
<div align='center'>
    <a href='https://hub.docker.com/r/hongzhongx/taiyi/tags'><img src='https://img.shields.io/github/v/tag/hongzhongx/taiyi?style=flat-square'></a>
    <a href='https://github.com/hongzhongx/taiyi/blob/main/LICENSE.md'><img src='https://img.shields.io/github/license/hongzhongx/taiyi?style=flat-square'></a>
    <a href='https://github.com/hongzhongx/taiyi/graphs/contributors'><img src='https://img.shields.io/github/contributors-anon/hongzhongx/taiyi?style=flat-square'></a>
    <a href='https://github.com/hongzhongx/taiyi/commits/main'><img src='https://img.shields.io/github/commit-activity/y/hongzhongx/taiyi?style=flat-square'></a>
</div>
<br>

# 介绍

准备做一个在线小游戏，模拟虚拟世界，取材于网络小说《道诡异仙》的世界观，为乐子人找找乐子。

## 模拟游戏世界的基础网络——太乙（Taiyi）

司命们相互连接形成的一个因果网络，上面运行各种天道规则，在这些规则下，演化众多虚拟的世界，例如大傩世界。

在这个游戏中，不区分开发者、运营者、玩家还是NPC、真人还是AI Agent，任何接入者都有资格平等地建设虚拟世界并且和它交互。不仅如此，各种规则（天道）也随着游戏进行而逐渐创建完善并不断演化，这些规则也可以由任何接入者来开发。

开发和维持这个游戏网络，可以由任何匿名接入者完成，不需要中心化设施。玩家们能在互不了解和信任的情况下，组织游戏设计和开发，并公平地获得奖励。

大傩世界游戏使用了区块链思想和技术，游戏不是建立在抽象的通用区块链上，游戏本身包含了区块链。

## 文档

* 智能游戏脚本（SGS）：[taiyi.io/SGSWhitepaper.pdf](https://sgs.taiyi.io/sgs-whitepaper.pdf) (*不断更新中*)
* 前期交流社区（Discord）：[坐忘道](https://discord.gg/g4f84UEGCD)。理想情况下，社区将由太乙网络本身承载，详见[坐忘道项目](https://)

## 特色

* 游戏交互无需费用（可恢复的行动力 = 免费模式）
* 快速事件确认（一息=3秒钟）
* 延时安全性（气/QI = 延时转换的阳寿/YANG）
* 分层权限（多级私钥）
* 原生实现的内置通证分配
* 智能游戏脚本（Smart Game Scripts）
* 持续丰富的游戏内容用户创建

## 为什么用Lua作为智能游戏脚本的虚拟机？而不是EVM、WASM、V8或者Docker等等

* 实现的司命网络中使用了区块链技术，并且原生实现了游戏的基本逻辑和资产。记住太乙网络本身属于游戏的一部分，她已经部署了原生的游戏资产，用户不需要再使用智能合约来为游戏资产定义。太乙网络上的智能合约就是智能游戏脚本（SGS），被用于游戏应用本身的逻辑和配置，而非为通用应用开发，这点很像早期的MUD游戏编程语言。
* Lua本身非常轻量级，而且能很好地和C++应用集成。欢迎所有用过Lua开发游戏的程序员们！
* 我们需要的是一种配置型的智能合约，而不是通用智能合约。我们要方便地在这种区块链式的网络上配置大傩世界游戏的资源和逻辑，而不是要搞各种各样的区块链应用。 一句话，太乙网络本身就是一个游戏，只不过使用了加密区块链技术。
* 更多关于太乙Lua虚拟机的信息，请参阅[文档](libraries/lua/README.md)

## 一些指标细节

* 基本通货是阳寿丹，简称阳寿，符号是YANG
* 司命因果网络的共识机制采用了代理权益证明（DPOS）算法，司命的实力需要信众的力量
* 全网阳寿供应持续进行通货膨胀，在现实时间二十年内，通货膨胀率从10%年化率（APR）线性降低到1%年化率（APR）
    * 90%的新增供应用于奖励游戏逻辑开发、游戏内容的生产和运作。
    * 10%的新增供应用于奖励维护天道因果网络的司命们（即区块生产节点）。

# 路线图/阶段计划

- [x] 【司命】集结最多二十一个司命启动因果天道网络————太乙网络，建立基础的因果历史信息序列   
    * 实现[白玉京APIs](libraries/plugins/baiyujing_api/)，统一对天道网络的访问请求
    * 实现可执行程序“[太阴（taiyin）](programs/taiyin/)”来运行节点
    * 实现可执行程序“[玄牝（xuanpin）](programs/xuanpin/)”来和太阴交互

- [ ] 【阳寿】智能游戏通证（SGT，一种同质化通证）的原生实现
    * 对司命开始进行奖励

- [ ] 【秏之门】文本发布机制的原生实现，以支撑游戏逻辑和内容的社交化创建
    * 支持无准入的文本数据发布、点赞、评论等操作

- [ ] 【坐忘】实现访问白玉京的一个前端，提供Web界面的文本社区实现
    * 项目名为[坐忘道（ZuowangDAO）](https://)

- [ ] 【天道】实现智能游戏脚本（SGS）
    * 集成[Lua虚拟机](libraries/lua/README.md)
    * 实现SGS的社交化发布机制
    * 升级内容激励规则，应用智能脚本运行时的消耗到激励，使得通胀后大部分新阳寿奖励给内容创作者

- [ ] 【袄景】游戏角色（Actor）对象的原生实现
    * 角色内置属性原生实现（天赋、基本属性等）
    * 角色托管资产，支持SGT
    * 角色主合约

- [ ] 【监天】游戏区域（Zone）对象的原生实现
    * 区域自然属性（数字地理学）
    * 区域连通性
    * 区域主合约

- [ ] 【安慈】可分割游戏通证（DGT）原生实现
    * 角色托管资产支持DGT
    * 物品/道具对象的原生实现

- [ ] 【大傩】开始构建最初的大傩世界
    * 实现一个类MUD前端可执行程序，名为“[大傩（danuo）](programs/danuo/)”
    * 建立起至少一个村落，如牛心村
    * 建立起至少一个城镇，如四齐
    * 建立起至少一个门派，如清风观

# 责任和权益

大傩世界游戏没有项目方，该游戏网络（即太乙网络）仅由坐忘道🀄️发起，纯粹为了乐子人找乐子，任何爱好者都可以自由参与、分发或者离开，本项目造成一切后果均由坐忘道负责。

本项目的代码全部开源，坐忘道🀄️放弃一切权益，不承担任何责任，不承诺提供任何支持。

<br>
<div align='center'><b><big><i>听到天外的号角，揭开你修仙的帷幕</i></big></b></div>

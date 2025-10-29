# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- NFA的物质性，材质和五行属性。NFA Symbol增加对材质的要求。NFA内禀天道运转时对材质的消耗。
- 合约API增加`is_nfa_action_exist`，用于判断nfa是否具有指定行为。
- 合约API增加获得nfa当前位置的接口：`get_nfa_location`。
- NFA定义增加实例最大数量，创建nfa实例超过最大数量后抛出异常。
- 合约API增加对zone对象进行炼化的方法`refine_zone`。
- 司命出块奖励操作记录。
- 出块司命错过出块超过24小时自动出局。
- 首次申请成为司命需要支付7倍当前司命出块奖励的真气。
- 角色相互关系（亲友夫妻师徒父兄仇敌等）对象以及数据库索引。
- NFA合约API增加角色“交谈”方法，以及对角色之间好感度的影响。
- 增加zone和特定合约的许可数据对象以及数据库索引。
- NFA合约在执行前对所属zone进行许可判定。
- 虚拟时间报时精确到一天的四个时候。

### Changed

- 创世出块司命的账号名称改为“danuo”。
- 各操作消耗真气按“执行”+“状态存储”+“天道合约运行”修正调整。
- 创建角色天赋规则的操作限制为仅限`TAIYI_COMMITTEE_ACCOUNT`账号操作，要求active权限。
- 探索（`exploit_zone`）和开拓（`break_new_zone`）新区域的合约API真气消耗大幅增加。
- 模拟时间调整为真实时间1小时模拟虚拟时间1天。
- 最大修真持续时间调整为真实时间7天。
- 角色心跳基础消耗真气调整（涉及触发天赋和生长事件）。
- 简化出块奖励计算，司命出块获得相同的奖励。
- NFA合约API的`active`执行时，不再影响已经是active状态的nfa。

### Fixed

- nfa心跳逻辑中，在当前主合约无效或者合约没有心跳入口时，自动禁止心跳。
- nfa和账号之间在注入和提取真气时没有更新账号对司命的信仰力（adore）问题。
- 调用nfa合约时候的对caller账号的错误引用。
- 账号提取非罡后没有对adore代理的真气更新的问题。

### Removed

- 创建区域操作去掉fee。
- 合约api去掉直接改变区域类型的方法`change_zone_type`。

## [0.0.0] - 2025-09-22

### Added

- 司命天道共识基础代码，建立一息约为3秒钟的因果链式信息结构。
- 状态数据库。
- 白玉京APIs插件（baiyujing），统一对天道网络的访问请求。
- 可执行程序“太阴（taiyin）”，运行太乙节点。
- 可执行程序“玄牝（xuanpin”，和白玉京交互。
- 原生同质化资产阳寿（YANG）和真气（QI）。
- 账号可以转换“阳寿”和“气”。
- 账号用气来祭拜供养司命（Adore）。
- 对出块司命开始奖励阳寿。
- 游戏基本物质资产：金石（Gold）、食物（Food）、木材（Wood）、织物（Fabric）、药材（Herb）。
- 账号各种操作的运行资源消耗记录，对应各种操作都要消耗账号的真气，消耗的真气以非罡（feigang）形式奖励给创作者和坐忘道财库。
- 确立《真气守恒定律》。
- 集成Lua虚拟机作为天道运行环境，Lua作为天道代码格式（智能游戏脚本）。
- SGS中与天道合约交互的底层APIs。
- 天道运转（Lua执行）需要消耗真气，天道运转消耗的真气转化为对应的非罡来奖励天道创作者。
- 非同质游戏资产（NFA）原生实现。
- NFA内禀天道（NFA绑定SGS，成为**可编程游戏内组件**），NFA心跳。
- NFA内含真气，NFA运转天道时候对其真气的消耗，以及转化成非罡奖励到天道创作者。
- NFA托管的FA资产。
- NFA包含关系。
- SGS中针对NFA的底层APIs。
- 游戏角色（Actor）对象以及数据库索引。
- 角色天赋事件SGS接口。
- 账号创建角色操作。
- 游戏区域（Zone）对象以及数据库索引。
- 区域连通性对象以及数据库索引，建立游戏的空间概念（从箱子到房间到城市都是相互连接的区域）。
- 角色通过一种“概率性挖矿”形式创造真气（修真-Cultivation），这部分真气来自链上通货膨胀的激励Fund（修真基金）。

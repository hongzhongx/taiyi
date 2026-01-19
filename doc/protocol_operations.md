# 协议操作说明文档 (Protocol Operations Documentation)

本文档详细描述了太乙区块链协议中所有的操作（Operation），包括普通操作和虚拟操作。每个操作都给出了参数解释、业务逻辑及注意事项。

普通操作可以被封装进交易（Transaction）中，而虚拟操作则主要用于系统内部的事件通知。

---

## 操作概览 (Index)

| 序号 | 操作名称 | 描述 |
| :--- | :--- | :--- |
| 0 | `account_create_operation` | 创建新账户 |
| 1 | `account_update_operation` | 更新账户属性 |
| 2 | `transfer_operation` | 转账 (任何FT) |
| 3 | `transfer_to_qi_operation` | 将 YANG 转化为真气 (QI) |
| 4 | `withdraw_qi_operation` | 提取真气 (QI) 转化为 YANG |
| 5 | `set_withdraw_qi_route_operation` | 设置真气提取路由 |
| 6 | `delegate_qi_operation` | 代理真气（委派真气） |
| 7 | `siming_update_operation` | 司命（因果见证人）更新/创建 |
| 8 | `siming_set_properties_operation` | 设置司命属性 |
| 9 | `account_siming_adore_operation` | 账户对司命进行投票（信仰/反对） |
| 10 | `account_siming_proxy_operation` | 设置司命投票代理 |
| 11 | `decline_adoring_rights_operation` | 拒绝投票权利 |
| 12 | `custom_operation` | 自定义操作（二进制格式） |
| 13 | `custom_json_operation` | 自定义操作（JSON 格式） |
| 14 | `request_account_recovery_operation` | 请求账户恢复 |
| 15 | `recover_account_operation` | 恢复账户 |
| 16 | `change_recovery_account_operation` | 变更恢复账户 |
| 17 | `claim_reward_balance_operation` | 领取奖励收益 |
| 18 | `create_contract_operation` | 创建合约 |
| 19 | `revise_contract_operation` | 修改合约 (未发布版本) |
| 20 | `release_contract_operation` | 发布合约 |
| 21 | `call_contract_function_operation` | 调用合约函数 |
| 22 | `action_nfa_operation` | 执行 NFA 实体的行为（action） |
| --- | **以下为虚拟操作 (Virtual Operations)** | --- |
| 23 | `hardfork_operation` | 发生了硬分叉操作 |
| 24 | `fill_qi_withdraw_operation` | 完成真气提取 |
| 25 | `return_qi_delegation_operation` | 退回真气代理 |
| 26 | `producer_reward_operation` | 产生出块（司命）奖励 |
| 27 | `nfa_symbol_create_operation` | NFA 符号定义被创建 |
| 28 | `nfa_symbol_authority_change_operation` | NFA 符号对应实体的创建权限变更 |
| 29 | `nfa_create_operation` | NFA 实例被创建 |
| 30 | `nfa_transfer_operation` | NFA 实例所有权转移 |
| 31 | `nfa_active_approve_operation` | NFA 执行权限授权 |
| 32 | `nfa_convert_resources_operation` | NFA 真气与资源转换 |
| 33 | `nfa_asset_transfer_operation` | NFA 之间的资产转移 |
| 34 | `nfa_deposit_withdraw_operation` | 账号对 NFA 资产的充提 |
| 35 | `reward_feigang_operation` | 奖励非罡（用于消耗真气分配） |
| 36 | `reward_cultivation_operation` | 修真的修为奖励 |
| 37 | `tiandao_year_change_operation` | 天道年岁变更 |
| 38 | `tiandao_month_change_operation` | 天道月份变更 |
| 39 | `tiandao_time_change_operation` | 天道时间（节气）变更 |
| 40 | `actor_talent_rule_create_operation` | 角色天赋规则被创建 |
| 41 | `actor_create_operation` | 角色被创建 |
| 42 | `actor_born_operation` | 角色出生（投胎） |
| 43 | `actor_movement_operation` | 角色移动 |
| 44 | `actor_talk_operation` | 角色对话 |
| 45 | `zone_create_operation` | 区域被创建 |
| 46 | `zone_type_change_operation` | 区域类型变更 |
| 47 | `zone_connect_operation` | 两个区域产生连接 |
| 48 | `narrate_log_operation` | 旁白日志记录 |
| 49 | `shutdown_siming_operation` | 司命节点关闭 |
| 50 | `create_proposal_operation` | 坐忘道提案被创建 |
| 51 | `update_proposal_votes_operation` | 坐忘道提案投票更新 |
| 52 | `remove_proposal_operation` | 坐忘道提案被移除 |
| 53 | `proposal_execute_operation` | 坐忘道提案被执行 |

---

## 操作详细说明

### 0. account_create_operation

**描述**: 一个现有账户（creator）创建一个新账户。

**参数解释**:

- `fee`: 创建账户所需的手续费（由 YANG 支付，最终会转化为 QI 注入新账户）。
- `creator`: 创建者的账户名。
- `new_account_name`: 新账户的名称（需符合命名规则）。
- `owner`: 账户所有者权限（最高权限）。
- `active`: 账户活跃权限（日常操作）。
- `posting`: 账户备注权限（极低权限操作）。
- `memo_key`: 用于加密备忘录的公钥。
- `json_metadata`: 账户的扩展元数据信息。

**注意事项**:

- 创建者必须拥有足够的余额支付手续费。
- 新账户名称不能已存在。
- 手续费的最小额度由系统当前动态属性确定。

---

### 1. account_update_operation

**描述**: 更新现有账户的权限或元数据。

**参数解释**:

- `account`: 待更新的账户名。
- `owner`: (可选) 新的所有者权限。
- `active`: (可选) 新的活跃权限。
- `posting`: (可选) 新的备注权限。
- `memo_key`: 新的备忘录公钥。
- `json_metadata`: 新的账户扩展元数据。

**注意事项**:

- 如果修改 `owner` 权限，需要 `owner` 级别的授权。
- 如果不修改 `owner`，则只需要 `active` 权限。

---

### 2. transfer_operation

**描述**: 在两个账户之间转移资产。

**参数解释**:

- `from`: 付款账户名。
- `to`: 收款账户名。
- `amount`: 转移金额（必须是FT资产）。
- `memo`: 转账备注。

**注意事项**:

- `from` 账户必须有足够的可用资产余额。
- 操作需要 `from` 账户的 `active` 权限。

---

### 3. transfer_to_qi_operation

**描述**: 将账户中的流动资产（YANG）转化为真气（QI）。

**参数解释**:

- `from`: 出资账户名。
- `to`: (可选) 目标账户名，即被充能的账户。如果为空，则默认为 `from` 账户。
- `amount`: 要转化的 YANG 金额。

**注意事项**:

- 转化是实时的，按照当前真气价格（真气对阳寿的转化率）进行计算。
- 真气决定了账户在系统中的影响力和执行合约的能力。

---

### 4. withdraw_qi_operation

**描述**: 账户发起将真气（QI）提取并转化回流动资产（YANG）的操作。

**参数解释**:

- `account`: 提取真气的账户名。
- `qi`: 想要提取的真气总量。

**注意事项**:

- 提取不是瞬间完成的。目前太乙系统的规则是分 4 周线性释放，每周提取一次。
- 如果账户正在提取中，发送此操作将更新或取消当前的提取计划（如果金额设为 0）。

---

### 5. set_withdraw_qi_route_operation

**描述**: 设置真气提取后的去向。允许将提取出的 YANG 直接发送给另一个账户，甚至可以选择自动将其再次转化为目标账户的真气。

**参数解释**:

- `from_account`: 提取真气的账户。
- `to_account`: 接收资金的账户。
- `percent`: 转移金额占总提取金额的百分比（1-10000，即 0.01% - 100%）。
- `auto_vest`: 是否自动将转出的 YANG 再次转化为目标账户的真气（QI）。

**注意事项**:

- 一个账户可以设置多个提取路线，只要总百分比不超过 100%。

---

### 6. delegate_qi_operation

**描述**: 将真气（QI）代理（委派）给另一个账户。

**参数解释**:

- `delegator`: 代理出让方。
- `delegatee`: 代理接收方。
- `qi`: 代理的真气数量。

**注意事项**:

- 真气的所有权仍属于 `delegator`，但其产生的投票权和执行权限归 `delegatee` 使用。
- 撤回代理（将 `qi` 设为 0）会有一定的冷却期（通常为一周），在此期间真气处于锁定状态（不能立即再次代理）。
- 不能代理自己通过他人代理获得的真气。

---

### 7. siming_update_operation

**描述**: 注册成为系统司命（见证人）或更新司命参数。

**参数解释**:

- `owner`: 申请成为司命的账户。
- `url`: 司命的个人介绍或网站链接。
- `block_signing_key`: 用于签署区块的公钥。如果设为空（null），则表示退出司命服务。
- `props`: 司命提议的链参数（如：账户创建费、最大区块大小等）。

**注意事项**:

- 成为司命需要支付一定的申请费用。
- 排名前 21 的司命负责生成区块。

---

### 8. siming_set_properties_operation

**描述**: 设置司命的相关属性（更现代的属性设置方式，支持扩展性）。

**参数解释**:

- `owner`: 司命账户。
- `props`: 键值对形式的属性数据。
- `extensions`: 预留的扩展字段。

**注意事项**:

- 主要用于在不分叉的情况下调整共识参数（如：费率平衡等）。

---

### 9. account_siming_adore_operation

**描述**: 账户对特定的司命进行“崇拜”（投票）或表示反对。

**参数解释**:

- `account`: 投票者。
- `siming`: 被投票的司命。
- `approve`: true 表示赞成（崇拜），false 表示反对（不再崇拜）。

**注意事项**:

- 投票权重等同于投票账户当前的真气（QI）总量。
- 更多崇拜意味着该司命有更高概率进入前 21 名并获得生产收益（司命的力量来自信众）。

---

### 10. account_siming_proxy_operation

**描述**: 将原本由自己行使的司命投票权转交给另一个账户（代理人）。

**参数解释**:

- `account`: 投票权利的出让账户。
- `proxy`: 被授权的代理账户。如果设为空，则表示撤销代理。

**注意事项**:

- 代理后，`account` 的真气权重将叠加到 `proxy` 上一并行使投票权。

---

### 11. decline_adoring_rights_operation

**描述**: 账户永久或临时放弃对司命投票（崇拜）的权利。

**参数解释**:

- `account`: 发起操作的账户。
- `decline`: true 表示拒绝权利，false 表示恢复权利。

**注意事项**:

- 主要用于某些特殊用途账户（如合约账户、官方基金等）为示公正而放弃投票。

---

### 12. custom_operation

**描述**: 通用的自定义二进制数据操作，用于在区块链上存储任意信息。

**参数解释**:

- `required_auths`: 执行此操作所需的账户授权。
- `id`: 自定义 ID。
- `data`: 任意二进制数据。

**注意事项**:

- 只要支付了必要的交易手续费（根据数据量决定占用带宽资源），该数据将被记录在链上。

---

### 13. custom_json_operation

**描述**: 与 `custom_operation` 类似，但使用 JSON 格式，更易被人或普通开发者维护。

**参数解释**:

- `required_auths`: 需要的主动活跃权限（Active Authority）。
- `required_posting_auths`: 需要的备注权限（Posting Authority）。
- `id`: 操作的唯一标识符（如插件名）。
- `json`: JSON 格式的数据内容。

**注意事项**:

- 广泛用于基于太乙协议开发的各类去中心化应用（dApp）。

---

### 14. request_account_recovery_operation

**描述**: 当账户被盗（Owner 权限变更）时，由恢复代理发起申请，开始恢复流程。

**参数解释**:

- `recovery_account`: 账户设定的恢复代理人账户。
- `account_to_recover`: 待恢复的账户名。
- `new_owner_authority`: 申请的新所有者权限。

**注意事项**:

- 该操作只能由原本就指定的恢复代理人发起。

---

### 15. recover_account_operation

**描述**: 被盗账户持有者通过提供旧的权限证明，配合 `request_account_recovery_operation` 完成最终的权限重置。

**参数解释**:

- `account_to_recover`: 待恢复的账户。
- `new_owner_authority`: 与恢复请求中一致的新权限。
- `recent_owner_authority`: 过去 30 天内曾使用的旧所有者权限。

**注意事项**:

- 需要同时满足新旧两个权限的签名证明，安全性极高。

---

### 16. change_recovery_account_operation

**描述**: 修改账户指定的恢复代理人。

**参数解释**:

- `account_to_recover`: 想要更改代理的账户。
- `new_recovery_account`: 新的恢复代理账户名。

**注意事项**:

- 此操作生效有 30 天的延迟期，以防攻击者在盗取账号的一瞬间修改掉恢复代理。

---

### 17. claim_reward_balance_operation

**描述**: 将累积在待领取奖励池中的收益结算到账户余额中。

**参数解释**:

- `account`: 结算账户。
- `reward_yang`: 待领取的阳寿（YANG）。
- `reward_qi`: 待领取的真气（QI）。
- `reward_feigang`: 待领取的非罡（FEIGANG）。

**注意事项**:

- 奖励通常来自各种激励机制。领取后，这些真气将可以用于投票权益。

---

### 18. create_contract_operation

**描述**: 账户在链上创建并部署一个新的智能合约（智能游戏脚本 SGS）。

**参数解释**:

- `owner`: 合约的所有者（创建者）。
- `name`: 合约的唯一名称。
- `data`: 合约的源代码内容（Lua 语言）。
- `extensions`: 预留扩展。

**注意事项**:

- 创建者必须是“心素”级别（而且要拥有足够真气）。
- 创建过程会消耗大量的真气（QI），用于支付执行和存储成本。
- 合约名称全局唯一。

---

### 19. revise_contract_operation

**描述**: 修改一个尚未正式发布（未 Release）的合约内容。

**参数解释**:

- `reviser`: 合约修改者。
- `contract_name`: 待修改的合约名称。
- `data`: 新的合约源代码内容。
- `extensions`: 预留扩展。

**注意事项**:

- 只有合约的所有者（Owner）可以修改合约。
- 一旦合约被执行过 `release` 操作，将无法再进行物理修改。

---

### 20. release_contract_operation

**描述**: 正式发布合约。发布后，合约逻辑被视为不可篡改，且无法再被修改。

**参数解释**:

- `owner`: 合约所有者。
- `contract_name`: 合约名称。

**注意事项**:

- 这是一个单向操作，一旦发布则永久锁定。

---

### 21. call_contract_function_operation

**描述**: 调用智能合约中的特定函数。

**参数解释**:

- `caller`: 调用方账户名。
- `contract_name`: 目标合约名。
- `function_name`: 要执行的函数名。
- `value_list`: (可选) 直接以 Lua 类型表示的参数列表。
- `extensions`: (可选) 以 JSON 字符串表示的参数列表（主要用于前端交互）。

**注意事项**:

- 调用合约会根据复杂度和资源占用消耗调用方的真气（QI）。
- 部分真气消耗会作为奖励分配给合约所有者（Owner）和坐忘道国库。

---

### 22. action_nfa_operation

**描述**: 对一个 NFA（非同质化资产实体）执行特定的交互动作，该动作通常会在状态空间产生变化（因果）。

**参数解释**:

- `caller`: 操作发起方。
- `id`: NFA 实体的全局唯一 ID。
- `action`: 要执行的动作名称。在实体合约中，实际动作函数名称通常为“do_动作名”
- `value_list`: 动作函数的参数列表。
- `extensions`: 扩展参数列表。

**注意事项**:

- 发起方必须拥有该 NFA 的所有权（Owner）或活跃操作权（Active Authority）。
- 此操作本质上是调用 NFA 绑定的主合约中的对应动作函数（名为“do_动作名”的函数）。

---

### 23. hardfork_operation (虚拟)

**描述**: 当区块链发生硬分叉升级时，由系统自动产生的通知操作。

**参数解释**:

- `hardfork_id`: 分叉的版本或 ID 号。

---

### 24. fill_qi_withdraw_operation (虚拟)

**描述**: 当真气提取计划（Withdraw Qi）执行时，由系统自动产生的资金结算操作。

**参数解释**:

- `from_account`: 提取方。
- `to_account`: 接收方。
- `withdrawn`: 实际提取出的真气量。
- `deposited`: 最终转换并发放给目标账户的金额。

---

### 25. return_qi_delegation_operation (虚拟)

**描述**: 当真气代理被撤回且冷却期结束后，由系统自动触发的资金归还操作。

**参数解释**:

- `account`: 归还真气的接收账户。
- `qi`: 归还的真气总量。

---

### 26. producer_reward_operation (虚拟)

**描述**: 生产区块后，由系统自动发放给司命（见证人）的奖励操作。

**参数解释**:

- `producer`: 获得奖励的司命。
- `reward`: 奖励的真气（QI）数量。

---

### 27. nfa_symbol_create_operation (虚拟)

**描述**: 在系统底层创建一个新的 NFA 符号定义。

**参数解释**:

- `creator`: 创建账户。
- `symbol`: NFA 的唯一符号名（如“丹药”、“灵剑”）。
- `describe`: 关于此种实体的详细描述。
- `default_contract`: 默认绑定的逻辑合约名。
- `max_count`: 该符号实体的发行上限（0 为不限）。
- `min_equivalent_qi`: 此类实例在运行（执行行为）时其内部材质的最少等价真气量。（如果实例的材质不足或是因为损坏导致材质等价的真气量不足，那么实例将无法执行行为，直到修复）
- `is_sbt`: 是否为灵魂绑定（不可转让）。

---

### 28. nfa_symbol_authority_change_operation (虚拟)

**描述**: 变更 NFA 符号对应实体的创建权限。

**参数解释**:

- `creator`: 符号原始创建者。
- `symbol`: NFA 符号名。
- `authority_account`: 新的权限账户。
- `authority_nfa_symbol`: 持有 NFA 符号实体的账号具有权限（如果存在）。

---

### 29. nfa_create_operation (虚拟)

**描述**: 当一个新的 NFA 实例被铸造（Mint）出来时产生的记录操作。

**参数解释**:

- `creator`: 铸造者。
- `symbol`: 所属的 NFA 符号。

---

### 30. nfa_transfer_operation (虚拟)

**描述**: NFA 实例在账户间转移的记录。

**参数解释**:

- `from`: 原主。
- `to`: 新主。
- `id`: NFA 实例 ID。

---

### 31. nfa_active_approve_operation (虚拟)

**描述**: NFA 实例活跃操作权的授权记录。

**参数解释**:

- `owner`: 所有者。
- `active_account`: 被授权的活跃操作账户。
- `id`: NFA 实例 ID。

---

### 32. nfa_convert_resources_operation (虚拟)

**描述**: NFA 内部真气与具体资源（金食木织药）之间的相互转换。

**参数解释**:

- `nfa`: NFA 实例 ID。
- `owner`: 所有人。
- `qi`: 涉及的真气量。
- `resource`: 涉及的资源量。
- `is_qi_to_resource`: 转换方向（true 为真气转资源）。

---

### 33. nfa_asset_transfer_operation (虚拟)

**描述**: NFA 实例之间（而非账户之间）转移内部资产的记录。

**参数解释**:

- `from`: 出让 NFA ID。
- `from_owner`: 出让方主人。
- `to`: 接收 NFA ID。
- `to_owner`: 接收方主人。
- `amount`: 转移的资产。

---

### 34. nfa_deposit_withdraw_operation (虚拟)

**描述**: 账户向 NFA 充值或从 NFA 提现资产的记录。

**参数解释**:

- `nfa`: 目标 NFA ID。
- `account`: 操作账户。
- `deposited`: 充值额度。
- `withdrawn`: 提现额度。

---

### 35. reward_feigang_operation (虚拟)

**描述**: 系统发放非刚奖励的逻辑操作。真气相关的消耗通常通过此机制回馈给生态参与者。

**参数解释**:

- `from`: 支付方。
- `from_nfa`: 涉及的 NFA（如果有）。
- `to`: 奖励接收方。
- `qi`: 奖励的真气等值非罡。

---

### 36. reward_cultivation_operation (虚拟)

**描述**: 修真活动中，针对角色的修为提升而产生的奖励记录。

**参数解释**:

- `account`: 角色拥有者。
- `nfa`: 角色对应的 NFA 实例。
- `qi`: 修为奖励增加的真气值。

---

### 37. tiandao_year_change_operation (虚拟)

**描述**: 系统级的天道年度变更。记录这一年中的生死人数等重要宏观数据。

**参数解释**:

- `messager`: 广播此变更的系统账户。
- `years`: 当前天道年数。
- `months`: 当前月份。
- `times`: 当前节气。
- `live_num`: 当前世间存活的人口（角色）总数。
- `dead_num`: 历史累计死亡总人数。
- `born_this_year`: 本年出生人数。
- `dead_this_year`: 本年死亡人数。

---

### 38. tiandao_month_change_operation (虚拟)

**描述**: 系统级的天道月份变更。

**参数解释**:

- `messager`: 广播者。
- `years/months/times`: 对应的天道时间戳。

---

### 39. tiandao_time_change_operation (虚拟)

**描述**: 系统级的天道节气变更。

**参数解释**:

- `messager`: 广播者。
- `years/months/times`: 节气时间戳。

---

### 40. actor_talent_rule_create_operation (虚拟)

**描述**: 系统注册了一个新的角色天赋生成规则（底层由 Lua 合约逻辑定义）。

**参数解释**:

- `creator`: 规则创建者。
- `contract`: 规则对应的合约名称。

---

### 41. actor_create_operation (虚拟)

**描述**: 一个新的角色（Actor）底层配置被初始化，同时角色的NFA实体被铸造。

**参数解释**:

- `creator`: 创建该角色的账户。
- `family_name`: 角色的姓。
- `last_name`: 角色的名。
- `nfa`: 该角色绑定的 NFA 实例 ID。

---

### 42. actor_born_operation (虚拟)

**描述**: 角色在游戏中正式“出生”或“降生”到某个特定区域。

**参数解释**:

- `owner`: 角色拥有账户名。
- `name`: 角色全名。
- `zone`: 出生的区域名称。
- `nfa`: 关联的 NFA ID。

---

### 43. actor_movement_operation (虚拟)

**描述**: 角色在区域（Zone）之间的移动记录。

**参数解释**:

- `owner`: 所有人。
- `name`: 角色名。
- `from_zone`: 出发区域。
- `to_zone`: 抵达区域。
- `nfa`: 关联的 NFA ID。

---

### 44. actor_talk_operation (虚拟)

**描述**: 角色之间的对话记录或角色对环境的发言。

**参数解释**:

- `actor_owner/actor_name`: 发言者的账户和角色名。
- `target_owner/target_name`: (可选) 听话者的账户和角色名。
- `content`: 对话内容。
- `v_years/v_months/v_days/v_tod/v_times`: 对话发生的精确天道时间。

---

### 45. zone_create_operation (虚拟)

**描述**: 游戏世界中一个新区域（Zone）被创建。

**参数解释**:

- `creator`: 区域创建者。
- `name`: 区域名称。
- `nfa`: 该区域实体对应的 NFA ID。

---

### 46. zone_type_change_operation (虚拟)

**描述**: 区域的类型发生变更时的记录。

**参数解释**:

- `creator`: 发起变更的账户。
- `name`: 区域名。
- `type`: 新的区域类型（如将“原野”变为“农田”）。
- `nfa`: 区域对应的 NFA ID。

---

### 47. zone_connect_operation (虚拟)

**描述**: 两个相邻区域建立了连接通路，允许角色往来。

**参数解释**:

- `account`: 连接操作者。
- `zone1/zone1_nfa`: 起始区域信息。
- `zone2/zone2_nfa`: 目标区域信息。

---

### 48. narrate_log_operation (虚拟)

**描述**: 太乙世界的“旁白”（Narrator）记录的系统事件日志。

**参数解释**:

- `narrator`: 记录者。
- `nfa`: 事件涉及的主体实体。
- `log`: 日志文本内容。

---

### 49. shutdown_siming_operation (虚拟)

**描述**: 司命（见证人）节点的计划性关闭记录。

**参数解释**:

- `owner`: 司命账户名。

---

### 50. create_proposal_operation (虚拟)

**描述**: 创建一项新的系统级治理提案。

**参数解释**:

- `creator`: 提案发起人。
- `contract_name`: 提案拟执行的合约名。
- `function_name`: 提案拟执行的函数名。
- `value_list`: 提案执行参数。
- `end_date`: 投票截止日期。
- `subject`: 提案主题/描述。

---

### 51. update_proposal_votes_operation (虚拟)

**描述**: 账户对提案进行投票或更新已有的投票。

**参数解释**:

- `voter`: 投票者账户。
- `proposal_ids`: 要投票的提案 ID 集合。
- `approve`: true 为赞成，false 为反对。

---

### 52. remove_proposal_operation (虚拟)

**描述**: 撤回或移除已经过时/取消的提案。

**参数解释**:

- `proposal_owner`: 提案所有人。
- `proposal_ids`: 拟移除的提案 ID 列表。

---

### 53. proposal_execute_operation (虚拟)

**描述**: 当前提案通过并由系统自动执行时产生的记录。

**参数解释**:

- `contract_name/function_name/value_list`: 执行时的具体业务信息。
- `subject`: 提案主题。

---

# 白玉京接口 (Baiyujing API) 说明文档

“白玉京”寓意为“能看到天道和司命的地方”。在太乙协议中，`baiyujing_api` 插件提供了一系列用于查询当前区块链和世界状态、执行只读合约逻辑以及与网络交互的接口。

## 核心概念与注意事项

1. **只读型合约调用 (eval*)**:
    * 以 `eval_nfa_action` 和 `eval_nfa_action_with_string_args` 为代表的接口，用于执行 NFA 的读取行为逻辑（如“读书”、“查看”等）。
    * **重要提示**: 这些 API 不产生上链的状态变化，因此可以在非出块节点（只读节点）上运行，响应速度快。
2. **状态查询**: 接口大多用于对当前整个状态（天道、司命、账户、NFA、角色、区域等）的各种查询。
3. **参数说明**: 大部分参数通过 JSON-RPC 传递，通常作为一个数组传入。

---

## 目录

1. [全局与链属性 API](#1-全局与链属性-api)
2. [账户管理 API](#2-账户管理-api)
3. [司命 (Siming) 管理 API](#3-司命-siming-管理-api)
4. [区块与交易管理 API](#4-区块与交易管理-api)
5. [NFA (非同质资产) 管理 API](#5-nfa-非同质资产-管理-api)
6. [角色 (Actor) 管理 API](#6-角色-actor-管理-api)
7. [区域 (Zone) 管理 API](#7-区域-zone-管理-api)
8. [角色关系 (Relation) 管理 API](#8-角色-关系-relation-管理-api)
9. [合约管理 API](#9-合约管理-api)

---

## 1. 全局与链属性 API

### `get_version`

获取当前因果区块链的版本信息。

* **参数**: 无
* **返回**: 包含 `blockchain_version`, `taiyi_revision`, `fc_revision` 的结构体。

### `get_config`

获取因果区块链的配置信息。

* **参数**: 无
* **返回**: 包含链配置常量（如 `TAIYI_BLOCKCHAIN_VERSION`, `TAIYI_CHAIN_ID` 等）的 JSON 对象。

### `get_dynamic_global_properties`

获取当前动态全局属性。

* **参数**: 无
* **返回**: 包含区块链头块号、当前司命、总真气、资源总量等实时数据的结构体。

### `get_chain_properties`

获取因果区块链的共识属性。

* **参数**: 无
* **返回**: 司命投票产生的中位数链参数（如最大块大小等）。

### `get_siming_schedule`

获取当前的司命调度表。

* **参数**: 无
* **返回**: 包括当前活跃司命列表和混淆后的司命顺序。

### `get_hardfork_version`

获取当前生效的硬分叉版本。

* **参数**: 无
* **返回**: 版本号。

### `get_next_scheduled_hardfork`

获取下一次计划的硬分叉信息。

* **参数**: 无
* **返回**: 包含 `hf_version` 和 `live_time` 的结构体。

### `get_reward_fund`

获取指定的奖励基金信息。

* **参数**: `[fund_name]` (string) 基金池名称
* **返回**: 包含余额、上次更新时间等信息的 `api_reward_fund_object`。

### `get_tiandao_properties`

获取“天道”属性，即模拟世界的运行参数。

* **参数**: 无
* **返回**: `api_tiandao_property_object`。

---

## 2. 账户管理 API

### `get_accounts`

批量获取账户信息。

* **参数**: `[[account_name1, account_name2, ...]]`
* **返回**: `extended_account` 数组，包含余额、真气、资源、历史概览等。

### `lookup_account_names`

根据名称查询账户基本信息。

* **参数**: `[[account_name1, account_name2, ...]]`
* **返回**: 账户对象数组，不存在则为 null。

### `lookup_accounts`

模糊匹配查询账户名称。

* **参数**: `[lower_bound_name, limit]`
* **返回**: 账户名列表。

### `get_account_count`

获取账户总数。

* **参数**: 无
* **返回**: 账户数量。

### `get_owner_history`

获取账户 Owner 权限修改历史。

* **参数**: `[account_name]`
* **返回**: 修改记录数组。

### `get_recovery_request`

获取账户恢复请求。

* **参数**: `[account_name]`
* **返回**: 恢复请求对象（如果存在）。

### `get_withdraw_routes`

获取真气提取路由。

* **参数**: `[account, type]` (type: `incoming`, `outgoing`, `all`)
* **返回**: 路由信息数组。

### `get_qi_delegations`

获取账户发出的真气委托。

* **参数**: `[account, from_account, limit]`
* **返回**: 委托对象数组。

### `get_expiring_qi_delegations`

获取正在过期的真气委托。

* **参数**: `[account, from_time, limit]`
* **返回**: 过期委托数组。

### `get_key_references`

根据公钥查找引用的账户。

* **参数**: `[[public_key1, ...]]`
* **返回**: 账户名数组。

### `get_account_resources`

获取账户拥有的资源资产。

* **参数**: `[[account_name1, ...]]`
* **返回**: 资源资产结构体数组（金、食、木、织、药）。

### `verify_authority`

校验交易权限是否满足。

* **参数**: `[signed_transaction]`
* **返回**: bool。

### `verify_account_authority`

校验账户是否拥有指定的公钥权限。

* **参数**: `[account_name, [public_key1, ...]]`
* **返回**: bool。

### `get_account_history`

分页获取账户操作历史。

* **参数**: `[account, from_seq, limit]`
* **返回**: 历史记录映射。

---

## 3. 司命 (Siming) 管理 API

### `get_active_simings`

获取当前正在参与出块的司命列表。

* **参数**: 无
* **返回**: 账户名数组。

### `get_simings`

批量通过 ID 获取司命信息。

* **参数**: `[[siming_id1, ...]]`
* **返回**: 司命对象数组。

### `get_siming_by_account`

通过账户名获取司命信息。

* **参数**: `[account_name]`
* **返回**: 司命对象。

### `get_simings_by_adore`

根据“崇拜”票数排序获取司命列表。

* **参数**: `[start_account, limit]`
* **返回**: 司命对象数组。

### `lookup_siming_accounts`

模糊匹配司命账户名。

* **参数**: `[lower_bound_name, limit]`
* **返回**: 账户名数组。

### `get_siming_count`

获取司命总数。

* **参数**: 无
* **返回**: 数量。

---

## 4. 区块与交易管理 API

### `get_block_header`

获取区块头信息。

* **参数**: `[block_num]`
* **返回**: 区块头结构体。

### `get_block`

获取完整区块。

* **参数**: `[block_num]`
* **返回**: `legacy_signed_block`。

### `get_ops_in_block`

获取区块内包含的所有操作（可选是否包含虚拟操作）。

* **参数**: `[block_num, only_virtual]`
* **返回**: 操作对象数组。

### `get_transaction_hex`

序列化交易为十六进制字符串。

* **参数**: `[signed_transaction]`
* **返回**: hex string。

### `get_transaction`

获取交易详情。

* **参数**: `[trx_id]`
* **返回**: 交易对象。

### `get_transaction_results`

获取交易执行产生的操作结果（Operation Results）。

* **参数**: `[trx_id]`
* **返回**: 结果数组。

### `get_required_signatures`

计算签署交易所必需的公钥。

* **参数**: `[transaction, available_keys]`
* **返回**: 必需公钥集。

### `get_potential_signatures`

获取可能签署交易的所有公钥。

* **参数**: `[transaction]`
* **返回**: 可能的公钥集。

### `broadcast_transaction`

异步广播交易。

* **参数**: `[signed_transaction]`
* **返回**: 无。

### `broadcast_transaction_synchronous`

同步广播交易，并等待其被包含进区块。

* **参数**: `[signed_transaction]`
* **返回**: 包含区块号和交易号的结果对象。

### `broadcast_block`

广播区块。

* **参数**: `[signed_block]`
* **返回**: 无。

---

## 5. NFA (非同质资产) 管理 API

### `find_nfa_symbol`

根据符号查找 NFA 符号对象。

* **参数**: `[symbol]`
* **返回**: `api_nfa_symbol_object`。

### `find_nfa_symbol_by_contract`

根据主合约查找 NFA 符号对象。

* **参数**: `[contract_name]`
* **返回**: `api_nfa_symbol_object`。

### `find_nfa`

根据 ID 查找 NFA 实例。

* **参数**: `[nfa_id]`
* **返回**: `api_nfa_object`。

### `find_nfas`

批量查找 NFA 实例。

* **参数**: `[[nfa_id1, nfa_id2, ...]]`
* **返回**: `api_nfa_object` 数组。

### `list_nfas`

列出属于某个账户的 NFA（按 Owner 排序）。

* **参数**: `[owner_account, limit]`
* **返回**: `api_nfa_object` 数组。

### `get_nfa_history`

获取 NFA 的操作历史。

* **参数**: `[nfa_id, from_seq, limit]`
* **返回**: 历史记录映射。

### `get_nfa_action_info`

查询 NFA 合约动作的元信息。

* **参数**: `[nfa_id, action_name]`
* **返回**: 包含动作是否存在及是否具有“后果”（改变状态）的结构。

### `eval_nfa_action`

**[只读]** 执行 NFA 合约动作并返回结果，不产生链上变化。

* **参数**: `[nfa_id, action_name, [arg1, arg2, ...]]`
* **返回**: `api_eval_action_return`（包含执行结果、叙事日志和错误信息）。

### `eval_nfa_action_with_string_args`

**[只读]** 执行 NFA 合约动作（参数以字符串形式传递）。

* **参数**: `[nfa_id, action_name, "[arg1, arg2, ...]"]`
* **返回**: 同上。

---

## 6. 角色 (Actor) 管理 API

### `find_actor`

根据名称寻找角色。

* **参数**: `[actor_name]`
* **返回**: `api_actor_object`。

### `find_actors`

批量寻找角色。

* **参数**: `[[actor_id1, ...]]`
* **返回**: 角色对象数组。

### `list_actors`

根据所有者列出角色。

* **参数**: `[owner_account, limit]`
* **返回**: 角色对象数组。

### `get_actor_history`

获取角色的操作历史（实际上关联至所属 NFA 历史）。

* **参数**: `[actor_name, from_seq, limit]`
* **返回**: 历史记录映射。

### `list_actors_below_health`

列出健康值低于指定水平的角色（用于寻找体弱或死亡角色）。

* **参数**: `[health_threshold, limit]`
* **返回**: 角色对象数组。

### `find_actor_talent_rules`

通过 ID 获取角色天赋规则。

* **参数**: `[[rule_id1, ...]]`
* **返回**: 天赋规则数组。

### `list_actors_on_zone`

列出位于指定区域的所有角色。

* **参数**: `[zone_id, limit]`
* **返回**: 角色对象数组。

### `get_actor_connections`

获取角色之间的社会关系映射（如血亲、配偶、师徒）。

* **参数**: `[actor_name, relation_type]`
* **返回**: `get_actor_connections_return`。

### `list_actor_groups`

获取角色加入的小组或宗门信息。

* **参数**: `[leader_account, limit]`
* **返回**: 列表信息。

### `find_actor_group`

寻找特定的小组信息。

* **参数**: `{ "leader": "...", "group_name": "..." }`
* **返回**: 小组详细信息。

### `list_actor_friends`

获取角色社交列表（友好度达到一定阈值的角色）。

* **参数**: `[actor_name, standpoint_threshold, only_live]`
* **返回**: `api_actor_friend_data` 数组。

### `get_actor_needs`

获取角色的生理或心理需求（如“春宵”、“欺辱”等目标）。

* **参数**: `[actor_name]`
* **返回**: `api_actor_needs_data`。

### `list_actor_mating_targets_by_zone`

在指定区域内搜寻适合“双修”或“春宵”的目标。

* **参数**: `[actor_name, zone_name]`
* **返回**: 简单角色信息数组。

### `stat_people_by_zone`

统计指定区域内的人物数量（生/死）。

* **参数**: `[zone_name]`
* **返回**: `api_people_stat_data`。

### `stat_people_by_base`

统计出身于指定区域的人物数量。

* **参数**: `[zone_name]`
* **返回**: `api_people_stat_data`。

---

## 7. 区域 (Zone) 管理 API

### `find_zones`

批量查找区域实例。

* **参数**: `[[zone_id1, ...]]`
* **返回**: 区域对象数组。

### `find_zones_by_name`

根据名称批量查找区域。

* **参数**: `[[zone_name1, ...]]`
* **返回**: 区域对象数组。

### `list_zones`

根据所有者列出区域。

* **参数**: `[owner_account, limit]`
* **返回**: 区域对象数组。

### `list_zones_by_type`

根据区域类型列出。

* **参数**: `[type_id, limit]`
* **返回**: 区域对象数组。

### `list_to_zones_by_from`

列出从指定区域可到达的下游区域。

* **参数**: `[from_zone_id, limit]`
* **返回**: 区域对象数组。

### `list_from_zones_by_to`

列出可到达指定区域的上游区域。

* **参数**: `[to_zone_id, limit]`
* **返回**: 区域对象数组。

### `find_way_to_zone`

寻找从一个区域到另一个区域的路径。

* **参数**: `[from_zone_name, to_zone_name]`
* **返回**: 路径详情。

### `list_zones_by_prohibited_contract`

列出禁止某个合约执行的区域。

* **参数**: `[contract_name, limit]`
* **返回**: 区域对象数组。

### `list_contracts_prohibited_by_zone`

列出指定区域内禁止的所有合约。

* **参数**: `[zone_name]`
* **返回**: 合约名称列表。

---

## 8. 角色-关系 (Relation) 管理 API

### `list_relations_from_actor`

列出角色主动对其他角色的关系。

* **参数**: `[actor_name]`
* **返回**: `api_actor_relation_data` 数组。

### `list_relations_to_actor`

列出其他角色对该角色的关系。

* **参数**: `[actor_name]`
* **返回**: `api_actor_relation_data` 数组。

### `get_relation_from_to_actor`

查询两个固定角色之间的关系数据。

* **参数**: `[actor_name1, actor_name2]`
* **返回**: `api_actor_relation_data`。

---

## 9. 合约管理 API

### `get_contract_source_code`

获取指定合约的源代码（需节点开启非低内存模式）。

* **参数**: `[contract_name]`
* **返回**: string (源代码)。

---

## 通用接口

### `get_state`

综合查询接口，根据路径返回状态树（用于前端快速构建页面数据）。

* **参数**: `[path]`
* **返回**: `state` 结构，包含全局属性、指定路径下的账户或司命信息等。
* **注意事项**: 广泛用于太乙的前端展示逻辑。

---
*更多详细信息请参考源码：`libraries/plugins/baiyujing_api/baiyujing_api.hpp`*

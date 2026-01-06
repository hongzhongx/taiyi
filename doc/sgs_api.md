# 智能游戏脚本（合约）API 说明文档

本文档详细列举并描述了 C++ 层暴露给 Lua 合约的 API，包括类型定义、全局方法、合约处理器 (`contract_helper`) 以及 NFA 处理器 (`nfa_helper`)。

---

## 1. 全局方法

### `import_contract(contract_name)`

* **功能**: 在当前合约中导入另一个已发布的合约。
* **参数**:
  * `contract_name` (string): 要导入的合约名称。
* **返回值**: 返回该合约的结构（table）。
* **注意事项**:
  * 导入的合约会在当前合约环境下创建并缓存其运行空间。
  * 不能导入当前正在运行的同名合约。

### `format_vector_with_table(table)`

* **功能**: 将 Lua table 格式化为 JSON 字符串。
* **参数**:
  * `table` (table): 需要格式化的 table。
* **返回值**: 格式化后的 JSON string。

---

## 2. 基础数据类型 (Exposed Types)

### `contract_base_info` (合约基础信息)

* `owner` (string): 合约拥有者账号名。
* `name` (string): 合约名称。
* `caller` (string): 当前调用者账号名。
* `creation_date` (string): 合约创建日期。
* `invoker_contract_name` (string): 调用者所属合约名称（如果是跨合约调用）。

### `contract_asset_resources` (资源/材料信息)

* `gold` (int): 金石。
* `food` (int): 食物。
* `wood` (int): 木材。
* `fabric` (int): 织物。
* `herb` (int): 药材。

### `contract_tiandao_property` (天道时间属性)

* `v_years` (uint): 虚拟年。
* `v_months` (uint): 虚拟月。
* `v_days` (uint): 虚拟日（月内第几日）。
* `v_timeonday` (uint): 当天时刻。
* `v_times` (uint): 节气编号。
* `v_1day_blocks` (uint): 虚拟一日包含的区块数。

### `contract_nfa_base_info` (NFA 基础信息)

* `id` (int): NFA 唯一 ID。
* `symbol` (string): NFA 符号。
* `owner_account` (string): 当前拥有者账号。
* `creator_account` (string): 创建者账号。
* `active_account` (string): 当前操作者账号（有操作权限）。
* `qi` (int): 真气（能量值）。
* `parent` (int): 父 NFA 的 ID。
* `five_phase` (int): 五行属性。
* `main_contract` (string): 主合约名称。
* `data` (table): 关联的合约私有数据。

### `contract_zone_base_info` (区域/空间信息)

* `nfa_id` (int): 区域对应的 NFA ID。
* `name` (string): 区域名称。
* `type` (string): 区域类型名称。
* `type_id` (int) : 区域类型 ID。
* `ref_prohibited_contract_zone` (string): 合约许可引用的区域名称。

### `contract_actor_base_info` (角色基础信息)

* `nfa_id` (int): 角色对应的 NFA ID。
* `name` (string): 角色完整名称。
* `age` (uint): 年龄。
* `health` (int): 当前健康度。
* `health_max` (int): 最大健康度。
* `born` (bool): 是否已出生。
* `born_vyears` / `born_vmonths` / `born_vdays` / `born_vtod` / `born_vtimes`: 出生虚拟时间点。
* `five_phase` (int): 五行。
* `gender` (int): 性别。
* `standpoint` (int): 立场数值。
* `standpoint_type` (int): 立场数值对应的类型。
* `location` (string): 当前所在区域名称。
* `base` (string): 出生地/从属地/归属地名称。

### `contract_actor_core_attributes` (角色核心属性)

详细列举角色的各项核心属性，每项属性包含当前值和最大值（`_max`）：

* `strength` (int16): **臂力/力量**。影响物理攻击、负重等。
* `physique` (int16): **体质**。影响生命值上限、抗性、体力等。
* `agility` (int16): **敏捷/身法**。影响闪避、速度、远程攻击精度等。
* `vitality` (int16): **根骨**。影响修炼速度、灵力承载能力、内功防御等。
* `comprehension` (int16): **悟性**。影响领悟新技能的速度、功法升级难度等。
* `willpower` (int16): **定力/意志**。影响精神防御、抵抗debuff能力等。
* `charm` (int16): **魅力**。影响社交互动中的好感度获取、交易价格等。
* `mood` (int16): **心情**。影响角色行动效率、突破成功率等。

### `contract_actor_talent_rule_info` (角色天赋规则信息)

* `id` (int64): 天赋 ID。
* `main_contract` (string): 该天赋关联的主合约。
* `title` (string): 天赋标题。
* `description` (string): 天赋描述。
* `init_attribute_amount_modifier` (int): 初始属性总点数修正。

### `contract_actor_relation_info` (角色关系信息)

* `actor_name` (string): 角色名。
* `target_actor_name` (string): 目标角色名。
* `favor` (int32): 好感度数值。
* `favor_level` (int): 好感等级。

---

## 3. 合约处理器 API (`contract_helper`)

### 基础功能

* **`log(message)`**: 打印到节点日志的信息。
  * `message` (string): 打印到节点日志的信息字符串。
* **`narrate(message, time_prefix)`**: 输出叙事文本（产生叙事事件）。
  * `message` (string): 输出的叙事文本字符串。
  * `time_prefix` (bool): 是否添加当前虚拟时间作为前缀。
* **`time()`**: 获取当前头区块的时间戳（uint32）。
* **`block()`**: 获取当前头区块号。
* **`random()`**: 获取基于当前区块上下文生成的随机数。
* **`is_owner()`**: 判断当前调用者是否为合约拥有者。
* **`zuowangdao_account_name()`**: 获取坐忘道账号名称。
* **`get_nfa_caller()`**: 如果是由 NFA 发起的合约调用，返回该 NFA 的 ID。

### 数值范围与哈希

* **`number_max()` / `number_min()`**: 获取 Lua 数值的最大/最小值。
* **`integer_max()` / `integer_min()`**: 获取 64 位整数的最大/最小值。
* **`hash256(source)` / `hash512(source)`**: 获取指定字符串的哈希值。
  * `source` (string): 需要进行哈希计算的源字符串。
* **`generate_hash(offset)`**: 生成基于当前区块上下文的哈希值。
  * `offset` (uint32): 计算哈希时的偏移量。

### 合约数据操作

* **`read_contract_data(read_list)`**: 读取合约数据。
  * `read_list` (lua_map/table): 需要读取的数据路径列表。
* **`write_contract_data(data, write_list)`**: 写入合约数据。
  * `data` (lua_map/table): 要写入的数据。
  * `write_list` (lua_map/table): 指定写入路径的映射。
* **`get_contract_data(contract_name, read_list)`**: 获取其他合约公开出的数据。
  * `contract_name` (string): 目标合约名。
  * `read_list` (table): 读取路径列表。
* **`read_account_contract_data(read_list)`**: 读取当前调用者在合约下的私有数据。
  * 参数同上，仅针对当前调用者在该合约下的私有空间。
* **`write_account_contract_data(data, write_list)`**: 写入当前调用者在合约下的私有数据。
  * 参数同上。
* **`get_account_contract_data(account_name, contract_name, read_list)`**: 从指定账号在指定合约的空间读取数据。
  * `account_name` (string): 账号名。
  * `contract_name` (string): 目标合约名。
  * `read_list` (table): 读取路径列表。
* **`get_account_balance(account_name, symbol_name)`**: 获取指定账号的资产余额。
  * `account_name` (string): 账号名。
  * `symbol_name` (string): 资产符号（如 "YANG", "GOLD" 等）。
* **`invoke_contract_function(contract_name, function_name, value_list_json)`**: 调用合约中的方法。
  * `contract_name` (string): 目标合约名。
  * `function_name` (string): 方法名。
  * `value_list_json` (string): JSON 格式的参数列表。
* **`get_contract_source_code(contract_name)`**: 获取合约源代码。
  * `contract_name` (string): 目标合约名。

### 资产转移

* **`transfer_from_owner(to, amount, symbol, enable_logger)`**: 从合约拥有者转移资产。
  * `to` (string): 接收账号名。
  * `amount` (double): 金额。
  * `symbol` (string): 资产符号。
  * `enable_logger` (bool, 可选): 是否启用日志，默认为 false。
* **`transfer_from_caller(to, amount, symbol, enable_logger)`**: 从当前调用者转移资产。
  * 参数同上。
* **`transfer_nfa_from_owner(to, nfa_id, enable_logger)`**: 从合约拥有者转移 NFA。
  * `to` (string): 接收账号。
  * `nfa_id` (int64): 转移的 NFA ID。
  * `enable_logger` (bool): 是否打印日志。
* **`transfer_nfa_from_caller(to, nfa_id, enable_logger)`**: 从当前调用者转移 NFA。
  * 参数同上。
* **`approve_nfa_active_from_owner(to, nfa_id, enable_logger)`**: 从合约拥有者授权 NFA 的 active 权限。
  * `to` (string): 接收账号。
  * `nfa_id` (int64): 转移的 NFA ID。
  * `enable_logger` (bool): 是否打印日志。
* **`approve_nfa_active_from_caller(to, nfa_id, enable_logger)`**: 从当前调用者授权 NFA 的 active 权限。
  * 参数同上。

### NFA 管理

* **`create_nfa_symbol(symbol, describe, default_contract, max_count, min_equivalent_qi, is_sbt)`**: 创建NFA符号定义。
  * `symbol` (string): NFA 符号。
  * `describe` (string): 描述。
  * `default_contract` (string): 关联的初始合约名。
  * `max_count` (uint64): 最大发行量。
  * `min_equivalent_qi` (uint64): 最低等价“真气”，该NFA必须至少有等价这么多真气的材质才能正常运转。
  * `is_sbt` (bool): 是否为不可交易证书 (Soulbound Token)。
* **`change_nfa_symbol_authority(symbol, authority_account)`**: 授权指定账号能创建该符号的NFA实体。
  * `symbol` (string): NFA 符号。
  * `authority_account` (string): 权限账号。
* **`change_nfa_symbol_authority_nfa_symbol(symbol, authority_symbol_name)`**: 授权持有某种 NFA 的账号能创建该符号的NFA实体。
  * `symbol` (string): NFA 符号。
  * `authority_symbol_name` (string): 权限NFA符号（持有该NFA的账号将获得权限）。
* **`get_nfa_contract(nfa_id)`**: 返回 NFA 关联的合约名。
* **`change_nfa_contract(nfa_id, contract_name)`**: 修改 NFA 关联的合约。
* **`create_nfa_to_actor(to_actor_nfa_id, symbol, data)`**: 创建NFA实体并发送至角色。
  * `to_actor_nfa_id` (int64): 接收角色的 NFA ID。
  * `symbol` (string): NFA 符号。
  * `data` (table): 初始私有数据。
* **`create_nfa_to_account(to_account_name, symbol, data)`**: 创建NFA实体并发送至账号。
  * `to_account_name` (string): 接收账号。
  * `symbol` (string): NFA 符号。
  * `data` (table): 初始私有数据。
* **`change_nfa_active_operator(nfa_id, account_name)`**: 修改 NFA 操作者。
  * `nfa_id` (int64): NFA ID。
  * `account_name` (string): 操作者账号。
* **`read_nfa_contract_data(nfa_id, read_list)`**: 读取 NFA 合约私有数据。
  * `nfa_id` (int64): NFA ID。
  * `read_list` (table): 读取路径映射。
  * `return` (table): 读取结果。
* **`write_nfa_contract_data(nfa_id, data, write_list)`**: 写入 NFA 合约私有数据。
  * `nfa_id` (int64): NFA ID。
  * `data` (table): 要写入的数据。
  * `write_list` (table): 指定写入路径的映射。
* **`is_nfa_action_exist(nfa_id, action)`**: 判断 NFA 合约中是否存在指定 Action。
  * `nfa_id` (int64): NFA ID。
  * `action` (string): Action 名称。
* **`eval_nfa_action(nfa_id, action, params)`**: 在只读模式下运行 NFA Action（仅计算出结果）。
  * `nfa_id` (int64): NFA ID。
  * `action` (string): Action 名称。
  * `params` (table): Action 输入参数。
  * `return` (table): Action 输出结果。
* **`do_nfa_action(nfa_id, action, params)`**: 执行 NFA Action。
  * `nfa_id` (int64): NFA ID。
  * `action` (string): Action 名称。
  * `params` (table): Action 输入参数。
* **`get_nfa_info(nfa_id)`**: 返回指定ID的NFA实体的 `contract_nfa_base_info`。
* **`is_nfa_valid(nfa_id)`**: 判断 NFA ID 是否指向有效对象。
* **`get_nfa_balance(nfa_id, symbol_name)`**: 获取指定NFA持有的资产余额。
* **`get_nfa_resources(nfa_id)` / `get_nfa_materials(nfa_id)`**: 获取指定NFA持有的资源/材料。
* **`list_nfa_inventory(nfa_id, symbol_name)`**: 列出指定NFA持有的所有子项 NFA（和子项是父子关系）。
  * `nfa_id` (int64): NFA ID。
  * `symbol_name` (string): 过滤的NFA 符号，空字符串表示返回所有子项。
  * `return` (table): 子项 NFA 信息结构的列表。
* **`get_nfa_location(nfa_id)`**: 获取指定NFA 所在区域。
  * `nfa_id` (int64): NFA ID。
  * `return` (string): 区域名称。

### 区域 (Zone)

* **`create_zone(name, type_name)`**: 创建区域。仅限“心素”。
  * `name` (string): 区域名。
  * `type_name` (string): 区域类型（如 "HAIYANG", "CUNZHUANG" 等）。
* **`change_zone_type(nfa_id, type_name)`**: 修改区域类型。仅限“心素”或者DAO账号
  * `nfa_id` (int64): 区域 NFA ID。
  * `type_name` (string): 区域类型（如 "HAIYANG", "CUNZHUANG" 等）。
* **`refine_zone(nfa_id)`**: 炼化区域，根据区域的材质情况改变区域的类型。仅限“心素”。
  * `nfa_id` (int64): 区域 NFA ID。
* **`get_zone_info(nfa_id)` / `get_zone_info_by_name(name)`**: 获取区域信息。
* **`is_zone_valid(nfa_id)` / `is_zone_valid_by_name(name)`**: 指定区域是否有效。
* **`connect_zones(from_zone_nfa_id, to_zone_nfa_id)`**: 在两个区域之间建立连接（路径）。仅限两个区域的拥有者或者操作者。
  * `from_zone_nfa_id` (int64): 起始区域 NFA ID。
  * `to_zone_nfa_id` (int64): 目标区域 NFA ID。
* **`list_actors_on_zone(nfa_id)`**: 列出指定区域的所有角色。
  * `nfa_id` (int64): 区域 NFA ID。
  * `return` (table): 区域中的角色信息列表。
* **`exploit_zone(actor_name, zone_name)`**: 角色探索区域。
  * `actor_name` (string): 角色名。探索要消耗角色的真气。
  * `zone_name` (string): 区域名。探索会自动以“danuo”账号权限发起对指定区域合约函数“on_actor_exploit”的调用，传入角色nfa的id作为参数。
* **`is_contract_allowed_by_zone(zone_name, contract_name)`**: 检查区域是否允许运行指定的合约。
* **`set_zone_contract_permission(zone_name, contract_name, allowed)`**: 设置区域允许运行指定的合约。仅限区域拥有者或者操作者。
  * `zone_name` (string): 区域名。
  * `contract_name` (string): 合约名。
  * `allowed` (bool): 是否允许指定合约在该区域运行。
* **`remove_zone_contract_permission(zone_name, contract_name)`**: 移除指定合约在区域上的运行权限设置，恢复到默认状态。
  * `zone_name` (string): 区域名。
  * `contract_name` (string): 合约名。
* **`set_zone_ref_prohibited_contract_zone(zone_name, ref_zone_name)`**: 设置区域参考另外一个区域的合约许可设置。
  * `zone_name` (string): 区域名。
  * `ref_zone_name` (string): 参考的区域名。

### 角色 (Actor)

* **`create_actor_talent_rule(contract_name)`**: 注册天赋规则类合约。
  * `contract_name` (string): 合约名。
* **`create_actor(family_name, last_name)`**: 创建新角色，初始化角色对象及其姓名。
  * `family_name` (string): 家姓。
  * `last_name` (string): 本名。
  * `return` (int64): 新角色 NFA ID。
* **`is_actor_valid(nfa_id)` / `is_actor_valid_by_name(name)`**: 校验角色是否有效。
* **`get_actor_info(nfa_id)` / `get_actor_info_by_name(name)`**: 获取角色基础信息。
* **`get_actor_core_attributes(nfa_id)`**: 获取角色核心属性。
* **`born_actor(name, gender, sexuality, init_attrs, zone_name)`**: 让指定角色出生。
  * `name` (string): 角色名。
  * `gender` (int): 性别。
  * `sexuality` (int): 性取向。
  * `init_attrs` (table): 初始分配的属性列表。
  * `zone_name` (string): 出生所在的区域名。
* **`move_actor(actor_name, zone_name)`**: 将角色移动至目标区域。
  * `actor_name` (string): 角色名。
  * `zone_name` (string): 目标区域名。

### 修行、备忘与治理

* **`create_cultivation(nfa_id, beneficiary_nfa_ids, beneficiary_shares, prepare_time_blocks)`**: 创建修行实例。
  * `nfa_id` (int64): 修行发起者 NFA ID。
  * `beneficiary_nfa_ids` (table): 受益 NFA ID 列表。
  * `beneficiary_shares` (table): 对应的收益占比。
  * `prepare_time_blocks` (uint32): 准备时间（区块数）。
* **`participate_cultivation(cult_id, nfa_id, value)`**: 参与修行并注入真气。
  * `cult_id` (int64): 修行实例 ID。
  * `nfa_id` (int64): 参与者 NFA ID。
  * `value` (uint64): 参与注入的真气值（锁定为修真资源，修真活动结束后退还）。
* **`start_cultivation(cult_id)`**: 开始修真活动。
  * `cult_id` (int64): 修行实例 ID。
* **`stop_and_close_cultivation(cult_id)`**: 停止并结算修真活动。
  * `cult_id` (int64): 修行实例 ID。
* **`update_cultivation(cult_id)`**: 更新修真状态（步进计算）。
  * `cult_id` (int64): 修行实例 ID。
* **`is_cultivation_exist(cult_id)`**: 检查修行实例是否存在。
  * `cult_id` (int64): 修行实例 ID。
* **`create_named_contract(name, code)`**: 创建一个全局命名的合约。
  * `name` (string): 合约名。
  * `code` (string): 合约代码。
* **`make_memo(receiver_account_name, key, value, ss, enable_logger)`**: 创建加密备忘录。
  * `receiver_account_name` (string): 接收者账号名。
  * `key` (string): 备忘录加密键（用于产生随机私钥）。
  * `value` (string): 备忘录内容字符串（明文）。
  * `ss` (uint64): nonce。
  * `enable_logger` (bool): 是否启用日志。
  * `return` (string): 加密后的备忘录（含有发送者公钥和接收者公钥，并进行签名和非对称加密）。
* **`make_release()`**: 设置当前合约为“已发行”状态。
* **`get_tiandao_property()`**: 返回 `contract_tiandao_property`。
* **`create_proposal(contract_name, function_name, params, subject, end_time)`**: 发起一个治理提案。仅限心素。
  * `contract_name` (string): 提案执行的目标合约名。
  * `function_name` (string): 提案执行的目标合约方法名。
  * `params` (table): 提案执行的参数列表。
  * `subject` (string): 提案主题。
  * `end_time` (uint64): 提案截止时间。
* **`update_proposal_votes(proposal_ids, approve)`**: 对指定提案进行投票。仅限心素。
  * `proposal_ids` (table): 提案 ID 列表。
  * `approve` (bool): 是否赞成。
* **`remove_proposals(proposal_ids)`**: 移除过期或作废提案。仅限心素。
  * `proposal_ids` (table): 提案 ID 列表。
* **`grant_xinsu(receiver_account)` / `revoke_xinsu(account_name)`**: 授予或撤销账号的心素身份。仅限DAO（坐忘道）集体账号。

---

## 4. NFA 处理器 API (`nfa_helper`)

`nfa_helper` 绑定到特定 NFA 运行，隐式包含该 NFA 上下文。

### 基础信息

* **`get_info()`**: 获取当前绑定的 NFA 基础信息。
* **`get_resources()`**: 获取 NFA 携带的基础资产。
* **`get_materials()`**: 获取 NFA 的材料资源。

### 生命周期与状态

* **`enable_tick()` / `disable_tick()`**: 启用或禁用该 NFA 的周期性回调（心跳）。
* **`destroy()`**: 彻底销毁该 NFA（使其变为无主NFA）。
* **`convert_qi_to_resource(amount, resource_symbol_name)`**: 将 NFA 内部积蓄的“真气”转化为指定的物质资源。
  * `amount` (uint64): 转化真气的数量。
  * `resource_symbol_name` (string): 目标物质资源的符号名。
* **`convert_resource_to_qi(amount, resource_symbol_name)`**: 将物质资源炼化为“真气”。
  * `amount` (uint64): 转化物质资源的数量。
  * `resource_symbol_name` (string): 物质资源的符号名。

### 层级管理

* **`add_child(nfa_id)`**: 将目标 NFA 收纳为当前 NFA 的子项（注意不是转移所有权，仅仅是层级/父子关系）。
* **`add_to_parent(parent_nfa_id)`**: 将当前 NFA 放入目标 NFA 内部（注意不是转移所有权，仅仅是层级/父子关系）。
* **`add_child_from_contract_owner(nfa_id)`**: 以合约拥有者身份进行收纳操作。
* **`add_to_parent_from_contract_owner(parent_nfa_id)`**: 以合约拥有者身份进行放入操作。
* **`remove_from_parent()`**: 从父项 NFA 中移除当前 NFA（注意不是转移所有权，仅仅是层级/父子关系）。

### NFA 数据读写

* **`read_contract_data(read_list)`**: 读取当前 NFA 的合约数据。
  * `read_list` (table): 要读取的合约数据字段列表。
* **`write_contract_data(data, write_list)`**: 写入当前 NFA 的合约数据。
  * `data` (table): 要写入的合约数据。
  * `write_list` (table): 要写入的合约数据字段列表。

### NFA 资产操作

* **`transfer_to(to_nfa_id, amount, symbol, enable_logger)`**: 将当前 NFA 的资产转移到目标 NFA。
  * `to_nfa_id` (int64): 目标 NFA 的 ID。
  * `amount` (uint64): 转移的资产数量。
  * `symbol` (string): 资产的符号名。
  * `enable_logger` (bool): 是否启用日志。
* **`inject_material_to(to_nfa_id, amount, symbol, enable_logger)`**: 向目标 NFA 注入材料。
  * `to_nfa_id` (int64): 目标 NFA 的 ID。
  * `amount` (uint64): 注入的材料数量。
  * `symbol` (string): 材料的符号名。
  * `enable_logger` (bool): 是否启用日志。
* **`separate_material_out(to_nfa_id, amount, symbol, enable_logger)`**: 从当前 NFA 析出材料并转移到目标 NFA。
  * `to_nfa_id` (int64): 目标 NFA 的 ID。
  * `amount` (uint64): 析出的材料数量。
  * `symbol` (string): 材料的符号名。
  * `enable_logger` (bool): 是否启用日志。
* **`deposit_from_owner(amount, symbol, enable_logger)`**: 从合约拥有者通过合约向当前 NFA 充值。
  * `amount` (uint64): 充值的资产数量。
  * `symbol` (string): 资产的符号名。
  * `enable_logger` (bool): 是否启用日志。
* **`deposit_from(from, amount, symbol, enable_logger)`**: 从指定账号通过合约向当前 NFA 充值。
  * `from` (string): 充值来源账号。注意当前发起调用的隐含账号必须就是from。
  * `amount` (uint64): 充值的资产数量。
  * `symbol` (string): 资产的符号名。
  * `enable_logger` (bool): 是否启用日志。
* **`withdraw_to(to, amount, symbol, enable_logger)`**: 从当前 NFA 提现至目标账号。当前发起调用的账号必须是当前 NFA 的拥有者或者操作者。
  * `to` (string): 提现目标账号。
  * `amount` (uint64): 提现的资产数量。
  * `symbol` (string): 资产的符号名。
  * `enable_logger` (bool): 是否启用日志。

### 角色属性与交互 (针对 Actor NFA)

* **`get_actor_relation_info(target_actor_name)`**: 获取当前角色与目标角色的关系。
  * `target_actor_name` (string): 目标角色的名称。
* **`modify_actor_attributes(values, max_values)`**: 修改当前角色的基础属性。
  * `values` (table): 要修改的基础属性偏移量，可以同时修改多个属性。
  * `max_values` (table): 要修改的最大值偏移量，可以同时修改多个属性。
* **`modify_actor_relation_values(target_actor_name, values)`**: 修改与目标角色的关系数值。
  * `target_actor_name` (string): 目标角色的名称。
  * `values` (table): 要修改的关系数值，可以同时修改多个关系属性。
* **`talk_to_actor(target_actor_name, something)`**: 对指定角色说话（产生叙事文字或交互逻辑）。
  * `target_actor_name` (string): 目标角色的名称。
  * `something` (string): 要说的话。
  * 该方法会自动调用说话方合约的“on_actor_talking”方法和听话方合约的“on_actor_listening”方法。
* **`get_actor_talent_trigger_number(talent_rule_id)`**: 获取特定天赋规则在该角色身上的触发/计数器值。
* **`set_actor_talent_trigger_number(talent_rule_id, num)`**: 设置特定天赋规则在该角色身上的触发/计数器值。

### 动态调用

* **`eval_action(action, params)`** / **`do_action(action, params)`**: 计算函数或者动作执行。执行主体为当前合约绑定的 NFA 实体。
  * `action` (string): 动作名称。
  * `params` (table): 动作参数。
* **`create_nfa_to_actor(to_actor_nfa_id, symbol, data)`**:  创建 NFA。新创建的 NFA 持有者初始为指定目标角色 NFA 的持有者。调用发起者要有对应符号NFA的创建权限。
  * `to_actor_nfa_id` (int64): 目标角色的 NFA ID。
  * `symbol` (string): NFA 的符号名。
  * `data` (table): NFA 的数据。
* **`create_nfa_to_account(to_account_name, symbol, data)`**: 创建 NFA。新创建的 NFA 持有者初始为指定目标账号。调用发起者要有对应符号NFA的创建权限。
  * `to_account_name` (string): 目标账号。
  * `symbol` (string): NFA 的符号名。
  * `data` (table): NFA 的数据。

---

## 5. 注意事项

1. **权限控制**: 许多涉及资产转移和 NFA 修改的方法（如 `transfer_from_owner`, `withdraw_to`）在 C++ 层有严格的权限校验，通常要求调用者拥有相应的授权。
2. **只读模式 (`is_in_eval`)**: 当合约在 `eval` 模式下运行时，写入操作（如 `write_contract_data`）可能无效或抛出异常。
3. **数据消耗**: 合约数据的写入会消耗 CPU 资源和存储配额，应避免在单个交易中进行大规模的数据操作。
4. **行为 (Action)**: eval或者do_action调用的动作行为，在目标合约中要以“eval_行为名”或者“do_行为名”来命名实际的动作函数。在目标合约顶部也要申明动作行为是否是会导致状态空间因果变化的（即可写还是只读）。
  
  例如某种角色的合约顶部是这样的：

```lua
welcome = { consequence = false }
look = { consequence = false }
view = { consequence = false }
inventory = { consequence = false }
hp = { consequence = false }
resource = { consequence = false }
map = { consequence = false }
help = { consequence = false }

go = { consequence = true }
deposit_qi = { consequence = true }
withdraw_qi = { consequence = true }
exploit = { consequence = true }
start_cultivation = { consequence = true }
stop_cultivation = { consequence = true }
eat = { consequence = true }
touch = { consequence = true }
active = { consequence = true }
talk = { consequence = true }
```

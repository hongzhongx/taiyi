#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/tiandao_property_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/zone_objects.hpp>

#include <fc/macros.hpp>

#include <limits>
#include <numeric>
#include <random>

namespace taiyi { namespace chain {
    
    // 五行属性结构体
    struct FiveElements {
        int64_t metal;
        int64_t wood;
        int64_t water;
        int64_t fire;
        int64_t earth;
    };

    // 定义每种区域类型的基础五行亲和度
    // 注意：这些值是设计的核心，调整它们会极大影响生成结果的风格
    const std::map<E_ZONE_TYPE, FiveElements> ZONE_AFFINITY = {
        // 自然环境
        {YUANYE,   {2, 4, 3, 3, 8}},   // 原野: 土、木为主
        {HUPO,     {1, 2, 10, 1, 4}},  // 湖泊: 水权重极高
        {NONGTIAN, {1, 6, 5, 2, 8}},   // 农田: 土、木、水均衡
        {LINDI,    {2, 8, 3, 2, 5}},   // 林地: 木为主
        {MILIN,    {1, 10, 4, 2, 5}},  // 密林: 木权重极高
        {YUANLIN,  {3, 7, 5, 3, 6}},   // 园林: 更均衡的林地
        {SHANYUE,  {6, 3, 2, 2, 10}},  // 山岳: 土、金权重高
        {DONGXUE,  {7, 1, 3, 1, 9}},   // 洞穴: 土、金为主，幽暗
        {SHILIN,   {8, 1, 1, 2, 8}},   // 石林: 金、土权重极高
        {QIULIN,   {3, 5, 3, 2, 7}},   // 丘陵: 土、木为主
        {TAOYUAN,  {4, 8, 6, 5, 7}},   // 桃源: 五行均衡且旺盛
        {SANGYUAN, {2, 9, 5, 2, 7}},   // 桑园: 木、土为主
        {XIAGU,    {5, 3, 6, 2, 7}},   // 峡谷: 金、土、水
        {ZAOZE,    {1, 5, 8, 1, 6}},   // 沼泽: 水、土、木
        {YAOYUAN,  {5, 8, 6, 4, 7}},   // 药园: 木、土为主，需要金来收获，水火滋养
        {HAIYANG,  {3, 1, 12, 2, 1}},  // 海洋: 水权重最高
        {SHAMO,    {4, 1, 0, 10, 8}},  // 沙漠: 火、土权重高，缺水
        {HUANGYE,  {3, 2, 1, 5, 6}},   // 荒野: 五行驳杂且贫瘠
        {ANYUAN,   {6, 2, 8, 7, 8}}   // 暗渊: 金、水、火、土，阴暗之地

        // 人造环境
//        {DUHUI,    {10, 4, 5, 6, 8}},  // 都会: 金(建筑/繁华)、土(基础)、火(人气)
//        {MENPAI,   {8, 7, 6, 5, 9}},   // 门派: 通常建在五行充沛之地
//        {SHIZHEN,  {7, 4, 4, 5, 7}},   // 市镇: 小都会
//        {GUANSAI,  {9, 2, 2, 4, 8}},   // 关寨: 金(兵器/防御)、土(城墙)
//        {CUNZHUANG,{2, 5, 4, 3, 6}}    // 村庄: 土、木、水为主，生活气息
    };
    //=============================================================================
    /**
        当一个区域的多种五行属性都很高时，它们之间会产生相互作用：
        - 相生: 一种属性会增强它所“生”的另一种属性的有效性。
        - 相克: 一种属性会削弱它所“克”的另一种属性的有效性。

        相生循环:
        - 木生火
        - 火生土
        - 土生金
        - 金生水
        - 水生木

        相克循环:
        - 木克土
        - 土克水
        - 水克火
        - 火克金
        - 金克木
     
        先根据生克规则，将输入的“基础五行值”转换为“有效五行值”。
        一个元素的有效值取决于三个部分：它的基础值、”生我“的元素带来的增益、以及”克我“的元素带来的削弱。

        公式：
            有效值 = max(0, 基础值 + k生 × “生我”的元素值− k克 × “克我”的元素值)
        其中：
            k生 (k_gen) 是生成系数，代表”相生“的转化效率。
            k克 (k_over) 是克制系数，代表”相克“的削弱强度。

        使用 max(0, ...) 是为了确保有效值不会成为负数。

        通常，为了让“克制”效果更明显，可以将 k_text克 设置得比 k_text生 稍大一些。
     */
    
    /**
     * @brief 根据五行生克规则，计算有效五行值
     * @param base_elements 基础五行数值
     * @param k_gen 生成系数
     * @param k_over 克制系数
     * @return FiveElements 计算出的有效五行值
     */
    FiveElements CalculateEffectiveElements(const FiveElements& base_elements, int k_gen, int k_over) {
        FiveElements effective_elements;

        // 1. 计算金的有效值: 土生金，火克金
        effective_elements.metal = base_elements.metal + (k_gen * base_elements.earth - k_over * base_elements.fire) / 100;

        // 2. 计算木的有效值: 水生木，金克木
        effective_elements.wood = base_elements.wood + (k_gen * base_elements.water - k_over * base_elements.metal) / 100;

        // 3. 计算水的有效值: 金生水，土克水
        effective_elements.water = base_elements.water + (k_gen * base_elements.metal - k_over * base_elements.earth) / 100;

        // 4. 计算火的有效值: 木生火，水克火
        effective_elements.fire = base_elements.fire + (k_gen * base_elements.wood - k_over * base_elements.water) / 100;

        // 5. 计算土的有效值: 火生土，木克土
        effective_elements.earth = base_elements.earth + (k_gen * base_elements.fire - k_over * base_elements.wood) / 100;

        // 确保所有有效值都不小于0
        effective_elements.metal = std::max<int64_t>(0, effective_elements.metal);
        effective_elements.wood  = std::max<int64_t>(0, effective_elements.wood);
        effective_elements.water = std::max<int64_t>(0, effective_elements.water);
        effective_elements.fire  = std::max<int64_t>(0, effective_elements.fire);
        effective_elements.earth = std::max<int64_t>(0, effective_elements.earth);

        return effective_elements;
    }
    //=============================================================================
    /**
     * @brief 根据五行生克和随机种子概率性地判定区域类型
     * @param elements 输入的区域基础五行数值
     * @param seed 用于随机数生成的种子
     * @return E_ZONE_TYPE 判定出的区域类型
     */
    E_ZONE_TYPE DetermineZoneTypeWithWuxing(const FiveElements& elements, uint32_t seed) {
        // 定义生克系数
        const int K_GENERATION = 25; // 生成系数 0.25
        const int K_OVERCOMING = 40; // 克制系数，0.4 通常比生成系数大

        // 步骤1: 计算有效五行值
        FiveElements effective_elements = CalculateEffectiveElements(elements, K_GENERATION, K_OVERCOMING);

        // 步骤2: 使用有效五行值进行后续计算
        int64_t total_elements = effective_elements.metal + effective_elements.wood + effective_elements.water + effective_elements.fire + effective_elements.earth;
        // 特殊情况处理：如果五行总值过低，直接判定为虚空
        if (total_elements < 500) { // 这个阈值可以调整
            return XUKONG;
        }

        std::map<E_ZONE_TYPE, int64_t> zone_scores;
        int64_t total_score = 0;

        // 1. 计算每个区域类型的匹配分数
        for (const auto& pair : ZONE_AFFINITY) {
            E_ZONE_TYPE type = pair.first;
            const FiveElements& affinity = pair.second;

            // 使用有效五行值计算分数。计算点积作为基础分数，这能很好地反映“占比”和“绝对值”
            // 输入元素的向量与类型亲和度的向量越接近，分数越高
            int64_t score = effective_elements.metal * affinity.metal +
                           effective_elements.wood  * affinity.wood +
                           effective_elements.water * affinity.water +
                           effective_elements.fire  * affinity.fire +
                           effective_elements.earth * affinity.earth;
            
            // 为了避免负分，并增加随机性，可以进行一些处理
            // 使用max(0, score)，并给一个小的基础分避免概率为0
            score = std::max<int64_t>(0, score) + 1;

            zone_scores[type] = score;
            total_score += score;
        }

        if (total_score <= 0) {
            // 如果所有分数都为0，也返回虚空
            return XUKONG;
        }

        std::mt19937_64 gen(seed);
        int64_t random_pick = gen() % total_score;

        int64_t cumulative_score = 0;
        for (const auto& pair : zone_scores) {
            cumulative_score += pair.second;
            if (random_pick <= cumulative_score) {
                return pair.first;
            }
        }

        return _ZONE_INVALID_TYPE;
    }
    //=============================================================================
    E_ZONE_TYPE g_generate_zone_refining_result(const zone_object& zone, const nfa_material_object& zone_materials, uint32_t seed)
    {
        if (zone.type >= _NATURE_ZONE_TYPE_NUM)
            return zone.type; //人造区域类型不变
        
        FiveElements eles = {
            zone_materials.gold.amount.value,
            zone_materials.wood.amount.value,
            zone_materials.fabric.amount.value,
            zone_materials.herb.amount.value,
            zone_materials.food.amount.value
        };
        
        return DetermineZoneTypeWithWuxing(eles, seed);
    }
    
} } // taiyi::chain

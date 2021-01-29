 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <type_traits>

namespace ayj {

using namespace std;
using namespace eosio;

static constexpr eosio::name active_perm{"active"_n};
static constexpr eosio::name SYS_BANK{"eosio.token"_n};

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("MGP"), 4);
static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t share_boost           = 10000;

#define CONTRACT_TBL [[eosio::table, eosio::contract("aiyunji.mall")]]

struct CONTRACT_TBL reward_plan_t {
    uint16_t ratio;
    bool mining;
};

struct [[eosio::table("config"), eosio::contract("aiyunji.mall")]] config_t {
    uint16_t withdraw_fee_ratio        = 3000;  //boost by 10000

    reward_plan_t customer_plan        = {4500, true };
    reward_plan_t shop_sunshine_plan   = {1500, false};
    reward_plan_t shop_top10_plan      = {500,  false};
    reward_plan_t newly_certified_plan = {800,  false};
    reward_plan_t pltform_top1k_plan   = {500,  false};
    reward_plan_t direct_invite_plan   = {1000, true };
    reward_plan_t direct_agent_plan    = {500,  true };
    reward_plan_t city_center_plan     = {300,  false};
    reward_plan_t ram_usage_plan       = {400,  false};
   
    name platform_account;

    config_t() {
    }

    // EOSLIB_SERIALIZE( config_t, (donate_coin_expiry_days) )
};
typedef eosio::singleton< "config"_n, config_t > config_singleton;

struct [[eosio::table("global"), eosio::contract("aiyunji.mall")]] global_t {
    asset ram_usage_share;
    asset platform_top_share;
    asset certified_user_share;

    global_t() {
    }

    EOSLIB_SERIALIZE( global_t, (ram_usage_share)(platform_top_share)(certified_user_share) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

/** 
 * 1. customer spending
 * 2. agent refeerral
 */
struct CONTRACT_TBL user_t {
    name user;
    name invited_by;
    asset share;
    time_point_sec updated_at;

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return user.value; }

};

struct CONTRACT_TBL citycenter_share_t {
    uint64_t id;
    name city_center_account;
    string city_center_name;
    asset share;
    time_point_sec updated_at;

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }

};

struct CONTRACT_TBL shop_t {
    uint64_t id;                //shop_id
    uint64_t parent_id;         //0 means top
    name shop_account;          //shop funds account
    name agent_account;
    asset shop_sunshine_share;
    asset shop_top_share;
    time_point_sec created_at;

    shop_t() {}
    shop_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }
    uint64_t by_parent_id() const { return parent_id; }
    uint64_t by_shop_agent() const { return agent_account.value; }
};

/**
 * accumulated spending for each customer and shop
 *  Top 1000 can be derived within
 */
struct CONTRACT_TBL total_spending_t {
    uint64_t id;
    uint64_t shop_id;
    name customer;
    asset spending;  //accumulated ever since beginning
    time_point_sec updated_at;

    uint64_t scope() const { return 0; }  //platform-wise spending sorting, to derive top1000
    uint64_t primary_key() const { return id; }
    uint128_t shop_customer_key() const { return ((uint128_t)shop_id << 64) | customer.value; } //to ensure uniqueness

    uint64_t by_spending() const { return std::numeric_limits<uint64_t>::max() - spending.amount; } //descending
};

struct CONTRACT_TBL day_spending_t {
    uint64_t id;
    uint64_t shop_id;
    name customer;
    asset spending; //accumulated for a customer in a shop in a day
    time_point_sec updated_at;

    uint64_t scope() const { return shop_id; } //shop-wise spending sorting, to derive top10
    uint64_t primary_key() const { return id; }
    uint128_t day_customer_key() { return (uint128_t (updated_at.sec_since_epoch() % seconds_per_day) << 64) | customer.value; } //to ensure uniqueness

    uint64_t by_spending() const { return std::numeric_limits<uint64_t>::max() - spending.amount; }
   
};

struct CONTRACT_TBL day_certified_t {
    name user;
    time_point_sec certified_at;

    uint64_t scope() const { return 0; } //shop-wise spending sorting, to derive top10
    uint64_t primary_key() const { return user.value; }
};

// typedef eosio::multi_index
//     < "buyorders"_n,  order_t,
//         indexed_by<"price"_n, const_mem_fun<order_t, uint64_t, &order_t::by_invprice> >,
//         indexed_by<"maker"_n, const_mem_fun<order_t, uint64_t, &order_t::by_maker> >
//     > buy_order_t;
    
/**
 * reset after reward, but reward might not happen as scheduled
 * hence there could be accumulation or records within
 */
struct CONTRACT_TBL shop_top_pool_t {
    uint64_t id;
    name customer;
    uint64_t spening;
    time_point_sec spent_at;
};

/**
 * reset after reward, but reward might not happen as scheduled
 * hence there could be accumulation or records within
 */
struct CONTRACT_TBL certified_user_pool_t {
    name account;
    time_point_sec certified_at;
};

struct CONTRACT_TBL platform_top_pool_t {
    name account;
    time_point_sec certified_at;
};



    // struct TABLE_IN_CONTRACT store_customer_t {
    //     uint64_t    store_id;
    //     regid       customer;

    //     uint64_t    acc_consume_points;

    //     uint64_t primary_key()const { return customer.value; }
    //     regid scope()const { return store_id; }

    //     store_customer_t() {}
    //     store_customer_t(uint64_t s, regid c): store_id(s), customer(s) {}

    //     typedef wasm::table< "stcustomer"_n, store_customer_t, uint64_t > table_t;

    //     EOSLIB_SERIALIZE( store_customer_t,   (store_id)(customer)(acc_consume_points) )
    // };

    // struct TABLE_IN_CONTRACT customer_t {
    //     regid       user;                  //PK

    //     regid       inviter;
    //     uint64_t    created_at;

    //     uint64_t primary_key()const { return user.value; }
    //     uint64_t scope()const { return DELIFE_SCOPE; }

    //     customer_t(regid u): user(u) {}
    //     customer_t() {}

    //     typedef wasm::table< "customer"_n, project_t, uint64_t > table_t;

    //     EOSLIB_SERIALIZE( customer_t,   (user)(inviter)(created_at) )
    // };



// typedef eosio::multi_index
// < "votes"_n, vote_t,
//     indexed_by<"voter"_n,        const_mem_fun<vote_t, uint64_t, &vote_t::by_voter>             >,
//     indexed_by<"candidate"_n,    const_mem_fun<vote_t, uint64_t, &vote_t::by_candidate>         >,
//     indexed_by<"voteda"_n,       const_mem_fun<vote_t, uint64_t, &vote_t::by_voted_at>          >,
//     indexed_by<"unvoteda"_n,     const_mem_fun<vote_t, uint64_t, &vote_t::by_unvoted_at>        >,
//     indexed_by<"restarted"_n,    const_mem_fun<vote_t, uint64_t, &vote_t::by_restarted_at>      >,
//     indexed_by<"electround"_n,   const_mem_fun<vote_t, uint64_t, &vote_t::by_election_round>    >,
//     indexed_by<"rewardround"_n,  const_mem_fun<vote_t, uint64_t, &vote_t::by_reward_round>      >
// > vote_tbl;


}
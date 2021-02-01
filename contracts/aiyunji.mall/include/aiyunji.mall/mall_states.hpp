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
static constexpr symbol   HST_SYMBOL            = symbol(symbol_code("HST"), 4);
static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t ratio_boost           = 10000;
static constexpr uint32_t MAX_STEP              = 20;

#define CONTRACT_TBL [[eosio::table, eosio::contract("aiyunji.mall")]]

struct [[eosio::table("config"), eosio::contract("aiyunji.mall")]] config_t {
    uint16_t withdraw_fee_ratio         = 3000;  //boost by 10000
    uint16_t withdraw_mature_days       = 1;
    vector<uint64_t> allocation_ratios  = {
        4500,   //user share
        1500,   //shop_sunshine share
        500,    //shop_top share
        800,    //certified share
        500,    //pltform_top share
        1000,   //direct_referral share
        500,    //direct_agent share
        300,    //city_center share
        400     //ram_usage share
    };
    uint8_t ITR_MAX                     = 10;
    uint16_t platform_top_count         = 1000;
    uint16_t shop_top_count             = 10;

    name platform_admin;
    name platform_account;
    name mall_bank;  //bank for the mall

    config_t() {
    }
        
};
typedef eosio::singleton< "config"_n, config_t > config_singleton;

struct [[eosio::table("global"), eosio::contract("aiyunji.mall")]] global_t {
    asset platform_total_spending       = asset(0, HST_SYMBOL);
    asset platform_top_share            = asset(0, HST_SYMBOL);
    asset ram_usage_share               = asset(0, HST_SYMBOL);
    asset lucky_draw_share              = asset(0, HST_SYMBOL);
    asset certified_user_share          = asset(0, HST_SYMBOL);
    uint64_t certified_user_count       = 0;

    global_t() {}
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct [[eosio::table("global2"), eosio::contract("aiyunji.mall")]] global2_t {
    uint64_t last_reward_shop_id                = 0;
    uint64_t last_sunshine_reward_spending_id   = 0;
    uint64_t last_platform_top_reward_step      = 0;
    time_point_sec last_shop_reward_finished_at;
    time_point_sec last_certified_reward_finished_at;
    time_point_sec last_platform_reward_finished_at;

    global2_t() {}
};
typedef eosio::singleton< "global2"_n, global2_t > global2_singleton;

struct CONTRACT_TBL citycenter_t {
    uint64_t id;
    name citycenter_account;
    string citycenter_name;
    asset share;
    time_point_sec updated_at;
    
    citycenter_t() {}
    citycenter_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }

    typedef eosio::multi_index<"citycenters"_n, citycenter_t> tbl_t;

    EOSLIB_SERIALIZE( citycenter_t, (id)(citycenter_account)(citycenter_name)(share)(updated_at) )
};

struct CONTRACT_TBL user_t {
    name account;
    name referral_account;
    asset spending_reward           = asset(0, HST_SYMBOL);
    asset customer_referral_reward  = asset(0, HST_SYMBOL);
    asset shop_referral_reward      = asset(0, HST_SYMBOL);
    time_point_sec created_at;
    time_point_sec updated_at;

    user_t(){}
    user_t(const name& a): account(a) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return account.value; }

    typedef eosio::multi_index<"users"_n, user_t> tbl_t;

    EOSLIB_SERIALIZE( user_t,   (account)(referral_account)
                                (spending_reward)(customer_referral_reward)(shop_referral_reward)
                                (created_at)(updated_at) )
};

struct CONTRACT_TBL shop_t {
    uint64_t id;                //shop_id
    uint64_t parent_id;         //0 means top
    uint64_t citycenter_id;
    name shop_account;          //shop funds account
    name referral_account;
    
    asset total_customer_spending           = asset(0, HST_SYMBOL);
    asset total_customer_day_spending       = asset(0, HST_SYMBOL);
    asset shop_sunshine_share               = asset(0, HST_SYMBOL);
    asset shop_top_share                    = asset(0, HST_SYMBOL);;

    uint64_t last_sunshine_reward_spending_id;
    time_point_sec created_at;
    time_point_sec updated_at;
    time_point_sec rewarded_at;

    shop_t() {}
    shop_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }
    // uint64_t by_parent_id() const { return parent_id; }
    // uint64_t by_referral_agent() const { return referral_account.value; }

    typedef eosio::multi_index<"shops"_n, shop_t> tbl_t;

    EOSLIB_SERIALIZE( shop_t,   (id)(parent_id)(citycenter_id)(shop_account)(referral_account)
                                (total_customer_spending)(total_customer_day_spending)
                                (shop_sunshine_share)(shop_top_share)(last_sunshine_reward_spending_id)
                                (created_at)(updated_at)(rewarded_at) )
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
    time_point_sec spent_at;

    uint64_t scope() const { return 0; }  //platform-wise spending sorting, to derive top1000
    uint64_t primary_key() const { return id; }
    uint128_t shop_customer_key() const { return ((uint128_t)shop_id << 64) | customer.value; } //to ensure uniqueness

    uint128_t by_shop_spending() const { return (uint128_t)shop_id << 64 | id; }
    uint64_t by_spending() const { return std::numeric_limits<uint64_t>::max() - spending.amount; } //descending
 
    typedef eosio::multi_index
    < "totalspends"_n, total_spending_t,
        indexed_by<"shopcustomer"_n, const_mem_fun<total_spending_t, uint128_t, &total_spending_t::shop_customer_key> >,
        indexed_by<"shopspends"_n, const_mem_fun<total_spending_t, uint128_t, &total_spending_t::by_shop_spending> >,
        indexed_by<"spends"_n, const_mem_fun<total_spending_t, uint64_t, &total_spending_t::by_spending> >
    > tbl_t;

};

struct CONTRACT_TBL day_spending_t {
    uint64_t id;
    uint64_t shop_id;
    name customer;
    asset spending; //accumulated for a customer in a shop in a day
    time_point_sec spent_at;

    uint64_t scope() const { return shop_id; } //shop-wise spending sorting, to derive top10
    uint64_t primary_key() const { return id; }
    uint128_t day_customer_key() const { return (uint128_t (spent_at.sec_since_epoch() % seconds_per_day) << 64) | customer.value; } //to ensure uniqueness

    uint64_t by_spending() const { return std::numeric_limits<uint64_t>::max() - spending.amount; }
   
    typedef eosio::multi_index
    < "dayspends"_n, day_spending_t,
        indexed_by<"daycustomer"_n, const_mem_fun<day_spending_t, uint128_t, &day_spending_t::day_customer_key> >,
        indexed_by<"spends"_n,      const_mem_fun<day_spending_t, uint64_t, &day_spending_t::by_spending> >
    > tbl_t;
    
};

struct CONTRACT_TBL day_certified_t {
    name user;
    time_point_sec certified_at;

    day_certified_t() {}
    day_certified_t(const name& u): user(u) { certified_at = time_point_sec( current_time_point() ); }

    uint64_t scope() const { return 0; } //shop-wise spending sorting, to derive top10
    uint64_t primary_key() const { return user.value; }

    typedef eosio::multi_index<"certusers"_n, day_certified_t> tbl_t;

    EOSLIB_SERIALIZE( day_certified_t, (user)(certified_at) )
};
    
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



}
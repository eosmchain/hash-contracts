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

namespace hst {

using namespace std;
using namespace eosio;
using uint256_t = fixed_bytes<32>;

static constexpr eosio::name active_perm{"active"_n};
static constexpr eosio::name SYS_BANK{"eosio.token"_n};
static constexpr eosio::name HST_BANK{"hst.token"_n};

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("MGP"), 4);
static constexpr symbol   HST_SYMBOL            = symbol(symbol_code("HST"), 4);
static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_halfday   = 12 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t ratio_boost           = 10000;
static constexpr uint32_t MAX_STEP              = 20;

#define CONTRACT_TBL [[eosio::table, eosio::contract("hst.mall")]]

struct [[eosio::table("config"), eosio::contract("hst.mall")]] config_t {
    uint16_t withdraw_fee_ratio         = 3000;  //boost by 10000
    uint16_t withdraw_mature_days       = 1;
    vector<uint64_t> allocation_ratios  = {
        4500,   //0: user share
        1500,   //1: shop_sunshine share
        800,    //2: shop_top share
        500,    //3: certified share
        500,    //4: pltform_top share
        1000,   //5: direct_referral share
        500,    //6: direct_agent share
        300,    //7: city_center share
        400     //8: ram_usage share
    };
    uint16_t platform_top_count         = 1000;
    name platform_admin;
    name platform_fee_collector;
    name platform_referrer;
    name mall_bank;  //bank for the mall

    config_t() {}

    EOSLIB_SERIALIZE(config_t,  (withdraw_fee_ratio)(withdraw_mature_days)(allocation_ratios)
                                (platform_top_count)(platform_admin)(platform_fee_collector)(platform_referrer)(mall_bank) )
        
};
typedef eosio::singleton< "config"_n, config_t > config_singleton;

struct platform_share_t {
    asset total_share               = asset(0, HST_SYMBOL);
    asset top_share                 = asset(0, HST_SYMBOL);
    asset ram_usage_share           = asset(0, HST_SYMBOL);
    asset lucky_draw_share          = asset(0, HST_SYMBOL);
    asset certified_user_share      = asset(0, HST_SYMBOL);
    uint64_t certified_user_count   = 0;

    void reset() {
        total_share.amount          = 0;
        top_share.amount            = 0;
        ram_usage_share.amount      = 0;
        lucky_draw_share.amount     = 0;
        certified_user_share.amount = 0;
        certified_user_count        = 0;
    }
};

struct [[eosio::table("global"), eosio::contract("hst.mall")]] global_t {
    bool executing = false;
    platform_share_t platform_share;
    platform_share_t platform_share_cache; //cached in executing
    bool share_cache_updated = false;

    global_t() {}

    EOSLIB_SERIALIZE(global_t,  (executing)(platform_share)(platform_share_cache)(share_cache_updated) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct [[eosio::table("global2"), eosio::contract("hst.mall")]] global2_t {
    uint64_t last_reward_shop_id                = 0;
    uint64_t last_sunshine_reward_spending_id   = 0;
    uint64_t last_platform_top_reward_step      = 0;
    uint128_t last_platform_top_reward_id       = 0;    //user_t idx id
    uint64_t last_certification_reward_step     = 0;

    uint128_t last_cache_update_useridx         = 0;
    uint128_t last_cache_update_shopidx         = 0;
    uint128_t last_cache_update_spendingidx     = 0;

    time_point_sec last_shops_rewarded_at;
    time_point_sec last_certification_rewarded_at;
    time_point_sec last_platform_reward_finished_at;

    global2_t() {}

    EOSLIB_SERIALIZE(global2_t, (last_reward_shop_id)(last_sunshine_reward_spending_id)
                                (last_platform_top_reward_step)(last_platform_top_reward_id)(last_certification_reward_step)
                                (last_cache_update_useridx)(last_cache_update_shopidx)(last_cache_update_spendingidx)
                                (last_shops_rewarded_at)(last_certification_rewarded_at)(last_platform_reward_finished_at) )
                                
};
typedef eosio::singleton< "global2"_n, global2_t > global2_singleton;

struct CONTRACT_TBL citycenter_t {
    uint64_t id;    //PK
    string cc_name;
    name admin;
    time_point_sec created_at;

    citycenter_t() {}
    citycenter_t(const uint64_t& ccid): id(ccid) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }

    typedef eosio::multi_index<"citycenter"_n, citycenter_t> tbl_t;

    EOSLIB_SERIALIZE( citycenter_t, (id)(cc_name)(admin)(created_at) )
};

struct user_share_t {
    asset spending_share           = asset(0, HST_SYMBOL);
    asset customer_referral_share  = asset(0, HST_SYMBOL);
    asset shop_referral_share      = asset(0, HST_SYMBOL);

    asset total_share() const { //used for top 1000 split
        return spending_share + customer_referral_share + shop_referral_share; 
    }
};
struct CONTRACT_TBL user_t {
    name account;
    name referral_account;
    set<uint64_t> owned_shops;    //max 100
    user_share_t share;
    user_share_t share_cache;
    bool share_cache_updated = false;
    time_point_sec created_at;
    time_point_sec updated_at;

    user_t(){}
    user_t(const name& a): account(a) {}
 
    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return account.value; }

    ///to derive platform top 1000
    uint128_t by_total_share() const {
        auto total = share_cache.total_share();
        return (uint128_t (std::numeric_limits<uint64_t>::max() - total.amount)) << 64 | account.value; 
    }

    uint128_t by_cache_update() const { return (uint128_t) (share_cache_updated ? 1 : 0) << 64 | account.value; }

    typedef eosio::multi_index<"users"_n, user_t,
        indexed_by<"totalshare"_n, const_mem_fun<user_t, uint128_t, &user_t::by_total_share> >,
        indexed_by<"cacheupdt"_n,    const_mem_fun<user_t, uint128_t, &user_t::by_cache_update>  >
    > tbl_t;

    EOSLIB_SERIALIZE( user_t,   (account)(referral_account)(owned_shops)(share)(share_cache)(share_cache_updated)
                                (created_at)(updated_at) )
};

typedef name reward_type_t;
static reward_type_t NEW_CERT_REWARD        = "newcert"_n;
static reward_type_t SHOP_SUNSHINE_REWARD   = "shopsunshine"_n;
static reward_type_t SHOP_TOP_REWARD        = "shoptop"_n;
static reward_type_t PLATFORM_TOP_REWARD    = "platformtop"_n;

struct CONTRACT_TBL reward_t {
    uint64_t id;
    name account;
    reward_type_t reward_type;
    asset reward_quantity;
    time_point_sec rewarded_at;

    reward_t(){}
    reward_t(const name& a): account(a) {}
 
    uint64_t by_reward_time() const { return rewarded_at.sec_since_epoch(); }
    uint128_t by_account_reward_time() const { 
        return (uint128_t) account.value << 64 | rewarded_at.sec_since_epoch();
    }

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }
    typedef eosio::multi_index<"rewards"_n, reward_t,
        indexed_by<"rewardat"_n, const_mem_fun<reward_t, uint64_t, &reward_t::by_reward_time> >,
        indexed_by<"acctrewardat"_n, const_mem_fun<reward_t, uint128_t, &reward_t::by_account_reward_time>  >
    > tbl_t;

    EOSLIB_SERIALIZE( reward_t, (id)(account)(reward_type)(reward_quantity)(rewarded_at) )
};

struct shop_share_t {
    asset total_spending                    = asset(0, HST_SYMBOL); //TODO: remove in release
    asset day_spending                      = asset(0, HST_SYMBOL); //TODO: remove in release
    asset sunshine_share                    = asset(0, HST_SYMBOL);
    asset top_share                         = asset(0, HST_SYMBOL);

    void reset() {
        day_spending.amount = 0;
        sunshine_share.amount = 0;
        top_share.amount = 0;
    }
};

struct spend_index_t {
    uint64_t shop_id                        = 0;
    uint64_t spending_id                    = 0;
    asset spending                          = asset(0, HST_SYMBOL);

    spend_index_t() {}
    spend_index_t(const uint64_t& shid, const uint64_t& spid, const asset& sp): shop_id(shid), spending_id(spid), spending(sp) {}

    checksum256 get_index() {
        return checksum256::make_from_word_sequence<uint64_t>(
            shop_id, 
            std::numeric_limits<uint64_t>::max() - spending.amount, 
            spending_id, 
            0ULL);
    }
    
    void reset() {
        shop_id = 0;
        spending_id = 0;
        spending.amount = 0;
    }

    static checksum256 get_first_index(const uint64_t& shop_id) {
        return checksum256::make_from_word_sequence<uint64_t>(shop_id, 0ULL, 0ULL, 0ULL);
    }

    EOSLIB_SERIALIZE( spend_index_t, (shop_id)(spending_id)(spending) )
};

struct CONTRACT_TBL shop_t {
    uint64_t id;                //shop_id
    uint64_t pid;               //parent id, 0 means top
    uint64_t cc_id;
    string shop_name;
    name owner_account;          //shop owner account
    name referral_account;
    asset received_payment                  = asset{0, HST_SYMBOL};     // clients pay with HST coins
    uint64_t top_reward_count               = 10;   //can be customized by shop admin
    uint64_t top_rewarded_count             = 0;
    spend_index_t last_sunshine_reward_spend_idx; //spending_t::id
    spend_index_t last_top_reward_spend_idx; //spending_t::id
    shop_share_t share;
    shop_share_t share_cache;
    bool share_cache_updated = false;
    time_point_sec created_at;
    time_point_sec updated_at;

    shop_t() {}
    shop_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }
    uint64_t by_pid() const { return pid; }
    uint64_t by_referral() const { return referral_account.value; }
    uint128_t by_cache_update() const { return (uint128_t) (share_cache_updated ? 1 : 0) << 1 | id; }

    typedef eosio::multi_index< "shops"_n, shop_t,
        indexed_by<"parentid"_n, const_mem_fun<shop_t, uint64_t, &shop_t::by_pid>     >,
        indexed_by<"referrer"_n, const_mem_fun<shop_t, uint64_t, &shop_t::by_referral>      >,
        indexed_by<"cacheupdt"_n,const_mem_fun<shop_t, uint128_t,&shop_t::by_cache_update>  >
    > tbl_t;

    EOSLIB_SERIALIZE( shop_t,   (id)(pid)(cc_id)(shop_name)(owner_account)(referral_account)(received_payment)
                                (top_reward_count)(top_rewarded_count)(last_sunshine_reward_spend_idx)(last_top_reward_spend_idx)
                                (share)(share_cache)(share_cache_updated)(created_at)(updated_at) )
};

struct spending_share_t {
    asset day_spending;     //accumulated for day(s), reset after reward
    asset total_spending;   //accumulated continuously, never reset

    EOSLIB_SERIALIZE( spending_share_t, (day_spending)(total_spending) )
};


/// one_shop-one_customer : one record (each day gets replaced/overwritten)

/// one_shop-one_customer : one record (each day gets replaced/overwritten)
struct CONTRACT_TBL spending_t {
    uint64_t id;
    uint64_t shop_id;
    name customer;
    spending_share_t share;
    spending_share_t share_cache;
    bool share_cache_updated = false;
    time_point_sec created_at;
    time_point_sec updated_at;

    spending_t() {}
    spending_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }
 
    
    ///to ensure uniquness
    uint128_t uk_shop_customer() const { return ((uint128_t) shop_id) << 64 | customer.value; }

    ///to get all spendings (cross-customer) of a shop
    uint64_t by_shop() const { return shop_id; }

    ///to get all spendings (cross-shop) of a customer
    uint64_t by_customer() const { return customer.value; }

    ///to derive shop top-N by day (of yesterday)
    checksum256 by_day_spending() const {
        auto days = updated_at.sec_since_epoch() / seconds_per_day;
        return checksum256::make_from_word_sequence<uint64_t>(
            shop_id,
            std::numeric_limits<uint64_t>::max() - days,
            std::numeric_limits<uint64_t>::max() - share_cache.day_spending.amount, 
            id);
    }
    
    checksum256 by_total_spending() const {
        return checksum256::make_from_word_sequence<uint64_t>(
            shop_id, 
            std::numeric_limits<uint64_t>::max() - share_cache.total_spending.amount, 
            id, 
            0ULL); 
    }

    uint128_t by_cache_update() const { return (uint128_t) (share_cache_updated ? 1 : 0) << 64 | id; }

     typedef eosio::multi_index
    < "spends"_n, spending_t,
        indexed_by<"ukshopcust"_n,   const_mem_fun<spending_t, uint128_t,    &spending_t::uk_shop_customer>   >,
        indexed_by<"idxshop"_n,      const_mem_fun<spending_t, uint64_t,     &spending_t::by_shop>            >,
        indexed_by<"idxcust"_n,      const_mem_fun<spending_t, uint64_t,     &spending_t::by_customer>        >,
        indexed_by<"idxdayspend"_n,  const_mem_fun<spending_t, checksum256,  &spending_t::by_day_spending>    >,
        indexed_by<"idxtotspend"_n,  const_mem_fun<spending_t, checksum256,  &spending_t::by_total_spending>  >,
        indexed_by<"cacheupdt"_n,    const_mem_fun<spending_t, uint128_t,    &spending_t::by_cache_update>    >
    > tbl_t;

    EOSLIB_SERIALIZE( spending_t, (id)(shop_id)(customer)(share)(share_cache)(share_cache_updated)(created_at)(updated_at) )
};

struct CONTRACT_TBL certification_t {
    name user;
    time_point_sec created_at;

    certification_t() {}
    certification_t(const name& u): user(u) { created_at = time_point_sec( current_time_point() ); }

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return user.value; }

    typedef eosio::multi_index<"certify"_n, certification_t> tbl_t;

    EOSLIB_SERIALIZE( certification_t, (user)(created_at) )
};

struct CONTRACT_TBL withdraw_t {
    uint64_t id;
    name withdrawer;    //withdrawn by
    name withdrawee;    //withdrawn to
    uint8_t withdraw_type;
    uint64_t withdraw_shopid;
    asset quantity;
    time_point_sec created_at;

    withdraw_t() {}
    withdraw_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }

    /// to get withdraw records by withdrawee
    uint64_t by_withdrawee() const { return withdrawee.value; }
    typedef eosio::multi_index
    < "withdraws"_n, withdraw_t,
        indexed_by<"withdrawee"_n, const_mem_fun<withdraw_t, uint64_t, &withdraw_t::by_withdrawee> >
    > tbl_t;

    EOSLIB_SERIALIZE( withdraw_t,   (id)(withdrawer)(withdrawee)(withdraw_type)(withdraw_shopid)
                                    (quantity)(created_at) )
};

} //end of namespace ayj
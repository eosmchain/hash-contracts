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
}

struct [[eosio::table("global"), eosio::contract("aiyunji.mall")]] global_t {
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


    global_t() {
    }
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;
 #define TABLE_IN_CONTRACT [[wasm::table, wasm::contract("delife")]]

    struct TABLE_IN_CONTRACT credit_ratio_t {
        uint8_t customer;             // 60%
        uint8_t store;                // 10%
        uint8_t store_top;            // 4%
        uint8_t all_customer;         // 10%
        uint8_t platform_top;         // 4%
        uint8_t inviter;              // 5%
        uint8_t agency;               // 2%
        uint8_t citycenter;           // 2%
        uint8_t gas;                  // 3%

        credit_ratio_t(uint8_t c, uint8_t s, uint8_t st, uint8_t ac, uint8_t pt, uint8_t i, uint8_t ag, uint8_t cc, uint8_t g):
                    customer(c), store(s), store_top(st), all_customer(ac), platform_top(pt), inviter(i), agency(ag), citycenter(cc), gas(g) {}
    };

    struct TABLE_IN_CONTRACT mall_config_t {
        credit_ratio_t credit_ratio_conf;

        uint8_t platform_trx_share;         // 30%
        uint8_t dev_trx_share;              // 5%

        regid platform_regid;
        regid devshare_regid;

        uint64_t primary_key()const { return 0ULL; }
        regid scope()const { return DELIFE_SCOPE; }

        mall_config_t() {}

        typedef wasm::table< "mallconf"_n, mall_config_t, uint64_t > table_t;

        EOSLIB_SERIALIZE( mall_config_t, (credit_ratio_conf)(platform_trx_share)(dev_trx_share)(platform_regid)(devshare_regid) )
    };

    struct TABLE_IN_CONTRACT pool_t {
        uint64_t daily_top_share_a;  //10%: platform-wise top 10
        uint64_t daily_top_share_b;  // 4%: platform-wise top 1000

        uint64_t primary_key()const { return 1ULL; }
        uint16_t scope()const { return DELIFE_SCOPE; }

        typedef wasm::table< "pool"_n, pool_t, uint64_t > table_t;
    };

    struct TABLE_IN_CONTRACT store_t {
        uint64_t    id;                 //PK

        uint64_t    daily_top_share;    //  4%: store-wise top 10
        uint64_t    daily_all_share;    // 10%: all store consumers
        uint64_t    created_at;

        store_t(uint64_t i): id(i) {}

        uint64_t primary_key()const { return id; }
        regid scope()const { return DELIFE_SCOPE; }

        typedef wasm::table< "store"_n, store_t, uint64_t > table_t;
    };

    struct TABLE_IN_CONTRACT store_customer_t {
        uint64_t    store_id;
        regid       customer;

        uint64_t    acc_consume_points;

        uint64_t primary_key()const { return customer.value; }
        regid scope()const { return store_id; }

        store_customer_t() {}
        store_customer_t(uint64_t s, regid c): store_id(s), customer(s) {}

        typedef wasm::table< "stcustomer"_n, store_customer_t, uint64_t > table_t;

        EOSLIB_SERIALIZE( store_customer_t,   (store_id)(customer)(acc_consume_points) )
    };

    struct TABLE_IN_CONTRACT customer_t {
        regid       user;                  //PK

        regid       inviter;
        uint64_t    created_at;

        uint64_t primary_key()const { return user.value; }
        uint64_t scope()const { return DELIFE_SCOPE; }

        customer_t(regid u): user(u) {}
        customer_t() {}

        typedef wasm::table< "customer"_n, project_t, uint64_t > table_t;

        EOSLIB_SERIALIZE( customer_t,   (user)(inviter)(created_at) )
    };



/**
 *  vote table
 */
struct CONTRACT_TBL vote_t {
    uint64_t id;        //PK: available_primary_key

    name owner;         //voter
    name candidate;     //target candidiate to vote for
    asset quantity;

    time_point voted_at;
    time_point unvoted_at;
    time_point restarted_at;   //restart age counting every 30-days
    uint64_t election_round;
    uint64_t reward_round;

    uint64_t by_voter() const             { return owner.value;                              }
    uint64_t by_candidate() const         { return candidate.value;                          }
    uint64_t by_voted_at() const          { return uint64_t(voted_at.sec_since_epoch());     }
    uint64_t by_unvoted_at() const        { return uint64_t(unvoted_at.sec_since_epoch());   }
    uint64_t by_restarted_at() const      { return uint64_t(restarted_at.sec_since_epoch()); }
    uint64_t by_election_round() const    { return election_round;                           }
    uint64_t by_reward_round() const      { return reward_round;                             }

    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return 0; }
    typedef eosio::multi_index<"votes"_n, vote_t> index_t;

    vote_t() {}
    // vote_t(const name& code) {
    //     index_t tbl(code, code.value); //scope: o
    //     id = tbl.available_primary_key();
    // }
    vote_t(const uint64_t& pk): id(pk) {}

    EOSLIB_SERIALIZE( vote_t,   (id)(owner)(candidate)(quantity)
                                (voted_at)(unvoted_at)(restarted_at)
                                (election_round)(reward_round) )
};

typedef eosio::multi_index
< "votes"_n, vote_t,
    indexed_by<"voter"_n,        const_mem_fun<vote_t, uint64_t, &vote_t::by_voter>             >,
    indexed_by<"candidate"_n,    const_mem_fun<vote_t, uint64_t, &vote_t::by_candidate>         >,
    indexed_by<"voteda"_n,       const_mem_fun<vote_t, uint64_t, &vote_t::by_voted_at>          >,
    indexed_by<"unvoteda"_n,     const_mem_fun<vote_t, uint64_t, &vote_t::by_unvoted_at>        >,
    indexed_by<"restarted"_n,    const_mem_fun<vote_t, uint64_t, &vote_t::by_restarted_at>      >,
    indexed_by<"electround"_n,   const_mem_fun<vote_t, uint64_t, &vote_t::by_election_round>    >,
    indexed_by<"rewardround"_n,  const_mem_fun<vote_t, uint64_t, &vote_t::by_reward_round>      >
> vote_tbl;


/**
 *  vote table
 */
struct CONTRACT_TBL voteage_t {
    uint64_t vote_id;        //PK
    asset votes;
    uint64_t age;           //days

    voteage_t() {}
    voteage_t(const uint64_t vid): vote_id(vid) {}
    voteage_t(const uint64_t vid, const asset& v, const uint64_t a): vote_id(vid), votes(v), age(a) {}

    asset value() { return votes * age; }

    uint64_t primary_key() const { return vote_id; }
    uint64_t scope() const { return 0; }

    typedef eosio::multi_index<"voteages"_n, voteage_t> index_t;

    EOSLIB_SERIALIZE( voteage_t,  (vote_id)(votes)(age) )
};

/**
 *  Incoming rewards for whole bpvoting cohort
 *
 */
struct CONTRACT_TBL reward_t {
    uint64_t id;
    asset quantity;
    time_point created_at;

    reward_t() {}
    reward_t(name code, uint64_t scope) {
        index_t tbl(code, scope);
        id = tbl.available_primary_key();
    }

    reward_t(const uint64_t& i): id(i) {}

    uint64_t primary_key() const { return id; }
    uint64_t scope() const { return 0; }

    typedef eosio::multi_index<"rewards"_n, reward_t> index_t;

    EOSLIB_SERIALIZE( reward_t,  (id)(quantity)(created_at) )
};

}
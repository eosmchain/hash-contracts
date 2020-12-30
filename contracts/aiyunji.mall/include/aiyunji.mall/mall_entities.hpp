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
static constexpr eosio::name token_account{"eosio.token"_n};

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("MGP"), 4);
static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t share_boost           = 10000;

#define CONTRACT_TBL [[eosio::table, eosio::contract("mgp.bpvoting")]]

struct [[eosio::table("global"), eosio::contract("aiyunji.mall")]] global_t {
    uint64_t donate_coin_expiry_days;   //expiration duration

    global_t() {
        donate_coin_expiry_days    = 30;
    }

    EOSLIB_SERIALIZE( global_t, (donate_coin_expiry_days) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

// charity donation
struct CONTRACT_TBL donate_t {
    uint64_t id;        //PK: available_primary_key

    name doner;
    asset quantity;
    time_point donated_at;
    time_point expired_at;

    donate_t() {}
    donate_t(const uint64_t& pk): id(pk) {}

    uint64_t primary_key() const {  return id; }
    uint64_t scope() const { return 0; }

    uint64_t by_doner() const { return doner.value; }

    typedef eosio::multi_index<"donates"_n, donate_t> index_t;

    EOSLIB_SERIALIZE(donate_t,   (id)(doner)(quantity)(donated_at)(expired_at) )
};

typedef eosio::multi_index < "donates"_n, donate_t,
    indexed_by<"doner"_n, const_mem_fun<donate_t, uint64_t, &donate_t::by_doner> >
> donate_idx_t;


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
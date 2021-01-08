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

static constexpr uint64_t K_ASWAP_SYS_CONF              = 1;
static constexpr uint64_t RATIO_BOOST                   = 10000; // ratio boost 10000
static constexpr uint64_t RATIO_MAX                     = 1 * RATIO_BOOST; // 100%
static constexpr uint32_t LIQUIDITY_ID_MAX              = 9999;

static constexpr uint64_t UNISWAP_MINT_STEP1            = 1;
static constexpr uint64_t UNISWAP_MINT_STEP2            = 2;
static constexpr uint64_t MINIMUM_LIQUIDITY             = 10*10*10;
static constexpr int128_t PRECISION_1                   = 100000000;
static constexpr uint64_t PRECISION                     = 8;
static constexpr bool     UNISWAP_DEBUG                 = true;

#define CONTRACT_TBL struct [[eosio::table, eosio::contract("aiyunji.swap")]]

struct [[eosio::table("global"), eosio::contract("aiyunji.swap")]] global_t {
    name owner;
    name liquidity_bank;
    uint64_t swap_fee_ratio = 0; // ratio, boost 10000, range[0, 10000]
    set<symbol> permitted_base_syms;
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

CONTRACT_TBL market_t{
    symbol_code         market_sym;
    symbol_code         base_sym;    //maket-base
    name                bank0;
    name                bank1;
    asset               reserve0;
    asset               reserve1;
    asset               total_liquidity;
    time_point_sec      last_skimmed_at;
    time_point_sec      last_synced_at;
    time_point_sec      last_updated_at;  //reserved
    uint128_t           price0_cumulative_last   = 0;
    uint128_t           price1_cumulative_last   = 0;
    bool                closed                   = false;

    market_t(){}
    market_t(const symbol_code& market, const symbol_code& base): market_sym(market), base_sym(base) {}
   
    /**
     * Note:    When there's a conflict with existing symbol pair, new market symbol must be renamed 
     *          to avoid such conflict!!!
     */
    uint64_t primary_key() const {  return base_sym.raw() + market_sym.raw() << 32; }
    uint64_t scope() const { return 0; }

    string get_sympair()const { return market_sym.to_string() + "-" + base_sym.to_string(); }
    uint64_t get_lpsym() const { return total_liquidity.symbol.raw(); }

    typedef eosio::multi_index<"markets"_n, market_t> tbl_t;

    EOSLIB_SERIALIZE(market_t,  (market_sym)(base_sym)(bank0)(bank1)(reserve0)(reserve1)(total_liquidity)
                                (last_skimmed_at)(last_synced_at)(last_updated_at)
                                (price0_cumulative_last)(price1_cumulative_last)(closed) )
};

CONTRACT_TBL liquidity_index_t {
    symbol_code              liquidity_symbol;
    symbol_code              market_sym;
    symbol_code              base_sym;    //maket-base

    liquidity_index_t() {}
    liquidity_index_t(const symbol_code& liquidity_sym, const symbol_code& market_sym, const symbol_code& base_sym)
        : liquidity_symbol(liquidity_sym), market_sym(market_sym), base_sym(base_sym) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const {  return liquidity_symbol.raw(); }

    typedef eosio::multi_index<"liquidids"_n, liquidity_index_t> tbl_t;

    EOSLIB_SERIALIZE( liquidity_index_t, (liquidity_symbol)(market_sym)(base_sym) )
};

CONTRACT_TBL mint_action_t {
    name            owner;
    uint64_t        nonce;
    symbol_code     market_sym;
    symbol_code     base_sym;    //maket-base
    name            bank0;
    name            bank1;
    asset           amount0_in;
    asset           amount1_in;

    time_point_sec  created_at;
    bool            closed;

    mint_action_t(){}
    mint_action_t(uint64_t o, uint64_t n): owner(name(o)),nonce(n){}

    uint64_t scope() const { return owner.value; }
    uint128_t primary_key()const { return (uint128_t)owner.value << 64 | (uint128_t)nonce; }

    typedef eosio::multi_index<"mintactions"_n, mint_action_t> tbl_t;

    EOSLIB_SERIALIZE( mint_action_t,    (owner)(nonce)(market_sym)(base_sym)
                                        (bank0)(bank1)(amount0_in)(amount1_in)
                                        (created_at)(closed) )

};


}// namespace wasm

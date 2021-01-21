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

static constexpr name     active_perm           = "active"_n;
static constexpr name     SYS_BANK              = "eosio.token"_n;
static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("MGP"), 4);

static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t share_boost           = 10000;

static constexpr uint64_t K_ASWAP_SYS_CONF      = 1;
static constexpr uint64_t RATIO_BOOST           = 10000; // ratio boost 10000
static constexpr uint64_t RATIO_MAX             = 1 * RATIO_BOOST; // 100%
static constexpr uint32_t LIQUIDITY_ID_MAX      = 9999;

static constexpr uint64_t UNISWAP_MINT_STEP1    = 1;
static constexpr uint64_t UNISWAP_MINT_STEP2    = 2;
static constexpr uint64_t MINIMUM_LIQUIDITY     = 10*10*10;
static constexpr int128_t PRECISION_1           = 100000000;
static constexpr uint64_t PRECISION             = 8;
static constexpr bool     UNISWAP_DEBUG         = true;

const int64_t MAX = eosio::asset::max_amount;
const int64_t INIT_MAX = 1000000000000000;  // 10^15 
const int ADD_LIQUIDITY_FEE = 1;
const int DEFAULT_FEE = 10;

#define CONTRACT_TBL struct [[eosio::table, eosio::contract("aiyunji.swap")]]

struct [[eosio::table("global"), eosio::contract("aiyunji.swap")]] global_t {
    name swap_token_bank    = SYS_BANK;
    name lp_token_bank      = SYS_BANK;
    name fee_receiver;
    name admin;
    uint64_t swap_fee_ratio = 30; // 0.3%, boosted by 10000, range[0, 10000]
    map<string, uint8_t> base_symbols; //base_symb -> base_symb_index
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

CONTRACT_TBL lp_asset_t {
};

CONTRACT_TBL asset_t {
    uint64_t id;
    extended_asset balance;

    uint64_t primary_key()const { return id; }
    uint128_t secondary_key()const { 
        return make128key( balance.contract.value, balance.quantity.symbol.raw() ); 
    }
};
typedef eosio::multi_index 
    < "assets"_n, asset_t, 
        indexed_by<"assetsymb"_n, const_mem_fun<asset_t, uint128_t, &asset_t::secondary_key>> 
    > asset_tbl;

CONTRACT_TBL pair_t {
    asset               market_reserve;
    asset               base_reserve;
    uint8_t             base_symb_index;
    asset               lpt_reserve;    //liquidity provider token
    time_point_sec      created_at;
    time_point_sec      updated_at;
    bool                closed = false;

    pair_t(){}
    pair_t(const symbol_code& market_symb, const symbol_code& base_symb): 
        market_asset_reserve.symbol(market_symb), 
        base_asset_reserve.symbol(base_symb) {}

    pair_t(const symbol_code& market_symb, const symbol_code& base_symb, const symbol_code& lpt_symb): 
        market_asset_reserve.symbol(market_symb), 
        base_asset_reserve.symbol(base_symb),
        lpt_asset_reserve.symbol(lpt_symb) {}
   
    string to_string() { return market_reserve.symbol.to_string() + ":" + base_reserve.symbol.to_string(); }

    uint64_t primary_key() const {  
        auto virtual_symb = symbol( market_reserve.symbol.code(), base_symb_index );
        return virtual_symb.code().raw(); 
    }

    uint64_t scope() const { return 0; }

    // uint64_t get_lpsym() const { return total_liquidity.symbol.raw(); }

    typedef eosio::multi_index<"swappairs"_n, swap_pair_t> tbl_t;

    // EOSLIB_SERIALIZE(market_t,  (market_sym)(base_sym)(bank0)(bank1)(reserve0)(reserve1)(total_liquidity)
    //                             (last_skimmed_at)(last_synced_at)(last_updated_at)
    //                             (price0_cumulative_last)(price1_cumulative_last)(closed) )
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

    // EOSLIB_SERIALIZE( mint_action_t,    (owner)(nonce)(market_sym)(base_sym)
    //                                     (bank0)(bank1)(amount0_in)(amount1_in)
    //                                     (created_at)(closed) )

};


}// namespace wasm

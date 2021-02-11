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
static constexpr uint32_t ratio_boost           = 10000;

static constexpr uint64_t K_ASWAP_SYS_CONF      = 1;
static constexpr uint64_t RATIO_BOOST           = 10000; // ratio boost 10000
static constexpr uint64_t RATIO_MAX             = 1 * RATIO_BOOST; // 100%
static constexpr uint32_t LIQUIDITY_ID_MAX      = 9999;

static constexpr uint64_t UNISWAP_MINT_STEP1    = 1;
static constexpr uint64_t UNISWAP_MINT_STEP2    = 2;
static constexpr uint64_t MINIMUM_LIQUIDITY     = 10*10*10;
static constexpr bool     UNISWAP_DEBUG         = true;

const int64_t MAX = eosio::asset::max_amount;
const int64_t INIT_MAX = 1000000000000000;  // 10^15 
const int ADD_LIQUIDITY_FEE = 1;
const int DEFAULT_FEE = 10;

#define CONTRACT_TBL struct [[eosio::table, eosio::contract("aiyunji.swap")]]

struct [[eosio::table("global"), eosio::contract("aiyunji.swap")]] global_t {
    name swap_token_bank    = SYS_BANK;
    name lp_token_bank      = SYS_BANK;
    name swap_fee_receiver;
    name admin;
    uint64_t swap_fee_ratio = 30; // 0.3%, boosted by 10000, range[0, 10000]
    map<string, uint8_t> base_symbols; //base_symb -> base_symb_index
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

// static uint128_t make128key(uint64_t a, uint64_t b);
// static checksum256 make256key(uint64_t a, uint64_t b, uint64_t c, uint64_t d);

static uint128_t make128key(uint64_t a, uint64_t b) {
    uint128_t aa = a;
    uint128_t bb = b;
    return (aa << 64) + bb;
}

static checksum256 make256key(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (make128key(a,b) < make128key(c,d))
      return checksum256::make_from_word_sequence<uint64_t>(a,b,c,d);
    else
      return checksum256::make_from_word_sequence<uint64_t>(c,d,a,b);
}

CONTRACT_TBL account {
    asset    balance;
    uint64_t primary_key()const { return balance.symbol.code().raw(); }

    typedef eosio::multi_index< "accounts"_n, account > tbl_t;
};

CONTRACT_TBL evodexaccount {
    extended_asset   balance;
    uint64_t id;
    uint64_t primary_key()const { return id; }
    uint128_t secondary_key()const { return 
        make128key(balance.contract.value, balance.quantity.symbol.raw() ); }

    typedef eosio::multi_index
        < "evodexacnts"_n, evodexaccount, 
            indexed_by<"extended"_n, const_mem_fun<evodexaccount, uint128_t, &evodexaccount::secondary_key>> 
        > tbl_t;
};

CONTRACT_TBL currency_stats {
    asset    supply;
    asset    max_supply;
    name     issuer;
    extended_asset    pool1;
    extended_asset    pool2;
    int fee;
    name fee_contract;
    uint64_t primary_key()const { return supply.symbol.code().raw(); }

    typedef eosio::multi_index< "stat"_n, currency_stats > tbl_t;
};

CONTRACT_TBL index_struct{
    symbol evo_symbol;
    checksum256 id_256;
    uint64_t primary_key()const { return evo_symbol.code().raw(); }
    checksum256 secondary_key()const { return id_256; }

    typedef eosio::multi_index< "evoindex"_n, index_struct, 
            indexed_by<"extended"_n, const_mem_fun<index_struct, checksum256, &index_struct::secondary_key>> 
        > tbl_t;
};


}// namespace wasm

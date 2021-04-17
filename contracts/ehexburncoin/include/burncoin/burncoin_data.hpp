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

namespace ehex {

using namespace eosio;

static constexpr eosio::name EHEX_BANK{"ehex.token"_n};
static constexpr symbol EHEX_SYMBOL = symbol(symbol_code("EHEX"), 8);
static constexpr uint64_t EHEX_MAX_SUPPLY = 10000000000;
static constexpr uint64_t EHEX_PRECISION  = 100000000;

#define CONTRACT_TBL [[eosio::table, eosio::contract("ehexburncoin")]]

struct asset_info_t {
    asset circulated;
    asset total_burned;
    asset last_burned;
    time_point last_burned_at;
};

struct [[eosio::table("global"), eosio::contract("ehex_burncoin")]] global_tbl {
    std::map<std::string, asset_info_t> asset_stats; // symbol string -> asset_info_t
   
    global_tbl(){}
};
typedef eosio::singleton< "global"_n, global_tbl > global_singleton;

}
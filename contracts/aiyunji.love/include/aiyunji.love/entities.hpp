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


using namespace eosio;
using namespace std;

static constexpr eosio::name active_perm{"active"_n};
static constexpr eosio::name SYS_BANK{"eosio.token"_n};

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("MGP"), 4);
static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t share_boost           = 10000;

#define CONTRACT_TBL [[eosio::table, eosio::contract("aiyunji.love")]]

struct [[eosio::table("global"), eosio::contract("aiyunji.love")]] global_t {
    name bank;  //always grow, round by round
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct CONTRACT_TBL project_t {
    uint64_t        id;                 //PK

    name            owner;
    string          proj_code;          //max 15 chars
    asset           total_donated   = asset(0, SYS_SYMBOL);
    asset           total_withdrawn = asset(0, SYS_SYMBOL);
    time_point_sec  created_at;

    uint64_t primary_key()const { return id; }
    uint64_t scope()const { return 0; }

    project_t() {}
    project_t(const uint64_t& i): id(i) {}
    project_t(const name& code, const uint64_t& scope) {
        tbl_t tbl(code, scope);
        id = tbl.available_primary_key();
    }
    
    checksum256 by_proj_code()const { 
        return sha256(const_cast<char*>(proj_code.c_str()), proj_code.size());
    }

    typedef eosio::multi_index
        < "projects"_n, project_t,
            indexed_by<"projcode"_n, const_mem_fun<project_t, checksum256, &project_t::by_proj_code> >
        > tbl_t;

    EOSLIB_SERIALIZE( project_t,    (id)(owner)(proj_code)
                                    (total_donated)(total_withdrawn)
                                    (created_at) )
};

struct CONTRACT_TBL donation_t {
    uint64_t            id;     //PK

    name                donor;
    uint64_t            proj_id;
    asset               quantity;
    string              pay_sn;     //15
    time_point_sec      donated_at;

    uint64_t primary_key() const { return id; }
    uint64_t       scope() const { return 0; }

    donation_t() {}
    donation_t(const uint64_t& i): id(i) {}
    donation_t(const name& code, const uint64_t& scope) {
        tbl_t tbl(code, scope);
        id = tbl.available_primary_key();
    }

    typedef eosio::multi_index<"donations"_n, donation_t> tbl_t;

    EOSLIB_SERIALIZE( donation_t, (id)(donor)(proj_id)(quantity)(pay_sn)(donated_at) )
};


struct CONTRACT_TBL acception_t {
    uint64_t            id;     //PK

    name                donee;
    uint64_t            proj_id;
    string              pay_sn;
    asset               received;
    time_point_sec      received_at;


    uint64_t primary_key() const { return id; }
    uint64_t       scope() const { return 0; }

    acception_t() {}
    acception_t(uint64_t i): id(i) {}
    acception_t(const name& code, const uint64_t& scope) {
        tbl_t tbl(code, scope);
        id = tbl.available_primary_key();
    }

    typedef eosio::multi_index<"acceptions"_n, acception_t> tbl_t;

    EOSLIB_SERIALIZE( acception_t, (id)(donee)(proj_id)(pay_sn)(received)(received_at) )
};
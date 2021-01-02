#pragma once

#include <eosio.token/eosio.token.hpp>

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "entities.hpp"

using namespace wasm::db;

namespace ayj {

using eosio::asset;
using eosio::check;
using eosio::datastream;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unsigned_int;

using std::string;


class [[eosio::contract("aiyunji.love")]] ayj_love: public eosio::contract {
  private:
    global_singleton    _global;
    global_t            _gstate;
    dbc                 _dbc;

  public:
    using contract::contract;

    ayj_love(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(get_self(), get_self().value), _dbc(get_self())
    {
      _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~ayj_love() {
      _global.set( _gstate, get_self() );
    }

    [[eosio::action]]
    void init();  //only code maintainer can init

    [[eosio::action]]
    void addproj(const name& issuer, const string& proj_code);

    [[eosio::action]]
    void delproj(const name& issuer, const uint64_t& proj_id);

    [[eosio::action]] 
    void donate(const name& issuer, const name& donor, const uint64_t& proj_id, const asset& quantity, const string& pay_sn, const string& memo);
    
    [[eosio::action]] 
    void accept(const name& issuer, const name& to, const uint64_t& proj_id, const asset& quantity, const string& pay_sn, const string& memo);

    using init_action     = action_wrapper<name("init"),      &ayj_love::init       >;
    using donate_action   = action_wrapper<name("donate"),    &ayj_love::donate     >;
    using accept_action   = action_wrapper<name("accept"),    &ayj_love::accept     >;

};

} //ayj namespace
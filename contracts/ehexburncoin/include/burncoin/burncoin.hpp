#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "burncoin_data.hpp"

namespace ehex {

using eosio::asset;
using eosio::check;
using eosio::datastream;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unsigned_int;

using std::string;
using namespace wasm::db;

class [[eosio::contract("ehexburncoin")]] ehex_burncoin: public eosio::contract {
  private:
    global_singleton    _global;
    global_tbl          _gstate;
    dbc                 _dbc;

  public:
    using contract::contract;

    ehex_burncoin(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(get_self(), get_self().value), _dbc(get_self())
    {
        _gstate = _global.exists() ? _global.get() : global_tbl{};
    }

    ~ehex_burncoin() 
    {
        _global.set( _gstate, get_self() );
    }

    [[eosio::action]]
    void init();  //only code maintainer can init

    [[eosio::on_notify("*::transfer")]]
    void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);

};

}
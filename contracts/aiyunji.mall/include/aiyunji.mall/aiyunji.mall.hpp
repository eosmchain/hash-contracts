#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include "mall_entities.hpp"
#include "wasm_db.hpp"

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace ayj {

   using std::string;
   using namespace wasm::db;
   using namespace std;
   using namespace eosio;

   /**
    * eosio.token contract defines the structures and actions that allow users to create, issue, and manage
    * tokens on eosio based blockchains.
    */
   class [[eosio::contract("aiyunji.mall")]] ayj_mall : public contract {
      private:
         global_singleton    _global;
         global_t            _gstate;
         dbc                 _dbc;

      public:
         using contract::contract;

         ayj_mall(eosio::name receiver, eosio::name code, datastream<const char*> ds):
         contract(receiver, code, ds), _global(get_self(), get_self().value), _dbc(get_self())
         {
            _gstate = _global.exists() ? _global.get() : global_t{};
         }

         ~ayj_mall() {
            _global.set( _gstate, get_self() );
         }

         [[eosio::action]]
         void init();  //only code maintainer can init

         [[eosio::on_notify("*::transfer")]]
         void deposit(name from, name to, asset quantity, string memo);

         // using create_action = eosio::action_wrapper<"create"_n, &token::create>;
     

   
   };

}

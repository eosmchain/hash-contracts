#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include "mall_states.hpp"
#include "safe.hpp"
#include "utils.hpp"
#include "wasm_db.hpp"

#include <string>

// namespace eosiosystem {
//    class system_contract;
// }

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
         config_singleton    _config;
         config_t            _cstate;

         global_singleton    _global;
         global_t            _gstate;

         dbc                 _dbc;

      public:
         using contract::contract;

         ayj_mall(eosio::name receiver, eosio::name code, datastream<const char*> ds):
            contract(receiver, code, ds), _config(_self, _self.value), _global(_self, _self.value), 
            _dbc(_self) {
            _cstate = _config.exists() ? _config.get() : config_t{};
            _gstate = _global.exists() ? _global.get() : global_t{};
         }

         ~ayj_mall() {
            _config.set( _cstate, _self );
            _global.set( _gstate, _self );
         }

         [[eosio::action]]
         void init();  //only code maintainer can init

         [[eosio::on_notify("*::transfer")]]
         void deposit(const name& from, const name& to, const asset& quantity, const string& memo);

         using transfer_action = action_wrapper<name("transfer"),  &ayj_mall::deposit  >;

         // using create_action = eosio::action_wrapper<"create"_n, &token::create>;


         inline static uint64_t parse_uint64(string_view str) {
            safe<uint64_t> ret;
            to_int(str, ret);
            return ret.value;
         }

   };

}
//mall related

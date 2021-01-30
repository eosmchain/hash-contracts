#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>

#include "mall_states.hpp"
#include "wasm_db.hpp"

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

         global2_singleton   _global2;
         global2_t           _gstate2;

         dbc                 _dbc;

      public:
         using contract::contract;

         ayj_mall(eosio::name receiver, eosio::name code, datastream<const char*> ds):
            contract(receiver, code, ds), _config(_self, _self.value), 
            _global(_self, _self.value), _global2(_self, _self.value), 
            _dbc(_self) {
            _cstate = _config.exists() ? _config.get() : config_t{};
            _gstate = _global.exists() ? _global.get() : global_t{};
            _gstate2 = _global2.exists() ? _global2.get() : global2_t{};
         }

         ~ayj_mall() {
            _config.set( _cstate, _self );
            _global.set( _gstate, _self );
            _global2.set( _gstate2, _self );
         }

         [[eosio::action]]
         void init();  //only code maintainer can init

         [[eosio::on_notify("*::transfer")]]
         void deposit(const name& from, const name& to, const asset& quantity, const string& memo);

         [[eosio::action]]
         void certifyuser(const name& issuer, const name& user);

         [[eosio::action]]
         void execute(); //anyone can invoke, but usually by the platform

         [[eosio::action]]
         void withdraw();

         using transfer_action = action_wrapper<"transfer"_n,  &ayj_mall::deposit  >;
         using execute_action  = action_wrapper<"execute"_n,   &ayj_mall::execute  >;
         using withdraw_action = action_wrapper<"withdraw"_n,  &ayj_mall::withdraw >;

      private:
         bool reward_shops();
         bool reward_shop(const uint64_t& shop_id);
         bool reward_certified();
         bool reward_platform_top();
   };

}
//mall related

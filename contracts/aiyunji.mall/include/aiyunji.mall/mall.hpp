#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include "mall_states.hpp"
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


//mall related


CONTRACT delife : public contract
{
    using contract::contract;

    public:
        ACTION init();
        ACTION config(string conf_key, uint64_t conf_val);
        ACTION add_admin(const name issuer, regid admin, bool is_supper_admin);
        ACTION del_admin(regid issuer, regid admin);
        ACTION add_project(regid issuer, string proj_code);
        ACTION del_project(regid issuer, uint64_t proj_id);


        //mall ACTIONS
        ACTION credit(regid issuer, regid consumer, uint64_t consume_points);
        ACTION decredit(regid issuer, regid consumer, uint64_t consume_points);
        ACTION withdraw(regid issuer, regid to, asset quantity);
        ACTION invest(regid issuer, regid investor, uint64_t invest_points);
        ACTION emit_reward();

        ACTION showid(uint64_t id);

        void    pre_action();

    private:
        uint64_t    gen_new_id(const wasm::name &counter_key, bool persist = true);
        uint64_t    inc_admin_counter(bool increase_by_one = true);
        void        check_admin_auth(regid issuer, bool need_super_admin = false);
        void        set_config(string conf_key, uint64_t conf_val);
        uint64_t    get_config(string conf_key);


}; //contract delife
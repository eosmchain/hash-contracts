#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>

#include "mall_states.hpp"
#include "wasm_db.hpp"

namespace hst {

   using std::string;
   using namespace wasm::db;
   using namespace std;
   using namespace eosio;

   /**
    * eosio.token contract defines the structures and actions that allow users to create, issue, and manage
    * tokens on eosio based blockchains.
    */
   class [[eosio::contract("hst.mall")]] hst_mall : public contract {
      private:
         dbc                 _dbc;

         config_singleton    _config;
         config_t            _cstate;

         global_singleton    _global;
         global_t            _gstate;

         global2_singleton   _global2;
         global2_t           _gstate2;

      public:
         using contract::contract;

         hst_mall(eosio::name receiver, eosio::name code, datastream<const char*> ds):
            contract(receiver, code, ds), _dbc(_self), _config(_self, _self.value), 
            _global(_self, _self.value), _global2(_self, _self.value) 
         {
            _cstate = _config.exists() ? _config.get() : config_t{};
            _gstate = _global.exists() ? _global.get() : global_t{};
            _gstate2 = _global2.exists() ? _global2.get() : global2_t{};
         }

         ~hst_mall() 
         {
            _config.set( _cstate, _self );
            _global.set( _gstate, _self );
            _global2.set( _gstate2, _self );
         }

         [[eosio::action]]
         void init();  //only code maintainer can init

         [[eosio::action]]
         void setownershop(const name& owner, const uint64_t& shop_id);

         [[eosio::on_notify("*::transfer")]]
         void ontransfer(const name& from, const name& to, const asset& quantity, const string& memo);

         [[eosio::action]]
         void registeruser(const name& issuer, const name& user, const name& referrer);
         
         [[eosio::action]]
         void registershop(const name& issuer, const name& owner_account, const string& shop_name, const uint64_t& cc_id, const uint64_t& parent_shop_id, const uint64_t& shop_id);

         [[eosio::action]]
         void registercc(const name& issuer, const uint64_t cc_id, const string& cc_name, const name& admin);
   
         [[eosio::action]]
         void certifyuser(const name& issuer, const name& user);

         [[eosio::action]]
         void rewardshops(const uint64_t& shop_id); //anyone can invoke, but usually by the platform
         [[eosio::action]]
         void rewardptops(); //anyone can invoke, but usually by the platform
         [[eosio::action]]
         void rewardcerts(); //anyone can invoke, but usually by the platform

         [[eosio::action]] // user or admin to withdraw, type 0: spending, 1: customer referral, 2: shop referral
         void withdraw(const name& issuer, const name& to, const uint8_t& withdraw_type, const uint64_t& shop_id);

         [[eosio::action]]
         void getnamevalue(const name& account) {
            check(false, account.to_string() + ": " + to_string(account.value) );
         }

         // [[eosio::action]] // forced withdrawal to users in mining w/o spending for 35+ days
         // void withdrawx(const name& issuer, const name& to, const uint8_t& withdraw_type);

      private:
         inline void _init();
         inline void _check_rewarded(const time_point_sec& last_rewarded_at);
         inline bool _reward_shop(const uint64_t& shop_id);
         inline bool _is_today(const time_point_sec& time);
         inline asset _withdraw_shop(const uint64_t& shop_id, user_t& user, spending_t::tbl_t& spends, bool del_spend);
         inline asset _withdraw_shops(user_t& user);
         inline asset _withdraw_customer_referral(user_t& user);
         inline asset _withdraw_shop_referral(user_t& user);

         inline void _log_reward(const name& account, const reward_type_t &reward_type, const asset& reward_quant, const time_point_sec& reward_time);

         inline void credit_user(const asset& total, user_t& user, const shop_t& shop, const time_point_sec& now);
         inline void credit_shop(const asset& total, shop_t& shop, const time_point_sec& now);
         inline void credit_certified(const asset& total);
         inline void credit_platform_top(const asset& total);
         inline void credit_referrer(const asset& total, const user_t& user, const shop_t& shop, const time_point_sec& now);
         inline void credit_citycenter(const asset& total, const uint64_t& cc_id);
         inline void credit_ramusage(const asset& total);
         
         inline void update_share_cache();
         inline void update_share_cache(user_t& user);
         inline void update_share_cache(shop_t& shop);
         inline void update_share_cache(spending_t& spend);
         
   };

}
//mall related

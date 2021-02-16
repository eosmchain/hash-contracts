#include <aiyunji.mall/mall.hpp>
#include <aiyunji.mall/utils.hpp>
#include <aiyunji.mall/math.hpp>
#include <eosio.token/eosio.token.hpp>

namespace ayj {

inline void ayj_mall::credit_user(const asset& total_share, const name& from) {
	//user:		45%
	user_t user(from);
	CHECK( !_dbc.get( user ), "user not registered: " + from.to_string() );

	auto share0 = total_share * _cstate.allocation_ratios[0] / ratio_boost;
	user.spending_reward += share0;

	_dbc.set( user );
}

inline void ayj_mall::credit_shop(const asset& total_share, shop_t& shop) {
	auto share1 = total_share * _cstate.allocation_ratios[1] / ratio_boost; 
	auto share2 = total_share * _cstate.allocation_ratios[2] / ratio_boost;
	shop.shop_sunshine_share += share1; //shop-all:		15%
	shop.shop_top_share += share2;		//shop-top:		5%
	shop.updated_at = time_point_sec(current_time_point());

	_dbc.set( shop );
}

inline void ayj_mall::credit_certified(const asset& total_share) {
	auto share3 = total_share * _cstate.allocation_ratios[3] / ratio_boost; 
	_gstate.certified_user_share += share3; //certified:	8%
}

inline void ayj_mall::credit_platform_top(const asset& total_share) {
	auto share4 = total_share * _cstate.allocation_ratios[4] / ratio_boost; 
	_gstate.platform_top_share += share4; 	//platform-top: 5%
}

inline void ayj_mall::credit_citycenter(const asset& total_share, const name& citycenter) {
	citycenter_t cc(citycenter);
	CHECK( _dbc.get(cc), "Err: citycenter not found: " + citycenter.to_string() )

	auto share6 = total_share * _cstate.allocation_ratios[6] / ratio_boost; 
	cc.share += share6; //citycenter:	3%

	_dbc.set( cc );
}

inline void ayj_mall::credit_referrer(const asset& total_share) {

	//TODO: referral/agent
	//direct-referral:	20%
	//agent:		5%
}

inline void ayj_mall::credit_ramusage(const asset& total_share) {
	auto share7 = total_share * _cstate.allocation_ratios[7] / ratio_boost; 
	_gstate.ram_usage_share += share7; //ram-usage:	4%
}

void ayj_mall::log_day_spending(const asset& quant, const name& customer, const uint64_t& shop_id) {
	auto spent_at = current_time_point();

	day_spending_t::tbl_t day_spends(_self, shop_id);
	auto idx_ds = day_spends.get_index<"daycustomer"_n>();
	auto key_ds = (uint128_t (spent_at.sec_since_epoch() % seconds_per_day) << 64) | customer.value; 
	auto itr_ds = idx_ds.lower_bound( key_ds );
	if (itr_ds == idx_ds.end()) {
		day_spends.emplace(_self, [&](auto& row) {
			row.id 			= day_spends.available_primary_key();
			row.shop_id 	= shop_id;
			row.customer 	= customer;
			row.spending 	= quant;
			row.spent_at 	= time_point_sec( current_time_point() );
		});
	} else {
		day_spends.modify(*itr_ds, _self, [&](auto& row) {
			row.spending 	+= quant;
			row.spent_at 	= time_point_sec( current_time_point() );
		});
	}
}

/**
 * 	@from: 		only admin user allowed
 *  @to: 		mall contract or self
 *  @quantity:	amount issued and transferred
 *  @memo: 		"<user_id>:<shop_id>"
 * 		
 */
void ayj_mall::ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {
	if (to != _self) return;

	require_auth(from);
	CHECK( from == _cstate.platform_admin, "non-admin user not allowed" )
	CHECK( quantity.symbol.is_valid(), "Invalid quantity symbol name" )
	CHECK( quantity.is_valid(), "Invalid quantity" )
	CHECK( quantity.symbol == HST_SYMBOL, "Token Symbol not allowed" )
	CHECK( quantity.amount > 0, "ontransfer quanity must be positive" )

	_gstate.platform_total_share += quantity;
	auto shop_id = parse_uint64(memo);
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )

	log_day_spending(quantity, from, shop_id);
	credit_user(quantity, from);
	credit_shop(quantity, shop);
	credit_certified(quantity);
	credit_platform_top(quantity);
	credit_referrer(quantity);
	credit_citycenter(quantity, shop.citycenter);
	credit_ramusage(quantity);
    
}

/**
 *  @issuer: platform admin who helps to register
 *  @user: the new user to register
 *  @referrer: one who referrs the user onto this platform, if "0" it means empty referrer
 */
ACTION ayj_mall::registeruser(const name& issuer, const name& user, const name& referrer) {
	CHECK( issuer == _cstate.platform_admin, "" )
	require_auth( issuer );

	user_t new_user(user);
	CHECK( !_dbc.get(new_user), "user not found: " + user.to_string() )

	new_user.referral_account = referrer;
	new_user.created_at = time_point_sec( current_time_point() );

	_dbc.set(new_user);

}
         
ACTION ayj_mall::registershop(const name& issuer, const name& referrer, const name& citycenter, const uint64_t& parent_shop_id, const name& owner_account) {
	CHECK( issuer == _cstate.platform_admin, "non-admin err" )
	require_auth( issuer );

	shop_t::tbl_t shops(_self, _self.value);
	shops.emplace(_self, [&](auto& row) {
		row.id 					= shops.available_primary_key();
		row.citycenter			= citycenter;
		row.parent_id 			= parent_shop_id;
		row.owner_account 		= owner_account;
		row.referral_account	= referrer;
	});

}

ACTION ayj_mall::registercc(const name& issuer, const name& cc_name, const name& cc_account) {
	CHECK( issuer == _cstate.platform_admin, "non-admin err" )
	require_auth( issuer );

	citycenter_t::tbl_t ccs(_self, _self.value);
	auto idx = ccs.get_index<"ccname"_n>();
	auto itr = idx.lower_bound(cc_name.value);

	CHECK( itr != idx.end(), "citycenter name already registered: " + cc_name.to_string() )

	ccs.emplace(_self, [&](auto& row)  {
		row.id 					= ccs.available_primary_key();
		row.citycenter_name 	= cc_name;
		row.citycenter_account 	= cc_account;
		row.created_at 			= time_point_sec( current_time_point() );
	});
}

ACTION ayj_mall::certifyuser(const name& issuer, const name& user) {
	require_auth( issuer );
	CHECK( issuer == _cstate.platform_admin, "issuer not platform admin: " )
	CHECK( is_account(user), "user account not valid" )

	certification_t certuser(user);
	CHECK( !_dbc.get( certuser ), "user already certified" )
	certuser.user = user;
	certuser.created_at = time_point_sec( current_time_point() );
	_dbc.set(certuser);
}

ACTION ayj_mall::init() {
	//init mall symbol
	_cstate.mall_bank = "aiyunji.coin"_n;
}

ACTION ayj_mall::execute() {
	if (!reward_shops()) return;

	if (!reward_certified()) return;

	reward_platform_top();
}

ACTION ayj_mall::withdraw(const name& issuer, const name& to, const uint8_t& withdraw_type, const uint64_t& shop_id) {
	require_auth( issuer );
	CHECK( withdraw_type < 3, "withdraw_type not valid: " + to_string(withdraw_type) )
	CHECK( is_account(to), "to account not valid: " + to.to_string() )

	user_t user(to);
	CHECK( _dbc.get(user), "to user not found: " + to.to_string() )
	CHECK( issuer == _cstate.platform_admin || issuer == to, "withdraw other's reward not allowed" )

	auto now = current_time_point();
	switch( withdraw_type ) {
	case 0: // spending share removal 
	{
		
	}
	break;
	case 1: //customer referral reward withdrawl
	{
		TRANSFER( _cstate.mall_bank, to, user.customer_referral_reward, "cref reward" )
		user.customer_referral_reward.amount = 0;
		_dbc.set( user );
	}
	break;
	case 2: //shop referral reward withdrawal
	{
		TRANSFER( _cstate.mall_bank, to, user.shop_referral_reward, "sref reward" )
		user.shop_referral_reward.amount = 0;
		_dbc.set( user );
	}
	break;
	}
}

bool ayj_mall::reward_shop(const uint64_t& shop_id) {

	return true;
}

bool ayj_mall::reward_shops() {
	if (_gstate2.last_shops_rewarded_at.sec_since_epoch() % seconds_per_day == current_time_point().sec_since_epoch() % seconds_per_day )
		return true; //shops already rewarded

	shop_t::tbl_t shops(_self, _self.value);
	auto itr = shops.find(_gstate2.last_reward_shop_id);
	CHECK( itr++ != shops.end(), "shop not found" + to_string(itr->id) )

	uint8_t step = 0;
	for (; itr != shops.end() && step < MAX_STEP; itr++, step++) {
		if (!reward_shop(itr->id)) return false;

		_gstate2.last_reward_shop_id = itr->id;			
	}

	if (itr == shops.end()) {
		_gstate2.last_reward_shop_id = 0;
		_gstate2.last_shops_rewarded_at = time_point_sec( current_time_point() );
		return true;
	}

	return false;
}

bool ayj_mall::reward_certified() {
	CHECK( _gstate2.last_certification_rewarded_at.sec_since_epoch() % seconds_per_day != current_time_point().sec_since_epoch() % seconds_per_day, "certified users already rewarded" )

	bool finished = false;
	auto quant = _gstate.certified_user_share / _gstate.certified_user_count;
	certification_t::tbl_t new_users(_self, _self.value);
	uint8_t step = 0;
	auto itr = new_users.begin();
	for (; itr != new_users.end() && step++ < MAX_STEP;) {
		TRANSFER( _cstate.mall_bank, itr->user, quant, "cert reward" )

		itr = new_users.erase(itr);
	}

	if (itr == new_users.end()) {
		_gstate.certified_user_count = 0;
		_gstate2.last_certification_rewarded_at = time_point_sec( current_time_point() );
		finished = true;
	}

	return finished;
}

bool ayj_mall::reward_platform_top() {
	CHECK( _gstate2.last_platform_reward_finished_at.sec_since_epoch() % seconds_per_day 
			!= current_time_point().sec_since_epoch() % seconds_per_day, "platform top users already rewarded" )

	auto finished = false;
	

	return true;
}

} /// namespace ayj

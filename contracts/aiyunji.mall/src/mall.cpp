#include <aiyunji.mall/mall.hpp>
#include <aiyunji.mall/utils.hpp>
#include <aiyunji.mall/math.hpp>
#include <eosio.token/eosio.token.hpp>

namespace ayj {

void ayj_mall::init() {

}

/**
 * 	@from: 		admin user
 *  @to: 		mall contract or self
 *  @quantity:	amount issued and transferred
 *  @memo: 		"<shop_id>"
 * 		
 */
void ayj_mall::deposit(const name& from, const name& to, const asset& quantity, const string& memo) {
	if (to != _self) return;

	CHECK( quantity.symbol.is_valid(), "Invalid quantity symbol name" )
	CHECK( quantity.is_valid(), "Invalid quantity" )
	CHECK( quantity.symbol == _cstate.mall_symb, "Token Symbol not allowed" )
	CHECK( quantity.amount > 0, "deposit quanity must be positive" )

	// vector<string_view> params = split(memo, ":");
	// CHECK( params.size() == 2, "memo must be of <user_id>:<shop_id>" )
	// auto user_id = parse_uint64(params[0]);
	auto shop_id = parse_uint64(memo);
	auto N = quantity;

	//user:		45%
	auto share0 = N * _cstate.allocation_ratios[0] / share_boost;
	user_t user(from);
	_dbc.get( user );
	user.share += share0;
	_dbc.set( user );
	
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )

	auto share1 = N * _cstate.allocation_ratios[1] / share_boost; 
	auto share2 = N * _cstate.allocation_ratios[2] / share_boost;
	shop.shop_sunshine_share += share1; //shop-all:		15%
	shop.shop_top_share += share2;	//shop-top:		5%
	shop.updated_at = time_point_sec(current_time_point());
	_dbc.set( shop );

	//certified:	8%
	auto share3 = N * _cstate.allocation_ratios[3] / share_boost; 
	_gstate.certified_user_share += share3;

	//platform-top: 5%
	auto share4 = N * _cstate.allocation_ratios[4] / share_boost; 
	_gstate.platform_top_share += share4;

	//direct-referral:	20%
	//agent:		5%
	
	//citycenter:	3%
	auto share6 = N * _cstate.allocation_ratios[6] / share_boost; 
	citycenter_t cc(shop.citycenter_id);
	CHECK( _dbc.get(cc), "Err: citycenter not found: " + to_string(shop.citycenter_id) )
	cc.share += share6;
	_dbc.set( cc );

	//ram-usage:	4%
	auto share7 = N * _cstate.allocation_ratios[7] / share_boost; 
	_gstate.ram_usage_share += share7;
    
    
}

ACTION ayj_mall::certifyuser(const name& issuer, const name& user) {
	require_auth( issuer );
	CHECK( isser = _gstate.platform_admin, "issuer not platform admin: " )
	CHECK( is_account(user), "user account not valid" )

	day_certified_t daycert(user);
	CHECK( !_dbc.get(user), "user already certified" )
	_dbc.set(daycert);
}

ACTION ayj_mall::execute() {

	bool finished = reward_shops();
	if (!finished) return;
	
	finished = reward_certified();
	if (!finished) return;

	reward_platform_top();
}

bool ayj_mall::reward_shop(const uint64_t& shop_id) {
	auto finished = false;

	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )
	CHECK( shop.last_sunshine_rewarded_at % seconds_per_day < current_time_point() % seconds_per_day, "shop sunshine reward already executed" )

	auto spend_key = ((uint128_t) shop.id << 64) | _gstate.last_sunshine_reward_spending_id;
	total_spending_t total_spends(_self, _self.value);
	auto spend_idx = total_spends.get_index<"shopspends"_n>();
	auto lower_itr = spend_idx.lower_bound( spend_key );
	uint8_t step = 0;
	while (auto itr = lower_itr; itr != spend_idx.end() && step++ < MAX_STEP; itr++) { /// sunshine reward
		shop.last_sunshine_reward_spending_id = itr->id;
		auto quant = shop.shop_sunshine_share * itr->spending / shop.total_customer_spending;

		user_t user(itr->customer);
		CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )

		TRANSFER( _gstate.mall_bank, user.account, quant, "shop reward" )
	}

	if (itr == spend_idx.end()) { /// top 10 reward
		shop.last_sunshine_rewarded_at = time_point_sec( current_time_point() );

		day_spending_t day_spends(_self, shop.id);
		auto spend_idx = day_spends.get_index<"spends"_n>();
		for (auto itr = spend_idx.lower_itr(), step = 0; itr != spend_idx.end() && step < _cstate.shop_top_count; itr++) {
			user_t user(itr->customer);
			CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )

			auto quant = shop.shop_top_share * itr->spending / shop.total_customer_day_spending;

			TRANSFER( _gstate.mall_bank, user.account, quant, "shop top reward" )
		}
		shop.last_top_rewarded_at = time_point_sec( current_time_point() );
		_dbc.set( shop );

		finished = true;
	}

	return finished;
}

bool ayj_mall::reward_shops() {
	if (_gstate2.last_shop_reward_finished_at % seconds_per_day == time_point_sec( current_time_point() % seconds_per_day ))
		return true; //shops already rewarded

	shop_t shops(_self, _self.value);
	auto itr = shops.find(_gstate2.last_reward_shop_id);
	CHECK( itr++ != shops.end(), "shop not found" + to_string(itr->id) )

	uint8_t step = 0;
	for (; itr != shops.end() && step < SHOP_ITR_MAX; itr++, step++) {
		auto finished = reward_shop(itr->id);
		if (finished) 
			_gstate2.last_reward_shop_id = itr->id;
		else 
			return false;
	}

	if (itr == shops.end()) {
		_gstate2.last_reward_shop_id = 0;
		_gstate2.last_shop_reward_finished_at = time_point_sec( current_time_point() );
		return true;
	}

	return false;
}

bool ayj_mall::reward_certified() {
	if (_gstate2.last_certified_reward_finished_at % seconds_per_day != time_point_sec( current_time_point() % seconds_per_day, "certified users already rewarded" )

	auto finished = false;
	auto quant = _gstate.certified_user_share / _gstate.certified_user_count;
	day_certified_t new_users(_self, _self.value);
	for (auto itr = new_users.begin(); itr != new_users.end() && step++ < _gstate.ITR_MAX){
		TRANSFER( _gstate.mall_bank, itr->account, quant, "cert reward" )

		itr = new_users.erase(itr);
	}

	if (itr == new_users.end()) {
		_gstate2.last_certified_reward_finished_at = time_point_sec( current_time_point() );
		finished = true;
	}

	return finished;
}

bool ayj_mall::reward_platform_top() {
	CHECK( _gstate2.last_platform_reward_finished_at % seconds_per_day != time_point_sec( current_time_point() % seconds_per_day, "platform top users already rewarded" )

	auto finished = false;
	total_spending_t total_spends(_self, _self.value);
	auto spend_idx = total_spends.get_index<"spends"_n>();
	uint8_t step = 0;
	while (auto itr = spend_idx.begin(); itr != spend_idx.end() && step < MAX_STEP; itr++) { /// sunshine reward
		if ( step++ < _gstate2.last_platform_top_reward_step) continue;
		
		_gstate2.last_platform_top_reward_step++;

		user_t user(itr->customer);
		CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )
		
		auto quant = _gstate.platform_top_share * itr->spending / _gstate.platform_total_spending / _cstate.platform_top_count;
		TRANSFER( _gstate.mall_bank, user.account, quant, "shop reward" )
	}

	if (itr == spend_idx.end()) {
		_gstate2.last_platform_reward_finished_at = time_point_sec( current_time_point() );
		finished = true;
	}

	return true;
}

} /// namespace ayj

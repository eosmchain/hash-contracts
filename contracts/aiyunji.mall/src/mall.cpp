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
	auto spent_at = current_time_point();

	_gstate.platform_total_spending += N;
	
	day_spending_t::tbl_t day_spends(_self, shop_id);
	auto idx_ds = day_spends.get_index<"daycustomer"_n>();
	auto key_ds = (uint128_t (spent_at.sec_since_epoch() % seconds_per_day) << 64) | from.value; 
	auto itr_ds = idx_ds.lower_bound( key_ds );
	if (itr_ds == idx_ds.end()) {
		day_spends.emplace(_self, [&](auto& row) {
			row.id 			= day_spends.available_primary_key();
			row.shop_id 	= shop_id;
			row.customer 	= from;
			row.spending 	= N;
			row.spent_at 	= time_point_sec( current_time_point() );
		});
	} else {
		day_spends.modify(*itr_ds, _self, [&](auto& row) {
			row.spending 	+= N;
			row.spent_at 	= time_point_sec( current_time_point() );
		});
	}

	total_spending_t::tbl_t total_spends(_self, _self.value);
	auto idx_ts = total_spends.get_index<"shopcustomer"_n>();
	auto key_ts = (uint128_t (spent_at.sec_since_epoch() % seconds_per_day) << 64) | from.value; 
	auto itr_ts = idx_ts.lower_bound( key_ts );
	if (itr_ts == idx_ts.end()) {
		total_spends.emplace(_self, [&](auto& row)  {
			row.id 			= total_spends.available_primary_key();
			row.shop_id 	= shop_id;
			row.customer 	= from;
			row.spending 	= N;
			row.spent_at 	= time_point_sec( current_time_point() );
		});
	} else {
		total_spends.modify(*itr_ts, _self, [&](auto& row) {
			row.spending 	+= N;
			row.spent_at 	= time_point_sec( current_time_point() );
		});
	}

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
	CHECK( issuer == _cstate.platform_admin, "issuer not platform admin: " )
	CHECK( is_account(user), "user account not valid" )

	day_certified_t daycert(user);
	CHECK( !_dbc.get( daycert ), "user already certified" )
	_dbc.set(daycert);
}

ACTION ayj_mall::execute() {
	if(!reward_shops()) return;
	
	if (!reward_certified()) return;

	reward_platform_top();
}

ACTION ayj_mall::withdraw() {

}

bool ayj_mall::reward_shop(const uint64_t& shop_id) {
	auto finished = false;

	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )
	CHECK( shop.last_sunshine_rewarded_at.sec_since_epoch() % seconds_per_day < current_time_point().sec_since_epoch() % seconds_per_day, "shop sunshine reward already executed" )

	auto spend_key = ((uint128_t) shop.id << 64) | _gstate2.last_sunshine_reward_spending_id;
	total_spending_t::tbl_t total_spends(_self, _self.value);
	auto spend_idx = total_spends.get_index<"shopspends"_n>();
	auto lower_itr = spend_idx.lower_bound( spend_key );
	uint8_t step = 0;
	auto itr = lower_itr;
	for (; itr != spend_idx.end() && step++ < MAX_STEP; itr++) { /// sunshine reward
		shop.last_sunshine_reward_spending_id = itr->id;
		_gstate2.last_sunshine_reward_spending_id = itr->id;
		auto quant = (shop.shop_sunshine_share / shop.total_customer_spending) * itr->spending;

		user_t user(itr->customer);
		CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )

		TRANSFER( _cstate.mall_bank, user.account, quant, "shop reward" )
	}

	if (itr == spend_idx.end()) { /// top 10 reward
		shop.last_sunshine_rewarded_at = time_point_sec( current_time_point() );

		day_spending_t::tbl_t day_spends(_self, shop.id);
		auto spend_idx = day_spends.get_index<"spends"_n>();
		uint8_t step = 0;
		for (auto itr = spend_idx.begin(); itr != spend_idx.end() && step < _cstate.shop_top_count; itr++) {
			user_t user(itr->customer);
			CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )

			auto quant = (shop.shop_top_share / shop.total_customer_day_spending) * itr->spending;

			TRANSFER( _cstate.mall_bank, user.account, quant, "shop top reward" )
		}
		shop.last_top_rewarded_at = time_point_sec( current_time_point() );
		_dbc.set( shop );

		finished = true;
		_gstate2.last_sunshine_reward_spending_id = 0;
	}

	return finished;
}

bool ayj_mall::reward_shops() {
	if (_gstate2.last_shop_reward_finished_at.sec_since_epoch() % seconds_per_day == current_time_point().sec_since_epoch() % seconds_per_day )
		return true; //shops already rewarded

	shop_t::tbl_t shops(_self, _self.value);
	auto itr = shops.find(_gstate2.last_reward_shop_id);
	CHECK( itr++ != shops.end(), "shop not found" + to_string(itr->id) )

	uint8_t step = 0;
	for (; itr != shops.end() && step < _cstate.ITR_MAX; itr++, step++) {
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
	CHECK( _gstate2.last_certified_reward_finished_at.sec_since_epoch() % seconds_per_day != current_time_point().sec_since_epoch() % seconds_per_day, "certified users already rewarded" )

	bool finished = false;
	auto quant = _gstate.certified_user_share / _gstate.certified_user_count;
	day_certified_t::tbl_t new_users(_self, _self.value);
	uint8_t step = 0;
	auto itr = new_users.begin();
	for (; itr != new_users.end() && step++ < _cstate.ITR_MAX;) {
		TRANSFER( _cstate.mall_bank, itr->user, quant, "cert reward" )

		itr = new_users.erase(itr);
	}

	if (itr == new_users.end()) {
		_gstate2.last_certified_reward_finished_at = time_point_sec( current_time_point() );
		finished = true;
	}

	return finished;
}

bool ayj_mall::reward_platform_top() {
	CHECK( _gstate2.last_platform_reward_finished_at.sec_since_epoch() % seconds_per_day 
			!= current_time_point().sec_since_epoch() % seconds_per_day, "platform top users already rewarded" )

	auto finished = false;
	total_spending_t::tbl_t total_spends(_self, _self.value);
	auto spend_idx = total_spends.get_index<"spends"_n>();
	uint8_t step = 0;
	auto itr = spend_idx.begin();
	for (; itr != spend_idx.end() && step < MAX_STEP; itr++) { /// sunshine reward
		if ( step++ < _gstate2.last_platform_top_reward_step) continue;
		
		_gstate2.last_platform_top_reward_step++;

		user_t user(itr->customer);
		CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )
		
		auto quant = (_gstate.platform_top_share / _gstate.platform_total_spending / _cstate.platform_top_count) * itr->spending;
		TRANSFER( _cstate.mall_bank, user.account, quant, "shop reward" )
	}

	if (itr == spend_idx.end()) {
		_gstate2.last_platform_reward_finished_at = time_point_sec( current_time_point() );
		finished = true;
	}

	return true;
}

} /// namespace ayj

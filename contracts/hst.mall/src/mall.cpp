#include <hst.mall/mall.hpp>
#include <hst.mall/utils.hpp>
#include <hst.mall/math.hpp>
#include <eosio.token/eosio.token.hpp>

namespace hst {

inline bool hst_mall::_is_today(const time_point_sec& time) {
	return time.sec_since_epoch() % seconds_per_day == current_time_point().sec_since_epoch() % seconds_per_day;
}

///platform share cache
inline void hst_mall::update_share_cache() {
	if (!_gstate.executing) _gstate.platform_share_cache = _gstate.platform_share;
	_gstate.share_cache_updated = !_gstate.executing;
}
///user share cache
inline void hst_mall::update_share_cache(user_t& user) {
	if (!_gstate.executing) {
		user.share_cache = user.share;
		_gstate2.last_cache_update_useridx = user.by_cache_update();
	}
	user.share_cache_updated 	= !_gstate.executing;
}
///shop share cache
inline void hst_mall::update_share_cache(shop_t& shop) {
	if (!_gstate.executing) {
		shop.share_cache = shop.share;
		_gstate2.last_cache_update_shopidx = shop.by_cache_update();
	}
	shop.share_cache_updated 	= !_gstate.executing;
}
///spend share cache
inline void hst_mall::update_share_cache(spending_t& spend) {
	if (!_gstate.executing) {
		spend.share_cache = spend.share;
		_gstate2.last_cache_update_spendingidx = spend.by_cache_update();
	}
	spend.share_cache_updated 	= !_gstate.executing;
}

//  total, remaining, customer, shop_id, now 
inline void hst_mall::credit_user(const asset& total, user_t& user, const shop_t& shop, const time_point_sec& now) {
	auto share0 						= total * _cstate.allocation_ratios[0] / ratio_boost; 
	_gstate.platform_share.total_share 	+= total;	//TODO: check its usage
	
	spending_t::tbl_t spends(_self, _self.value);
	auto idx = spends.get_index<"shopcustidx"_n>();
	auto key = ((uint128_t) shop.id << 64) | user.account.value; 
	auto lower_itr = idx.lower_bound( key );
	auto upper_itr = idx.upper_bound( key );
	auto itr = lower_itr;
	if (itr == upper_itr || itr == idx.end()) {
		spends.emplace(_self, [&](auto& row) {
			row.id 						= spends.available_primary_key();
			row.shop_id 				= shop.id;
			row.customer 				= user.account;
			row.share.day_spending		= share0; //will be reset upon sending daily reward
			row.share.total_spending 	= share0;
			row.created_at 				= now;

			update_share_cache(row);
		});
	} else { //existing
		spends.modify(*itr, _self, [&](auto& row) {
			row.share.day_spending 		+= share0;
			row.share.total_spending 	+= share0;

			update_share_cache(row);
		});
	}
	
	user.share.spending_share 		+= share0;
	user.updated_at 				= now;
	update_share_cache(user);
	_dbc.set( user );
}

// remaining, shop, now
inline void hst_mall::credit_shop(const asset& total, shop_t& shop, const time_point_sec& now) {
	auto share0 						= total * _cstate.allocation_ratios[0] / ratio_boost; 
	shop.share.total_spending			+= share0;
	shop.share.day_spending				+= share0;

	auto share1							= total * _cstate.allocation_ratios[1] / ratio_boost; 
	auto share2							= total * _cstate.allocation_ratios[2] / ratio_boost; 
	
	shop.share.sunshine_share 			+= share1; //shop-all:	15%
	shop.share.top_share 				+= share2; //shop-top:	5%
	shop.updated_at						= now;
	update_share_cache(shop);
	_dbc.set( shop );
}

inline void hst_mall::credit_certified(const asset& total) {
	auto share3 						= total * _cstate.allocation_ratios[3] / ratio_boost; 
	_gstate.platform_share.certified_user_share += share3; //certified:	8%
	update_share_cache();
}

inline void hst_mall::credit_platform_top(const asset& total) {
	auto share4 						= total * _cstate.allocation_ratios[4] / ratio_boost; 
	_gstate.platform_share.top_share 	+= share4; 	//platform-top: 5%
	update_share_cache();
}

inline void hst_mall::credit_referrer(const asset& total, const user_t& user, const shop_t& shop, const time_point_sec& now) {
	auto share5 = total * _cstate.allocation_ratios[5] / ratio_boost; //direct-referral:	10%
	auto share6 = total * _cstate.allocation_ratios[6] / ratio_boost; //agent-referral:		5%

	if (!is_account(user.referral_account)) {
		_gstate.platform_share.lucky_draw_share += share5 + share6;
		return;
	}

	//credit direct referrer
	auto direct_referrer = user_t(user.referral_account);
	CHECK( _dbc.get(direct_referrer), "Err: direct referrer not registered: " +  user.referral_account.to_string() )
	direct_referrer.share.customer_referral_share += share5;
	direct_referrer.updated_at = now;
	update_share_cache(direct_referrer);
	_dbc.set( direct_referrer );

	if (direct_referrer.owned_shops.empty()) { //none-shop-owner referrer
		_gstate.platform_share.lucky_draw_share += share6;
		return;
	}

	//check if upper referrer of a shop owner exists or not
	if (!is_account(direct_referrer.referral_account)) {
		_gstate.platform_share.lucky_draw_share += share6;
		return;
	}
	//check if upper referrer has been registered or not
	user_t upper_referrer(direct_referrer.referral_account);
	if (!_dbc.get(upper_referrer)) {
		_gstate.platform_share.lucky_draw_share += share6;
		return;	
	}

	//credit upper referrer of a shop owner
	upper_referrer.share.shop_referral_share += share6;
	upper_referrer.updated_at = now;
	update_share_cache( upper_referrer );
	_dbc.set( upper_referrer );

}

inline void hst_mall::credit_citycenter(const asset& total, const uint64_t& cc_id) {
	citycenter_t cc(cc_id);
	CHECK( _dbc.get(cc), "Err: citycenter not found: " + to_string(cc_id) )

	auto share7 						= total * _cstate.allocation_ratios[7] / ratio_boost; 
	auto to = is_account(cc.admin) ? cc.admin : _cstate.platform_fee_collector;
	TRANSFER( _cstate.mall_bank, to, share7, "cc reward" )
}

inline void hst_mall::credit_ramusage(const asset& total) {
	auto share8 						= total * _cstate.allocation_ratios[8] / ratio_boost; 
	_gstate.platform_share.ram_usage_share += share8; //ram-usage:	4%
}

/**
 * 	triggered by transfer event of a token contract
 *
 *  @from: 		admin or user
 *  @to: 		mall contract or self
 *  @quantity:	amount issued and transferred
 *  @memo: 		format-1: "<user_account>@<shop_id>"
 *              format-2: "burn@<external_serial_no>"
 * 		
 */
void hst_mall::ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {
	if (to != _self) return;

	require_auth(from);
	CHECK( quantity.symbol.is_valid(), "Invalid quantity symbol name" )
	CHECK( quantity.is_valid(), "Invalid quantity" )
	CHECK( quantity.symbol == HST_SYMBOL, "Token Symbol not allowed" )
	CHECK( quantity.amount > 0, "ontransfer quanity must be positive" )
	CHECK( get_first_receiver() == _cstate.mall_bank, "must transfer by HST_BANK: " + _cstate.mall_bank.to_string() )

	vector<string_view> params = split(memo, "@");	
	CHECK( params.size() == 2, "memo must be of <burn|user_account>@<shop_id>|<sn>" )

	auto act = std::string(params[0]);
	if (act == "burn") {
		BURN( _cstate.mall_bank, _self, quantity, "burn" )
		return;
	}
	
	auto user_acct 			= name(act);
	CHECK( is_account(user_acct), "user account not valid: " + std::string(params[0]) )
	user_t user(user_acct);
	CHECK( _dbc.get( user ), "user not registered: " + user_acct.to_string() );

	auto shop_id 			= parse_uint64(params[1]);
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )

	auto now				= time_point_sec( current_time_point() );
	auto total				= quantity;
	
	credit_user				( total, user, shop_id, now );
	credit_shop				( total, shop, now );
	credit_platform_top		( total );
	credit_certified		( total );
	credit_referrer			( total, user, shop, now );	//lukcy-draw possibly credited within
	credit_citycenter		( total, shop.cc_id);
	credit_ramusage			( total );
}

/**
 *  @issuer: platform admin who helps to register
 *  @user: the new user to register
 *  @referrer: one who referrs the user onto this platform, if "0" it means empty referrer
 */
ACTION hst_mall::registeruser(const name& issuer, const name& user, const name& referrer) {
	CHECK( issuer == _cstate.platform_admin, "non-platform admin not allowed" )
	require_auth( issuer );

	CHECK( is_account(user), "invalid user: " + user.to_string() )
	CHECK( user != referrer, "cannot refer self! ")

	user_t new_user(user);
	CHECK( !_dbc.get(new_user), "user already registered: " + user.to_string() )

	new_user.referral_account = is_account(referrer) ? referrer : _cstate.platform_referrer;
	new_user.created_at = time_point_sec( current_time_point() );

	_dbc.set(new_user);

}
         
ACTION hst_mall::registershop(const name& issuer, const name& owner_account, const string& shop_name, const uint64_t& cc_id, const uint64_t& parent_shop_id, const uint64_t& shop_id) {
	CHECK( issuer == _cstate.platform_admin, "non-platform-admin err" )
	require_auth( issuer );

	CHECK( shop_name.size() < 128, "shop name too long: " + to_string(shop_name.size()) )

	user_t shop_owner(owner_account);
	CHECK( _dbc.get(shop_owner), "user not registered: " + owner_account.to_string() )
	shop_owner.owned_shops.insert(shop_id);
	_dbc.set(shop_owner);

	auto shop_referrer = shop_owner.referral_account;
	if (!shop_referrer)
		shop_referrer = _cstate.platform_referrer;
	
	shop_t shop(shop_id);
	CHECK( !_dbc.get(shop), "shop already registered: " + to_string(shop_id) );

	if (parent_shop_id != 0) {
		shop_t p_shop(parent_shop_id);
		CHECK( _dbc.get(p_shop), "parnet shop not registered: " + to_string(parent_shop_id) );
	}

	citycenter_t cc(cc_id);
	CHECK( _dbc.get(cc), "Err: citycenter not found: " + to_string(cc_id) )

	shop.pid 				= parent_shop_id;
	shop.shop_name		 	= shop_name;
	shop.cc_id				= cc_id;
	shop.owner_account 		= owner_account;
	shop.referral_account	= shop_referrer;
	shop.created_at			= time_point_sec( current_time_point() );

	_dbc.set( shop );
}

ACTION hst_mall::registercc(const name& issuer, const uint64_t cc_id, const string& cc_name, const name& admin) {
	CHECK( issuer == _cstate.platform_admin, "non-admin err" )
	require_auth( issuer );

	CHECK( cc_name.size() < 128, "citycenter name length too long: " + cc_name )

	auto now = time_point_sec( current_time_point() );
	citycenter_t cc(cc_id);
	if (!_dbc.get(cc)) //must be called prior to setting its fields
		cc.created_at = now;

	cc.cc_name = cc_name;
	cc.admin = admin;

	_dbc.set( cc );
}

ACTION hst_mall::certifyuser(const name& issuer, const name& user) {
	require_auth( issuer );
	CHECK( issuer == _cstate.platform_admin, "issuer (" + issuer.to_string() + ") not platform admin: " + _cstate.platform_admin.to_string() )
	CHECK( is_account(user), "user account not valid" )

	certification_t certuser(user);
	CHECK( !_dbc.get( certuser ), "user already certified" )
	certuser.user = user;
	certuser.created_at = time_point_sec( current_time_point() );
	_dbc.set(certuser);

	_gstate.platform_share.certified_user_count++;
}

void hst_mall::_init() {
	_cstate.mall_bank = "hst.token"_n;
	// _cstate.mall_bank = "aiyunji.coin"_n;
	_cstate.platform_fee_collector = "hst.feeadmin"_n;
	_cstate.platform_admin = "hst.admin"_n;
	_cstate.platform_referrer = "hst.refadmin"_n;
}

ACTION hst_mall::init() {
	require_auth(_self);

	// check(false, "init disabled");

	// auto shops = shop_t::tbl_t(_self, _self.value);
	// for (auto itr = shops.begin(); itr != shops.end(); itr++) {
	// 	auto shop = shop_t(itr->id);
	// 	_dbc.get(shop);
	// 	shop.last_sunshine_reward_spend_idx.reset();
	// 	_dbc.set(shop);
	// }
	
	shop_t shop(1);
	_dbc.get(shop);
	shop.share.total_spending.amount = 1934550;
	_dbc.set(shop);

	// shop.id = 6;
	// _dbc.get(shop);
	// shop.share.total_spending.amount = 7200;
	// _dbc.set(shop);
	
	// shop.id = 17;
	// _dbc.get(shop);
	// shop.share.total_spending.amount = 669600;
	// _dbc.set(shop);

	// shop.id = 5;
	// _dbc.get(shop);
	// shop.share.total_spending.amount = 133650;
	// _dbc.set(shop);

	// shop.id = 41;
	// _dbc.get(shop);
	// shop.share.total_spending.amount = 27000;
	// _dbc.set(shop);

	// shop.id = 43;
	// _dbc.get(shop);
	// shop.share.total_spending.amount = 50400;
	// _dbc.set(shop);

	// list<spending_t> new_spends;

	// auto spends = spending_t::tbl_t(_self, _self.value);
	// for (auto itr = spends.begin(); itr != spends.end();) {
	// 	spending_t spend(itr->id);
	// 	_dbc.get(spend);
	// 	new_spends.push_back(spend);

	// 	itr = spends.erase(itr);
	// }

	// for (auto spend : new_spends) {
	// 	spends.emplace(_self, [&](auto& row) {
	// 		row.id 						= spend.id;
	// 		row.shop_id 				= spend.shop_id;
	// 		row.customer 				= spend.customer;
	// 		row.share					= spend.share;
	// 		row.share_cache				= spend.share_cache;
	// 		row.share_cache_updated		= spend.share_cache_updated;
	// 		row.created_at 				= spend.created_at;
	// 	});
	// }

	// _cstate.allocation_ratios[2] = 800;
	// _cstate.allocation_ratios[3] = 500;
	// _init();
	// _cstate.withdraw_mature_days = 1;

	// spending_t spend(1);
	// CHECK( _dbc.get(spend), "Err: spend not found" )
	// _dbc.del(spend); //remove spending stake from spends share pool

/*
	int max_step = 50;
	int step = 0;
	citycenter_t::tbl_t ccs(_self, _self.value);
	uint64_t cc_id = _cstate.withdraw_mature_days + 110000 - 2;
	auto idx = ccs.upper_bound(cc_id);
	bool found = false;
	for (auto itr = idx.begin(); itr != idx.end() && step < max_step; itr++, step++) {
		ccs.modify( itr, _self, [&]( auto& row ) {
    		row.share = asset(0, HST_SYMBOL);
  		});

		found = true;
		_cstate.withdraw_mature_days += 1;
	}
	check( found, "finished!" );
*/
}

ACTION hst_mall::setownershop(const name& owner, const uint64_t& shop_id) {
	require_auth(_self);

	user_t user(owner);
	CHECK( _dbc.get(user), "shop owner user not registered: " + owner.to_string() )

	user.owned_shops.insert(shop_id);
	_dbc.set( user );
	
}

inline void hst_mall::_check_rewarded(const time_point_sec& last_rewarded_at) {
	CHECK( current_time_point().sec_since_epoch() > last_rewarded_at.sec_since_epoch() + seconds_per_halfday, "too early to reward: < 12 hours" )
	CHECK( !_is_today(last_rewarded_at), "already rewarded: " + to_string(last_rewarded_at.sec_since_epoch())) //check if the particular category is rewarded
}


void hst_mall::_log_reward(const name& account, const reward_type_t &reward_type, const asset& reward_quant, const time_point_sec& reward_time) {
	auto reward = reward_t::tbl_t(_self, _self.value);

	reward.emplace(_self, [&](auto& row) {
		row.id						= reward.available_primary_key();
		row.account 				= account;
		row.reward_type 			= reward_type;
		row.reward_quantity			= reward_quant;
		row.rewarded_at 			= reward_time;
	});
}

/**
 * 阳光普照, 是按照累计排名的
 * 门店top5, 是按照当日消费额前5
 */
bool hst_mall::_reward_shop(const uint64_t& shop_id) {
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )
	if (_is_today(shop.updated_at))
		return true;
	
	if (shop.top_rewarded_count > shop.top_reward_count) 
		shop.top_rewarded_count = 0;

	if (shop.top_rewarded_count == 0) {
		shop.last_sunshine_reward_spend_idx.shop_id = 0;
		shop.share_cache = shop.share;
	}

	// check(false, "shop.top_rewarded_count=" + to_string(shop.top_rewarded_count) + " \n" +
	// 			 "shop.top_reward_count=" + to_string(shop.top_reward_count) );

	_dbc.set( shop );

	spending_t::tbl_t spends(_self, _self.value);
	auto spend_idx = spends.get_index<"shopspends"_n>();
	auto spend_key = (shop.last_sunshine_reward_spend_idx.shop_id == 0) ? 
		spend_index_t::get_first_index(shop_id) : shop.last_sunshine_reward_spend_idx.get_index();

	auto lower_itr = spend_idx.upper_bound( spend_key );
	auto itr = lower_itr;
	if (itr->shop_id != shop_id) //already iterate all spends within the given shop
		return true;

	auto now = time_point_sec( current_time_point() );
	
	auto share_cache = shop.share_cache;
	if (share_cache.total_spending.amount == 0 || share_cache.sunshine_share.amount == 0) 
		return true; //no share for this shop, hence skipping it

	bool processed = false;
	bool completed = false;
	for (uint8_t step = 0; itr != spend_idx.end() && step < MAX_STEP; itr++, step++) {
		if (itr->shop_id != shop_id) { //already iterate all spends within the given shop 
			completed = true;
			break;
		}

		shop.last_sunshine_reward_spend_idx = spend_index_t(itr->shop_id, itr->id, itr->share_cache.day_spending);
		
		auto spending_share_cache = itr->share_cache;
		check(spending_share_cache.total_spending.amount > 0, "Err: zero user total spending");
		check(spending_share_cache.total_spending.amount <= share_cache.total_spending.amount, "Err: individual spending > total spending for: " + to_string(itr->id) );

		auto sunshine_quant = share_cache.sunshine_share * spending_share_cache.total_spending.amount / 
								share_cache.total_spending.amount;

		user_t user(itr->customer);
		CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )

		TRANSFER( _cstate.mall_bank, user.account, sunshine_quant, "sunshine reward by shop-" + to_string(shop_id) )  /// sunshine reward
		_log_reward( user.account, SHOP_SUNSHINE_REWARD, sunshine_quant, now);

		auto quant_avg = share_cache.top_share / shop.top_reward_count; //choose average value, to avoid sum traverse
		if (shop.top_rewarded_count++ < shop.top_reward_count) {
			TRANSFER( _cstate.mall_bank, user.account, quant_avg, "top reward by shop-" +  to_string(shop_id) ) /// shop top reward
			_log_reward( user.account, SHOP_TOP_REWARD, quant_avg, now);
		} else {
			// top_reward_completed = true;
		}

		processed = true;

		_dbc.set( shop );
	}

	if (completed || itr == spend_idx.end()) { //means finished for this shop
		shop.share.day_spending 	-= shop.share_cache.day_spending;
		shop.share.sunshine_share 	-= shop.share_cache.sunshine_share;
		shop.share.top_share		-= shop.share_cache.top_share;
		shop.top_rewarded_count 	= 0;
		shop.updated_at 			= now;

		shop.share_cache.reset();
		shop.last_sunshine_reward_spend_idx.reset();

		_dbc.set( shop );

		return true;
	}

	return !processed;
}

/**
 * Usage: to reward shop spending for all shops
 */
ACTION hst_mall::rewardshops(const uint64_t& shop_id) {
	_check_rewarded( _gstate2.last_shops_rewarded_at );

	if (shop_id != 0) {
		_reward_shop(shop_id);
		return;
	}

	shop_t::tbl_t shops(_self, _self.value);
	auto itr = shops.upper_bound(_gstate2.last_reward_shop_id);
	uint8_t step = 0; 

	// string res = "";
	for (; itr != shops.end() && step < MAX_STEP; itr++, step++) {
		auto shop_id = itr->id;
		if (!_reward_shop(shop_id)) {
			// check( false, "shop not done: " + to_string(shop_id) );
			return; // this shop not finished, needs to re-enter in next round of this
		}

		// res += " " + to_string(shop_id);
		_gstate2.last_reward_shop_id = shop_id;
	}
	// res += "\n _gstate2.last_reward_shop_id = " + to_string(_gstate2.last_reward_shop_id );
	// check(false, 	"shops: " + res);

	if (itr == shops.end()) {
		_gstate2.last_reward_shop_id = 0;
		_gstate2.last_shops_rewarded_at = time_point_sec( current_time_point() );
		_gstate.executing = false;
	}
}

/**
 * Usage: to reward newly certified users
 */
ACTION hst_mall::rewardcerts() {
	_check_rewarded( _gstate2.last_certification_rewarded_at );

	update_share_cache();
	CHECK( _gstate.platform_share_cache.certified_user_count > 0, "Err: certified user count is zero" )
	auto quant = _gstate.platform_share_cache.certified_user_share / _gstate.platform_share_cache.certified_user_count;
	certification_t::tbl_t certifications(_self, _self.value);
	auto itr = certifications.begin();
	auto now = time_point_sec( current_time_point() );
	uint8_t step = 0;

	for (; itr != certifications.end() && step < MAX_STEP; step++) {
		TRANSFER( _cstate.mall_bank, itr->user, quant, "cert reward" )
		_log_reward( itr->user, NEW_CERT_REWARD, quant, now);

		itr = certifications.erase(itr); //remove it after rewarding
		_gstate2.last_certification_reward_step++;
	}

	if (step == 0 || 
		_gstate2.last_certification_reward_step == _gstate.platform_share_cache.certified_user_count) {
			
		_gstate.platform_share.certified_user_share 		-= _gstate.platform_share_cache.certified_user_share;
		_gstate.platform_share_cache.certified_user_share 	= _gstate.platform_share.certified_user_share;
		_gstate.platform_share.certified_user_count 		= 0;
		_gstate2.last_certification_reward_step 			= 0;
		_gstate2.last_certification_rewarded_at 			= now;
	}
}

/**
 *  Usage: to reward platform top 1000
 */
ACTION hst_mall::rewardptops() {
	_check_rewarded( _gstate2.last_platform_reward_finished_at );
	check( _gstate.platform_share_cache.top_share.amount > 0, "none top share" );

	user_t::tbl_t users(_self, _self.value);
	auto user_idx = users.get_index<"totalshare"_n>();
	auto itr = user_idx.upper_bound(_gstate2.last_platform_top_reward_id);
	check( _cstate.platform_top_count > 0, "Err: _cstate.platform_top_count = 0" );
	auto quant_avg = _gstate.platform_share_cache.top_share / _cstate.platform_top_count;
	auto now = time_point_sec( current_time_point() );
	uint8_t step = 0;

	bool processed = false;
	bool ended = false;
	for (; itr != user_idx.end() && step < MAX_STEP; itr++, step++) {
		_gstate2.last_platform_top_reward_step++;
		if (_gstate2.last_platform_top_reward_step > _cstate.platform_top_count || 	// top-1000 reward
			itr->share_cache.total_share().amount == 0) {							//sequential and it's the end
			ended = true;

			// check( itr->share_cache.total_share().amount > 0, "step: " + to_string(step) + ", 0 total_share from itr->account: " + itr->account.to_string() );
			break; 
		}

		TRANSFER( _cstate.mall_bank, itr->account, quant_avg, "platform top reward" )
		_log_reward( itr->account, PLATFORM_TOP_REWARD, quant_avg, now);
		_gstate2.last_platform_top_reward_id = itr->by_total_share();

		processed = true;
	}

	// CHECK( processed, "none processed" )

	if ( ended || itr == user_idx.end()) {
		_gstate.platform_share.top_share 			-= _gstate.platform_share_cache.top_share;
		_gstate.platform_share.total_share 			-= _gstate.platform_share_cache.top_share;
		_gstate.platform_share_cache.top_share 		= _gstate.platform_share.top_share;
		_gstate.platform_share_cache.total_share 	= _gstate.platform_share.total_share;

		_gstate2.last_platform_top_reward_step 		= 0;
		_gstate2.last_platform_top_reward_id		= 0;
		_gstate2.last_platform_reward_finished_at 	= now;
	}
}

/**
 * Usage: This is to withdraw one's share in mining pools
 * 
 * @issuer: either user or platform admin who withdraws for user
 * @to: the targer user to receive withdrawn amount
 * @withdraw_type: 
 * 			0: withdraw all;
 * 			1: withdraw spending share; 
 * 			2: withdraw referrer share; 
 * 			3: withdraw agent share
 * 
 * @shop_id: the specific shop to withdraw its spending reward when withdraw_type is 0
 * 			 when shop_id = 0, it withdraws stakes from all shops, otherwise from a specific given shop_id
 *
 */
ACTION hst_mall::withdraw(const name& issuer, const name& to, const uint8_t& withdraw_type, const uint64_t& shop_id) {
	require_auth( issuer );
	CHECK( is_account(to), "to account not valid: " + to.to_string() )
	CHECK( withdraw_type < 4, "withdraw_type not valid: " + to_string(withdraw_type) )
	CHECK( issuer == _cstate.platform_admin || issuer == to, "issuer (" + issuer.to_string() + ") not an admin (" + 
		_cstate.platform_admin.to_string() + ", neither a to user: " + to.to_string() )

	user_t user(to);
	CHECK( _dbc.get(user), "to user not found: " + to.to_string() )
	CHECK( issuer == _cstate.platform_admin || issuer == to, "withdraw other's reward not allowed" )
	
	auto now = current_time_point();
	auto withdrawn = asset(0, HST_SYMBOL);
	spending_t::tbl_t spends(_self, _self.value);

	switch( withdraw_type ) {
	case 0: //to withdraw everything
		CHECK( shop_id == 0, "shop_id none-zero error" )

		withdrawn += _withdraw_shops(user);
		withdrawn += _withdraw_customer_referral(user);
		withdrawn += _withdraw_shop_referral(user);
		break;

	case 1: //to withdraw spending share
		withdrawn += (shop_id > 0) ? _withdraw_shop(shop_id, user, spends, true) : _withdraw_shops(user);
		break;

	case 2: //to withdraw customer referral reward
		withdrawn += _withdraw_customer_referral(user);
		break;

	case 3: //to withdraw shop referral reward
		withdrawn += _withdraw_shop_referral(user);
		break;
	}

	CHECK( withdrawn.amount > 0, "none withdrawn" )

	withdraw_t::tbl_t withdraws(_self, _self.value);
	withdraws.emplace(_self, [&](auto& row) {
		row.id = withdraws.available_primary_key();
		row.withdrawer = issuer;
		row.withdrawee = to;
		row.withdraw_type = withdraw_type;
		row.withdraw_shopid = shop_id;
		row.quantity = withdrawn;
		row.created_at = time_point_sec(current_time_point());
	});

}

asset hst_mall::_withdraw_shop(const uint64_t& shop_id, user_t& user, spending_t::tbl_t& spends, bool del_spend) {
	auto idx = spends.get_index<"shopcustidx"_n>();
	auto key = (uint128_t) shop_id << 64 | user.account.value;
	auto l_itr = idx.lower_bound( key );
	auto u_itr = idx.upper_bound( key );
	CHECK( l_itr != u_itr && l_itr != idx.end(), "customer: " + user.account.to_string() + " @ shop: " + to_string(shop_id) + " not found" )
	auto quant = l_itr->share_cache.total_spending;
	CHECK( quant.amount > 0, "Err: zero shop total spending of customer: " +  user.account.to_string() )
	CHECK( user.share_cache.spending_share >= quant, "Err: user.share_cache.spending_reward: " 
				+ user.share_cache.spending_share.to_string() + " < itr->share_cache.total_spending: " 
				+ l_itr->share_cache.total_spending.to_string() )

	CHECK( _gstate.platform_share.total_share >= quant, "platform total share insufficient to withdraw" )
	_gstate.platform_share.total_share -= quant;
	update_share_cache();
	
	user.share.spending_share -= quant;
	update_share_cache( user );
	_dbc.set( user );

	if (del_spend) 
		idx.erase(l_itr);

	asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
	CHECK( platform_fees < quant, "Err: withdrawl fees oversized!" )

	TRANSFER( _cstate.mall_bank, _cstate.platform_fee_collector, platform_fees, "fees" )
	TRANSFER( _cstate.mall_bank, user.account, quant - platform_fees, "spend reward" )

	return quant;
}

asset hst_mall::_withdraw_shops(user_t& user) {
	//withdraw all shops, it might run out of time limit and thus throw exception
	spending_t::tbl_t spends(_self, _self.value);
	auto to = user.account;
	auto idx = spends.get_index<"custidx"_n>();
	auto lower_itr = idx.lower_bound( to.value );
	auto upper_itr = idx.upper_bound( to.value );
	auto total_withdrawn = asset(0, HST_SYMBOL);

	//iterate thru all shops of a user spent at
	// string res = "";
	for (auto itr = lower_itr; itr != upper_itr && itr != idx.end();) {
		// res += to_string(itr->id) + ",";

		total_withdrawn += _withdraw_shop(itr->shop_id, user, spends, false);
		itr = idx.erase(itr);
	}

	// check(false, "to: " + to.to_string() + "\n res: " + res);
	return total_withdrawn;
}

asset hst_mall::_withdraw_customer_referral(user_t& user) {
	auto quant = user.share.customer_referral_share;
	if (quant.amount == 0)
		return quant;

	user.share.customer_referral_share.amount = 0;
	update_share_cache( user );
	_dbc.set( user );

	asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
	CHECK( platform_fees < quant, "Err: withdrawl fees oversized!" )
	auto to = user.account;

	TRANSFER( _cstate.mall_bank, _cstate.platform_fee_collector, platform_fees, "customer ref fees" )
	TRANSFER( _cstate.mall_bank, to, quant - platform_fees, "customer ref reward" )

	return quant;
}

asset hst_mall::_withdraw_shop_referral(user_t& user) {
	auto quant = user.share.shop_referral_share;
	if (quant.amount == 0)
		return quant;

	user.share.shop_referral_share.amount = 0;
	update_share_cache( user );
	_dbc.set( user );
	
	asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
	CHECK( platform_fees < quant, "Err: withdrawl fees oversized!" )
	auto to = user.account;

	TRANSFER( _cstate.mall_bank, _cstate.platform_fee_collector, platform_fees, "shop ref fees" )
	TRANSFER( _cstate.mall_bank, to, quant - platform_fees, "shop ref reward" )

	return quant;
}

// bool hst_mall::update_all_caches() {

// 	update_share_cache(); // platform share cache

// 	user_t::tbl_t users(_self, _self.value);
// 	auto user_idx = users.get_index<"cacheupdt"_n>();
// 	auto user_itr = user_idx.upper_bound(_gstate2.last_cache_update_useridx);
// 	for (auto step = 0; user_itr != user_idx.end() && step < MAX_STEP; user_itr++, step++) {
// 		user_t user(user_itr->account);
// 		CHECK( _dbc.get(user), "Err: user not found: " + user_itr->account.to_string() )
// 		update_share_cache(user);
// 	}
// 	if (user_itr != user_idx.end() && !user_itr->share_cache_updated) return false;

// 	shop_t::tbl_t shops(_self, _self.value);
// 	auto shop_idx = shops.get_index<"cacheupdt"_n>();
// 	auto shop_itr = shop_idx.upper_bound(_gstate2.last_cache_update_shopidx);
// 	for (auto step = 0; shop_itr != shop_idx.end() && step < MAX_STEP; shop_itr++, step++) {
// 		shop_t shop(shop_itr->id);
// 		CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_itr->id) )
// 		update_share_cache(shop);
// 	}
// 	if (shop_itr != shop_idx.end() && !shop_itr->share_cache_updated) return false;

// 	spending_t::tbl_t spends(_self, _self.value);
// 	auto spend_idx = spends.get_index<"cacheupdt"_n>();
// 	auto spend_itr = spend_idx.upper_bound(_gstate2.last_cache_update_spendingidx);
// 	for (auto step = 0; spend_itr != spend_idx.end() && step < MAX_STEP; spend_itr++, step++) {
// 		spending_t spend(spend_itr->id);
// 		CHECK( _dbc.get(spend), "Err: spend not found: " + to_string(spend_itr->id) )
// 		update_share_cache(spend);
// 	}
// 	if (spend_itr != spend_idx.end() && !spend_itr->share_cache_updated) return false;

// 	return true;
// }

} /// namespace ayj

#include <aiyunji.mall/aiyunji.mall.hpp>

namespace ayj {

void ayj_mall::init() {

}

void ayj_mall::deposit(name from, name to, asset quantity, string memo) {
	if (to != _self) return;

    donate_idx_t donates(_self, _self.value);
	auto donate_id = donates.available_primary_key();
	donates.emplace( _self, [&]( auto& row ) {
        row.quantity = quantity;
        row.donated_at = current_time_point();
        row.expired_at = row.donated_at + microseconds(_gstate.donate_coin_expiry_days * seconds_per_day);
    });
    
}

} /// namespace ayj
